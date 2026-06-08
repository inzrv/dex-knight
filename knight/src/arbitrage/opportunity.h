#pragma once

#include "common/types.h"
#include "evm/contracts/pool.h"

namespace arbitrage
{

struct Opportunity final
{
    evm::Pool buy_pool;
    evm::Pool sell_pool;

    intx::uint256 amount_in_b{};
    intx::uint256 min_amount_out_a{};
    intx::uint256 min_amount_out_b{};
    intx::uint256 min_profit_b{};

    intx::uint256 expected_amount_out_a{};
    intx::uint256 expected_amount_out_b{};
    intx::uint256 expected_profit_b{};
};

} // namespace arbitrage
