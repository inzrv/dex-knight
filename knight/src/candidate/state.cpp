#include "candidate/state.h"

#include <utility>

namespace candidate
{

bool State::apply_snapshot(CandidateSnapshot snapshot, std::vector<PoolSnapshot> pools)
{
    const auto block_number = snapshot.block_number;

    {
        std::lock_guard lock{m_mutex};
        if (m_block_number && block_number < *m_block_number) {
            return false;
        }

        if (!m_local_mempool.apply_snapshot(std::move(snapshot))) {
            return false;
        }

        m_block_number = block_number;
        m_pools = std::move(pools);
        m_valid = true;
    }

    return true;
}

bool State::apply_pending_tx(Candidate candidate)
{
    {
        std::lock_guard lock{m_mutex};
        if (!m_valid) {
            return false;
        }

        return m_local_mempool.apply_pending_tx(std::move(candidate));
    }
}

bool State::mark_invalid(uint64_t block_number)
{
    {
        std::lock_guard lock{m_mutex};
        if (m_block_number && block_number < *m_block_number) {
            return false;
        }

        m_block_number = block_number;
        m_valid = false;
        m_pools.clear();
        m_local_mempool.clear();
    }

    return true;
}

std::optional<StateSnapshot> State::pop_next_snapshot()
{
    std::lock_guard lock{m_mutex};
    if (!m_valid) {
        return std::nullopt;
    }

    auto candidate = m_local_mempool.pop_next_candidate();
    if (!candidate) {
        return std::nullopt;
    }

    return make_snapshot_locked(std::move(candidate));
}

StateSnapshot State::snapshot() const
{
    std::lock_guard lock{m_mutex};
    return make_snapshot_locked(std::nullopt);
}

bool State::valid() const
{
    std::lock_guard lock{m_mutex};
    return m_valid;
}

size_t State::size() const
{
    std::lock_guard lock{m_mutex};
    return m_local_mempool.size();
}

uint64_t State::snapshot_seq() const
{
    std::lock_guard lock{m_mutex};
    return m_local_mempool.snapshot_seq();
}

std::optional<uint64_t> State::block_number() const
{
    std::lock_guard lock{m_mutex};
    return m_block_number;
}

StateSnapshot State::make_snapshot_locked(std::optional<Candidate> candidate) const
{
    return StateSnapshot{
        .valid = m_valid,
        .block_number = m_block_number,
        .pools = m_pools,
        .candidate = std::move(candidate),
    };
}

} // namespace candidate
