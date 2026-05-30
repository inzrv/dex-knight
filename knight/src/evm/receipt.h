#pragma once

#include "common/types.h"

#include <boost/json.hpp>

#include <cstdint>
#include <optional>

namespace evm
{

struct Receipt final
{
    uint8_t type{0};
    uint64_t status{0};
    uint64_t cumulative_gas_used{0};
    boost::json::array logs;
    bytes logs_bloom;
    bytes transaction_hash;
    uint64_t transaction_index{0};
    bytes block_hash;
    uint64_t block_number{0};
    uint64_t gas_used{0};
    intx::uint256 effective_gas_price{};
    std::optional<intx::uint256> blob_gas_price;
    bytes from;
    std::optional<bytes> to;
    std::optional<bytes> contract_address;
    std::optional<uint64_t> block_timestamp;

    static std::optional<Receipt> from_json(const boost::json::value& value);

    boost::json::object to_json() const;
};

} // namespace evm
