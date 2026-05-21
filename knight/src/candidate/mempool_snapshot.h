#pragma once

#include "common/pending_tx.h"

#include <boost/json.hpp>

#include <cstdint>
#include <optional>
#include <vector>

namespace candidate
{

struct MempoolSnapshot final
{
    uint64_t block_number{0};
    uint64_t snapshot_seq{0};
    std::vector<PendingTx> candidates;

    static std::optional<MempoolSnapshot> from_json(const boost::json::object& json, uint64_t block_number);
};

} // namespace candidate
