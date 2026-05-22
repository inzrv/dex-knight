#pragma once

#include "builder/pending_transaction.h"
#include "common/time.h"

#include <cstdint>
#include <string>
#include <variant>

namespace candidate
{

struct NewBlockEvent
{
    latency_time_point ingress_time;
    std::string source;
    uint64_t block_number{0};
};

struct PendingTransactionEvent
{
    latency_time_point ingress_time;
    std::string source;
    builder::PendingTransaction tx;
};

using Event = std::variant<NewBlockEvent, PendingTransactionEvent>;

} // namespace candidate
