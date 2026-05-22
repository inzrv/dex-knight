#include "builder/pending_snapshot.h"

#include "utils/utils.h"

#include <utility>

namespace builder
{

std::optional<PendingSnapshot> PendingSnapshot::from_json(const boost::json::object& json, uint64_t block_number)
{
    const auto snapshot_seq = json_uint64(json, "snapshotSeq");
    const auto* transactions = json_array(json, "transactions");
    if (!snapshot_seq || transactions == nullptr) {
        return std::nullopt;
    }

    PendingSnapshot snapshot{
        .block_number = block_number,
        .snapshot_seq = *snapshot_seq,
    };

    for (const auto& value : *transactions) {
        auto transaction = PendingTransaction::from_json(value);
        if (transaction) {
            snapshot.transactions.push_back(std::move(*transaction));
        }
    }

    return snapshot;
}

} // namespace builder
