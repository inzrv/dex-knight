#pragma once

#include "builder/pending_transaction.h"

#include <boost/json.hpp>

#include <cstdint>
#include <optional>
#include <vector>

namespace builder
{

struct PendingSnapshot final
{
    uint64_t block_number{0};
    uint64_t snapshot_seq{0};
    std::vector<PendingTransaction> transactions;

    static std::optional<PendingSnapshot> from_json(const boost::json::object& json,
                                                    uint64_t block_number);
};

} // namespace builder
