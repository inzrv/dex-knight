#pragma once

#include "common/types.h"

#include <cstdint>

namespace arbitrage
{

struct Params final
{
    intx::uint256 amount_in_b{};
    intx::uint256 min_profit_b{};
    uint16_t min_output_bps{10'000};
};

} // namespace arbitrage
