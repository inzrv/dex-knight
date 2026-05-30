#pragma once

#include "common/types.h"

#include <boost/json.hpp>

#include <cstdint>
#include <optional>
#include <vector>

namespace evm
{

struct Log final
{
    bytes address;
    std::vector<bytes> topics;
    bytes data;

    static std::optional<Log> from_json(const boost::json::value& value);

    boost::json::object to_json() const;
};

} // namespace evm
