#pragma once

#include "builder/pending_snapshot.h"
#include "builder/pending_transaction.h"
#include "evm/contracts/pool.h"

#include <cstdint>
#include <string_view>
#include <vector>

namespace candidate
{

enum class SwapKind
{
    A_FOR_B,
    B_FOR_A
};

inline std::string_view swap_kind_to_string(SwapKind kind) noexcept
{
    switch (kind) {
        case SwapKind::A_FOR_B: return "A_FOR_B";
        case SwapKind::B_FOR_A: return "B_FOR_A";
    }

    return "UNKNOWN_SWAP_KIND";
}

struct Candidate final
{
    builder::PendingTransaction tx;
    evm::Pool pool;
    SwapKind swap_kind;
};

struct CandidateSnapshot final
{
    uint64_t block_number{0};
    uint64_t snapshot_seq{0};
    std::vector<Candidate> candidates;
};

} // namespace candidate
