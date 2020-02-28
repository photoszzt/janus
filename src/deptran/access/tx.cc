#include "tx.h"

namespace janus {
    // TODO: fill in these stubs
    bool AccTxn::ReadColumn(Row *row,
                            colid_t col_id,
                            Value* value,
                            int hint_flag) {
        verify(row != nullptr);
        // class downcasting to get AccRow
        auto acc_row = dynamic_cast<AccRow*>(row);
        unsigned long index = 0;
        snapshotid_t ssid = acc_row->read_column(col_id, value, sg.ssid_spec, sg.offset_safe, index, sg.decided);
        row->ref_copy();
        sg.metadata.indices[row][col_id] = index;  // for later validation
	    //sg.metadata.earlier_read[row][col_id] = ssid.ssid_high;  // for wild card optimization, i.e., read ssid [0, x]
        // update metadata
        sg.update_metadata(ssid);
        return true;
    }

    bool AccTxn::ReadColumns(Row *row,
                             const std::vector<colid_t> &col_ids,
                             std::vector<Value>* values,
                             int hint_flag) {
        verify(row != nullptr);
        auto acc_row = dynamic_cast<AccRow*>(row);
        row->ref_copy();
        for (auto col_id : col_ids) {
            Value *v = nullptr;
            unsigned long index = 0;
            snapshotid_t ssid = acc_row->read_column(col_id, v, sg.ssid_spec, sg.offset_safe, index, sg.decided);
            verify(v != nullptr);
            values->push_back(std::move(*v));
            sg.metadata.indices[row][col_id] = index;  // for later validation
	        //sg.metadata.earlier_read[row][col_id] = ssid.ssid_high;  // for wild card optimization, i.e., read ssid [0, x]
            // update metadata
            sg.update_metadata(ssid);
        }
        return true;
    }

    bool AccTxn::WriteColumn(Row *row,
                             colid_t col_id,
                             Value& value,
                             int hint_flag) {
        verify(row != nullptr);
        auto acc_row = dynamic_cast<AccRow*>(row);
        unsigned long ver_index = 0;
        snapshotid_t ssid = acc_row->write_column(col_id, std::move(value), sg.ssid_spec, this->tid_, ver_index, sg.offset_safe); // ver_index is new write
        row->ref_copy();
	    sg.update_metadata(ssid);
        sg.metadata.indices[row][col_id] = ver_index; // for validation and finalize
        return true;
    }

    bool AccTxn::WriteColumns(Row *row,
                              const std::vector<colid_t> &col_ids,
                              std::vector<Value>& values,
                              int hint_flag) {
        verify(row != nullptr);
        auto acc_row = dynamic_cast<AccRow*>(row);
        int v_counter = 0;
        row->ref_copy();
        for (auto col_id : col_ids) {
            unsigned long ver_index = 0;
            snapshotid_t ssid = acc_row->write_column(col_id, std::move(values[v_counter++]), sg.ssid_spec, this->tid_, ver_index, sg.offset_safe);
            sg.update_metadata(ssid);
            sg.metadata.indices[row][col_id] = ver_index; // for validation and finalize
        }
        return true;
    }

    AccTxn::~AccTxn() {
        // TODO: fill in if holding any resources
    }

    void AccTxn::load_speculative_ssid(snapshotid_t ssid) {
        sg.ssid_spec = ssid;
    }
}
