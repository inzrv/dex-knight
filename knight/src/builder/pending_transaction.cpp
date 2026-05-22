#include "builder/pending_transaction.h"

#include "utils/utils.h"

#include <boost/json.hpp>
#include <utility>

namespace builder
{

std::optional<PendingTransaction> PendingTransaction::from_json(const boost::json::value& value)
{
    if (!value.is_object()) {
        return std::nullopt;
    }

    const auto& record = value.as_object();
    const auto seq_num = json_uint64(record, "seqNum");
    const auto* transaction_json = record.if_contains("transaction");
    if (!seq_num || transaction_json == nullptr) {
        return std::nullopt;
    }

    auto transaction = evm::Transaction::from_json(*transaction_json);
    if (!transaction) {
        return std::nullopt;
    }

    PendingTransaction pending;
    static_cast<evm::Transaction&>(pending) = std::move(*transaction);
    pending.seq_num = *seq_num;

    if (auto mempool_tx_id = json_string(record, "mempoolTxId")) {
        pending.mempool_tx_id = std::move(*mempool_tx_id);
    }
    if (auto status = json_string(record, "status")) {
        pending.status = std::move(*status);
    }
    if (auto submitted_at = json_string(record, "submittedAt")) {
        pending.submitted_at = std::move(*submitted_at);
    }

    return pending;
}

} // namespace builder
