#include "common/pending_tx.h"

#include "utils/utils.h"

#include <boost/json.hpp>

std::optional<PendingTx> PendingTx::from_json(const boost::json::value& value)
{
    if (!value.is_object()) {
        return std::nullopt;
    }

    const auto& record = value.as_object();
    const auto seq_num = json_uint64(record, "seqNum");
    if (!seq_num) {
        return std::nullopt;
    }

    return PendingTx{
        .seq_num = *seq_num,
        .payload = boost::json::serialize(value),
    };
}
