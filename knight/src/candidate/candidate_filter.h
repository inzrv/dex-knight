#pragma once

#include "builder/pending_snapshot.h"
#include "builder/pending_transaction.h"
#include "candidate/candidate.h"
#include "evm/contracts/pool.h"

#include <optional>
#include <vector>

namespace candidate
{

class CandidateFilter final
{
public:
    explicit CandidateFilter(std::vector<evm::Pool> pools);

    [[nodiscard]] std::optional<Candidate> filter(builder::PendingTransaction tx) const;
    [[nodiscard]] CandidateSnapshot filter_snapshot(builder::PendingSnapshot snapshot) const;

private:
    [[nodiscard]] const evm::Pool* find_pool(const bytes& address) const;

private:
    std::vector<evm::Pool> m_pools;
};

} // namespace candidate
