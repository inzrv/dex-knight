#include "candidate/mempool_snapshot.h"

#include "utils/utils.h"

#include <utility>

namespace candidate
{

std::optional<MempoolSnapshot> MempoolSnapshot::from_json(const boost::json::object& json, uint64_t block_number)
{
    const auto snapshot_seq = json_uint64(json, "snapshotSeq");
    const auto* transactions = json_array(json, "transactions");
    if (!snapshot_seq || transactions == nullptr) {
        return std::nullopt;
    }

    MempoolSnapshot snapshot{
        .block_number = block_number,
        .snapshot_seq = *snapshot_seq,
    };

    for (const auto& value : *transactions) {
        auto candidate = PendingTx::from_json(value);
        if (candidate) {
            snapshot.candidates.push_back(std::move(*candidate));
        }
    }

    return snapshot;
}

} // namespace candidate
