#pragma once

#include "evm/transaction.h"

#include <boost/json.hpp>

#include <cstdint>
#include <optional>
#include <string>

namespace builder
{

struct PendingTransaction final : public evm::Transaction
{
    std::string mempool_tx_id;
    uint64_t seq_num{0};
    std::string status;
    std::string submitted_at;

    static std::optional<PendingTransaction> from_json(const boost::json::value& value);
};

} // namespace builder
