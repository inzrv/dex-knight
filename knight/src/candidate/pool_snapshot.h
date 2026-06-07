#pragma once

#include "evm/contracts/pool.h"

#include <boost/json.hpp>

#include <optional>

namespace candidate
{

struct PoolSnapshot final
{
    evm::Pool pool;
    intx::uint256 reserve_a;
    intx::uint256 reserve_b;

    static std::optional<PoolSnapshot> from_json(const evm::Pool& pool, const boost::json::value& value);
};

} // namespace candidate
