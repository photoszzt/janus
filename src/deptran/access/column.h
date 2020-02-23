//
// Created by chance_Lv on 2020/2/14.
//
#pragma once

#include "constants.h"
#include "tx.h"
#include <memdb/value.h>
#include "safeguard.h"

namespace janus {
    struct SSID {  // SSID defines an ssid range by low--high
        txnid_t ssid_low;
        txnid_t ssid_high;
        SSID() : ssid_low(0), ssid_high(0) {}
        SSID(snapshotid_t low, snapshotid_t high) : ssid_low(low), ssid_high(high) {}
        SSID(const SSID& that) = default;
        SSID& operator=(const SSID& that) {
            if (this == &that) {
                return *this;
            }
            this->ssid_low = that.ssid_low;
            this->ssid_high = that.ssid_high;
            return *this;
        }
    };
    struct AccTxnRec {
        // an access txn record stored in txn queue.
        // This is the element of a node in LinkedVector
        // Each txn record must be a write
        txnid_t txn_id;
        SSID ssid;              // snapshot id
        acc_status_t status;    // either unchecked, validating, or finalized
        mdb::Value value;       // the value of this write

        AccTxnRec() : txn_id(-1), ssid(), status(UNCHECKED) {}
        explicit AccTxnRec(mdb::Value&& v, txnid_t tid = 0, acc_status_t stat = UNCHECKED)
                : txn_id(tid), ssid(), status(stat), value(std::move(v)) {}
        AccTxnRec(AccTxnRec&& that) noexcept
                : txn_id(that.txn_id),
                  ssid(that.ssid),
                  status(that.status),
                  value(std::move(that.value)) {}
        AccTxnRec(const AccTxnRec&) = delete;
        AccTxnRec& operator=(const AccTxnRec&) = delete;
        AccTxnRec(mdb::Value&& v, txnid_t tid, const SSID& ss_id, acc_status_t stat = UNCHECKED)
                : txn_id(tid), ssid(ss_id), status(stat), value(std::move(v)) {}
        const mdb::Value & get_value() const {
            return value;
        }
    };
    // an AccColumn has a vector of queued txns
    class AccColumn {
    public:
        //friend class AccRow;
        AccColumn();
        explicit AccColumn(colid_t, mdb::Value&& v);
        AccColumn(const AccColumn&) = delete;   // no copy
        AccColumn& operator=(const AccColumn&) = delete; // no copy
        const mdb::Value& read(bool& validate_abort, SSID& ssid, unsigned long& index) const;
        SSID write(mdb::Value&& v, txnid_t tid, unsigned long& ver_index, bool& is_decided);
        bool all_recent_aborted(unsigned long index) const;
        //static void update_metadata(MetaData& metadata, const SSID& ssid);
    private:
        colid_t col_id = -1;
        // a vector of txns as a versioned column --> txn queue
        std::vector<AccTxnRec> txn_queue;
        const int INITIAL_QUEUE_SIZE = 100;
        //SSID ssid_cur;
        unsigned long finalized_version = 0;  // the index points to the most recent finalized version
        unsigned long decided_head = 0;         // before which inclusive all writes are either finalized or aborted
        void update_finalized();    // called whenever commit a write
        bool is_logical_head(unsigned long index) const;
        SSID get_next_SSID() const;
        const AccTxnRec& logical_head() const;
        unsigned long logical_head_index() const;
        void update_decided_head();
        bool all_decided(unsigned long index) const;
        friend class AccRow;
    };
}
