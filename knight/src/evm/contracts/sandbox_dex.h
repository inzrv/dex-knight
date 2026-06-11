#pragma once

#include "common/types.h"

#include <solabi/decoder.h>
#include <solabi/utils.h>

namespace evm
{

struct SandboxDex
{
    inline static constexpr intx::uint256 kFeeNumerator{997};
    inline static constexpr intx::uint256 kFeeDenominator{1000};

    // function seedLiquidity(uint256 amountA, uint256 amountB)
    struct SeedLiquidity
    {
        using abi_tag = solabi::tuple_t<solabi::uint_t<256>, solabi::uint_t<256>>;

        inline static const bytes kSelector{solabi::from_hex("0x8d7e076d")};

        intx::uint256 amount_a;
        intx::uint256 amount_b;
    };

    // function getReserves() returns (uint256 reserveA, uint256 reserveB)
    struct GetReserves
    {
        using abi_tag = solabi::tuple_t<solabi::uint_t<256>, solabi::uint_t<256>>;

        inline static const bytes kSelector{solabi::from_hex("0x0902f1ac")};

        intx::uint256 reserve_a;
        intx::uint256 reserve_b;
    };

    // function swapExactAForB(uint256 amountIn, uint256 minAmountOut)
    struct SwapExactAForB
    {
        using abi_tag = solabi::tuple_t<solabi::uint_t<256>, solabi::uint_t<256>>;

        inline static const bytes kSelector{solabi::from_hex("0x4cabca0a")};

        intx::uint256 amount_in;
        intx::uint256 min_amount_out;
    };

    // function swapExactBForA(uint256 amountIn, uint256 minAmountOut)
    struct SwapExactBForA
    {
        using abi_tag = solabi::tuple_t<solabi::uint_t<256>, solabi::uint_t<256>>;

        inline static const bytes kSelector{solabi::from_hex("0xdd4f1611")};

        intx::uint256 amount_in;
        intx::uint256 min_amount_out;
    };
};

} // namespace evm
