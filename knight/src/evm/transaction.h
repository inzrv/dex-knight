#pragma once

#include "common/types.h"

#include <boost/json.hpp>

#include <cstdint>
#include <optional>

namespace evm
{

struct Transaction
{
    std::optional<bytes> hash;
    bytes input;
    bytes from;
    std::optional<uint64_t> chain_id;
    intx::uint256 value{};
    std::optional<intx::uint256> gas_price;
    uint64_t gas{0};
    std::optional<intx::uint256> max_fee_per_gas;
    std::optional<intx::uint256> max_priority_fee_per_gas;
    uint64_t nonce{0};
    uint8_t type{0};
    std::optional<bytes> to;

    static std::optional<Transaction> from_json(const boost::json::value& value);

    boost::json::object to_json() const;
};

} // namespace evm
