#pragma once

#include "common/types.h"

#include <solabi/types.h>

namespace evm
{

struct SandboxBackrun
{
    // event BackrunExecuted(
    //     address indexed buyPool,
    //     address indexed sellPool,
    //     uint256 amountInB,
    //     uint256 amountOutA,
    //     uint256 amountOutB,
    //     uint256 profitB
    // )
    struct BackrunExecuted
    {
        using data_abi_tag = solabi::tuple_t<solabi::uint_t<256>,
                                             solabi::uint_t<256>,
                                             solabi::uint_t<256>,
                                             solabi::uint_t<256>>;

        inline static const bytes kTopic{0xf1, 0x0a, 0x5f, 0x8a, 0x7c, 0x6a, 0x46, 0x93,
                                         0xcc, 0xca, 0x3e, 0xd1, 0x76, 0x69, 0xda, 0x82,
                                         0x4f, 0x6a, 0x42, 0xe8, 0x51, 0x01, 0xb8, 0x03,
                                         0x93, 0x70, 0xbd, 0xb4, 0xa8, 0x5f, 0x98, 0xca};

        bytes buy_pool;
        bytes sell_pool;
        intx::uint256 amount_in_b;
        intx::uint256 amount_out_a;
        intx::uint256 amount_out_b;
        intx::uint256 profit_b;
    };

    // function executeBackrun(
    //     address buyPool,
    //     address sellPool,
    //     uint256 amountInB,
    //     uint256 minAmountOutA,
    //     uint256 minAmountOutB,
    //     uint256 minProfitB
    // )
    struct ExecuteBackrun
    {
        using abi_tag = solabi::tuple_t<solabi::address_t,
                                        solabi::address_t,
                                        solabi::uint_t<256>,
                                        solabi::uint_t<256>,
                                        solabi::uint_t<256>,
                                        solabi::uint_t<256>>;

        inline static const bytes kSelector{0x21, 0x2c, 0x22, 0xbe};

        bytes buy_pool;
        bytes sell_pool;
        intx::uint256 amount_in_b;
        intx::uint256 min_amount_out_a;
        intx::uint256 min_amount_out_b;
        intx::uint256 min_profit_b;
    };
};

} // namespace evm
