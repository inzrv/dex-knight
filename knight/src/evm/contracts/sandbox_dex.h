#pragma once

#include "common/types.h"

#include <solabi/decoder.h>

namespace evm
{

struct SandboxDex
{
    inline static constexpr intx::uint256 fee_numerator{997};
    inline static constexpr intx::uint256 fee_denominator{1000};

    // function seedLiquidity(uint256 amountA, uint256 amountB)
    struct SeedLiquidity
    {
        using abi_tag = solabi::tuple_t<solabi::uint_t<256>, solabi::uint_t<256>>;

        inline static const bytes selector{0x8d, 0x7e, 0x07, 0x6d};

        intx::uint256 amount_a;
        intx::uint256 amount_b;
    };

    // function getReserves() returns (uint256 reserveA, uint256 reserveB)
    struct GetReserves
    {
        using abi_tag = solabi::tuple_t<solabi::uint_t<256>, solabi::uint_t<256>>;

        inline static const bytes selector{0x09, 0x02, 0xf1, 0xac};

        intx::uint256 reserve_a;
        intx::uint256 reserve_b;
    };

    // function swapExactAForB(uint256 amountIn, uint256 minAmountOut)
    struct SwapExactAForB
    {
        using abi_tag = solabi::tuple_t<solabi::uint_t<256>, solabi::uint_t<256>>;

        inline static const bytes selector{0x4c, 0xab, 0xca, 0x0a};

        intx::uint256 amount_in;
        intx::uint256 min_amount_out;
    };

    // function swapExactBForA(uint256 amountIn, uint256 minAmountOut)
    struct SwapExactBForA
    {
        using abi_tag = solabi::tuple_t<solabi::uint_t<256>, solabi::uint_t<256>>;

        inline static const bytes selector{0xdd, 0x4f, 0x16, 0x11};

        intx::uint256 amount_in;
        intx::uint256 min_amount_out;
    };
};

} // namespace evm
