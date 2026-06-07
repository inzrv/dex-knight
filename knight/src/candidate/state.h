#pragma once

#include "candidate/candidate.h"
#include "candidate/local_mempool.h"
#include "candidate/pool_snapshot.h"

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <vector>

namespace candidate
{

struct StateSnapshot final
{
    bool valid{false};
    std::optional<uint64_t> block_number;
    std::vector<PoolSnapshot> pools;
    std::optional<Candidate> candidate;
};

class State final
{
public:
    bool apply_snapshot(CandidateSnapshot snapshot, std::vector<PoolSnapshot> pools);
    bool apply_pending_tx(Candidate candidate);
    bool mark_invalid(uint64_t block_number);
    std::optional<StateSnapshot> pop_next_snapshot();
    [[nodiscard]] StateSnapshot snapshot() const;

    [[nodiscard]] bool valid() const;
    [[nodiscard]] size_t size() const;
    [[nodiscard]] uint64_t snapshot_seq() const;
    [[nodiscard]] std::optional<uint64_t> block_number() const;

private:
    StateSnapshot make_snapshot_locked(std::optional<Candidate> candidate) const;

private:
    // Guards block number, pool snapshots, validity, and mempool as one coherent state.
    mutable std::mutex m_mutex;
    LocalMempool m_local_mempool;
    std::vector<PoolSnapshot> m_pools;
    std::optional<uint64_t> m_block_number;
    bool m_valid{false};
};

} // namespace candidate
