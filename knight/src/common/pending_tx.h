#pragma once

#include <boost/json.hpp>

#include <cstdint>
#include <optional>
#include <string>

struct PendingTx final
{
    uint64_t seq_num{0};
    std::string payload;

    static std::optional<PendingTx> from_json(const boost::json::value& value);
};
