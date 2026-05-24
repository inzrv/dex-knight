#pragma once

#include "common/types.h"

#include <solabi/decoder.h>

namespace evm
{

struct SandboxDex
{
    // function seedLiquidity(uint256 amountA, uint256 amountB)
    struct SeedLiquidity
    {
        using abi_tag = solabi::tuple_t<solabi::uint_t<256>, solabi::uint_t<256>>;

        inline static const bytes selector{0x8d, 0x7e, 0x07, 0x6d};

        intx::uint256 amount_a;
        intx::uint256 amount_b;
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
