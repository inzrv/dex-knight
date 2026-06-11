#pragma once

#include "common/types.h"

#include <solabi/types.h>
#include <solabi/utils.h>

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

        inline static const bytes kTopic{
            solabi::from_hex("0xf10a5f8a7c6a4693ccca3ed17669da824f6a42e85101b8039370bdb4a85f98ca")};

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

        inline static const bytes kSelector{solabi::from_hex("0x212c22be")};

        bytes buy_pool;
        bytes sell_pool;
        intx::uint256 amount_in_b;
        intx::uint256 min_amount_out_a;
        intx::uint256 min_amount_out_b;
        intx::uint256 min_profit_b;
    };
};

} // namespace evm
