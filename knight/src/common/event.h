#pragma once

#include "time.h"

#include <cstdint>
#include <string>
#include <variant>

struct NewBlockEvent
{
    latency_time_point ingress_time;
    std::string source;
    uint64_t block_number{0};
};

struct PendingTxEvent
{
    latency_time_point ingress_time;
    std::string source;
    std::string payload;
};

using Event = std::variant<NewBlockEvent, PendingTxEvent>;
