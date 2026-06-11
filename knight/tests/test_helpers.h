#pragma once

#include "arbitrage/params.h"
#include "builder/pending_transaction.h"
#include "candidate/candidate.h"
#include "candidate/state.h"
#include "evm/contracts/sandbox_dex.h"

#include <solabi/utils.h>

#include <cstdint>
#include <string_view>
#include <utility>

namespace test
{

constexpr std::string_view kPool1Address = "0x1111111111111111111111111111111111111111";
constexpr std::string_view kPool2Address = "0x2222222222222222222222222222222222222222";
constexpr std::string_view kTokenAAddress = "0xaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
constexpr std::string_view kTokenBAddress = "0xbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb";
constexpr std::string_view kSenderAddress = "0xf0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0f0";

constexpr intx::uint256 kTokenUnit{1'000'000'000'000'000'000ULL};

inline intx::uint256 tokens(intx::uint256 value)
{
    return value * kTokenUnit;
}

inline evm::Pool make_pool(std::string_view address_hex = kPool1Address)
{
    return evm::Pool{
        solabi::from_hex(address_hex),
        solabi::from_hex(kTokenAAddress),
        solabi::from_hex(kTokenBAddress),
    };
}

inline candidate::Candidate make_candidate(const evm::Pool& pool,
                                           candidate::SwapKind swap_kind,
                                           bytes input = {})
{
    builder::PendingTransaction tx;
    tx.mempool_tx_id = "mp-test";
    tx.seq_num = 1;
    tx.status = "pending";
    tx.input = std::move(input);
    tx.from = solabi::from_hex(kSenderAddress);
    tx.to = pool.address;

    return candidate::Candidate{
        .tx = std::move(tx),
        .pool = pool,
        .swap_kind = swap_kind,
    };
}

inline candidate::StateSnapshot make_state(const evm::Pool& victim_pool,
                                           const evm::Pool& other_pool,
                                           const candidate::Candidate& candidate)
{
    return candidate::StateSnapshot{
        .valid = true,
        .block_number = 1,
        .pools =
            {
                {.pool = victim_pool, .reserve_a = tokens(1'000), .reserve_b = tokens(1'000)},
                {.pool = other_pool, .reserve_a = tokens(1'000), .reserve_b = tokens(1'000)},
            },
        .candidate = candidate,
    };
}

inline arbitrage::Params permissive_params()
{
    return arbitrage::Params{
        .max_amount_in_b = tokens(1'000'000),
        .min_profit_b = intx::uint256{0},
        .min_output_bps = 0,
    };
}

} // namespace test
