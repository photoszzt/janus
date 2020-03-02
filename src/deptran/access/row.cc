#include <memdb/value.h>
#include "row.h"

namespace janus {
    AccRow* AccRow::create(const mdb::Schema *schema,
                           std::vector<mdb::Value>& values) {
        std::vector<const mdb::Value*> values_ptr(values.size(), nullptr);
        size_t fill_counter = 0;
        for (auto& value : values) {
            fill_values_ptr(schema, values_ptr, value, fill_counter);
            fill_counter++;
        }
        auto* raw_row = new AccRow();
        auto* new_row = (AccRow*)mdb::Row::create(raw_row, schema, values_ptr);
        for (auto col_info_it = schema->begin(); col_info_it != schema->end(); col_info_it++) {
            int col_id = schema->get_column_id(col_info_it->name);
            new_row->_row.emplace(
                    std::piecewise_construct,
                    std::make_tuple(col_id),
                    std::make_tuple(col_id, std::move(values.at(col_id))));
        }
        return new_row;
    }

    SSID AccRow::read_column(mdb::colid_t col_id, mdb::Value* value, snapshotid_t ssid_spec, bool& offset_safe, unsigned long& index, bool& decided) {
        if (col_id >= _row.size()) {
            // col_id is invalid. We're doing a trick here.
            // keys are col_ids
            value = nullptr;
            return {};
        }
        SSID ssid;
        *value = _row.at(col_id).read(ssid_spec, ssid, offset_safe, index, decided);
        return ssid;
    }

    SSID AccRow::write_column(mdb::colid_t col_id, mdb::Value&& value, snapshotid_t ssid_spec, txnid_t tid, unsigned long& ver_index, bool& offset_safe) {
        if (col_id >= _row.size()) {
            return {};
        }
        // push back to the txn queue
        return _row.at(col_id).write(std::move(value), ssid_spec, tid, ver_index, offset_safe);
    }

    bool AccRow::validate(txnid_t tid, mdb::colid_t col_id, unsigned long index, snapshotid_t ssid_new, bool validate_consistent, bool& decided) {
        if (_row[col_id].is_read(tid, index)) {
            // this is validating a read
            if (!validate_consistent || !_row[col_id].is_logical_head(index)) {
                return false;
            }
            _row[col_id].txn_queue[index].extend_ssid(ssid_new);  // extend ssid range for reads
            // check decided, only need for reads and if consistent
            decided = _row[col_id].txn_queue[index].status == FINALIZED;
            return true;
        } else {
            // validating a write
            if (validate_consistent && _row[col_id].is_logical_head(index) && !_row[col_id].txn_queue[index].have_reads()) { // 2nd part is particularly required for safety for speculative execution now
                _row[col_id].txn_queue[index].update_ssid(ssid_new);  // update both low and high to new for writes
                _row[col_id].txn_queue[index].status = VALIDATING;
                return true;
            }
            _row[col_id].txn_queue[index].status = ABORTED;  // will abort anyways
            // TODO: DO CASCADING ABORTS LOGIC (early aborts)
            return false;
        }
    }

    void AccRow::finalize(txnid_t tid, mdb::colid_t col_id, unsigned long ver_index, int8_t decision) {
        if (_row[col_id].is_read(tid, ver_index)) {
            // deciding a read, nothing to do
            // TODO: should not send rpcs for deciding reads in the first place
            return;
        }
        _row[col_id].finalize(ver_index, decision);
    }

    int8_t AccRow::check_status(txnid_t tid, mdb::colid_t col_id, unsigned long index) {
        if (!_row[col_id].is_read(tid, index)) {
            // this was a write, no need to check status
            return FINALIZED;
        }
        switch (_row[col_id].txn_queue[index].status) {
            case UNCHECKED: return UNCHECKED;
            case VALIDATING: return VALIDATING;
            case FINALIZED: return FINALIZED;
            case ABORTED: return ABORTED;
            default: verify(0); return FINALIZED;
        }
    }
}
