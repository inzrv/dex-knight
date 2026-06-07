#pragma once

#include "candidate/candidate.h"
#include "candidate/errors.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>

namespace candidate
{

class LocalMempool final
{
public:
    bool apply_snapshot(CandidateSnapshot snapshot);
    bool apply_pending_tx(Candidate candidate);
    std::optional<Candidate> pop_next_candidate();
    void clear();

    [[nodiscard]] size_t size() const;
    [[nodiscard]] uint64_t snapshot_seq() const;

private:
    std::optional<Candidate> pop_next_candidate_locked();

private:
    mutable std::mutex m_mutex;
    std::map<uint64_t, Candidate> m_candidates;
    uint64_t m_snapshot_seq{0};
};

} // namespace candidate
