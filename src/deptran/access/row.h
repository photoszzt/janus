//
// Created by chance_Lv on 2020/2/5.
//
#pragma once

//#include "linked_vector.h"

#include "column.h"
#include "tx.h"
#include <unordered_map>
#include <memdb/row.h>
/*
 * Defines the row structure in ACCESS.
 * A row is a map of txn_queue (linked_vector); a txn_queue for each col
 */
namespace janus {
    class AccRow : public mdb::Row {
    public:
        static AccRow* create(const mdb::Schema *schema, std::vector<mdb::Value>& values);
        snapshotid_t read_column(mdb::colid_t col_id, mdb::Value* value, snapshotid_t ssid_spec, bool& offset_safe, unsigned long& index) const;
        snapshotid_t write_column(mdb::colid_t col_id, mdb::Value&& value, snapshotid_t ssid_spec, txnid_t tid, unsigned long& ver_index, bool& offset_safe);
        bool validate(txnid_t tid, mdb::colid_t col_id, unsigned long index, snapshotid_t ssid_new, bool validate_consistent);
        void finalize(txnid_t tid, mdb::colid_t col_id, unsigned long ver_index, int8_t decision);
    private:
        // a map of txn_q; keys are cols, values are linkedvectors that holding txns (versions)
        std::unordered_map<mdb::colid_t, AccColumn> _row;
    };
}   // namespace janus
