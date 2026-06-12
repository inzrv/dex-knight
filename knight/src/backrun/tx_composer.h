#pragma once

#include "arbitrage/opportunity.h"
#include "backrun/errors.h"
#include "builder/rest_client.h"
#include "common/config.h"
#include "evm/transaction.h"

#include <boost/asio/io_context.hpp>

#include <expected>
#include <optional>

namespace backrun
{

class TxComposer final
{
public:
    TxComposer(Config config, BackrunConfig backrun_config, boost::asio::io_context& io_ctx);

    std::expected<void, Error> initialize();
    std::expected<void, Error> refresh_nonce();
    std::expected<evm::Transaction, Error> compose(const arbitrage::Opportunity& opportunity) const;

    void mark_nonce_used();

private:
    BackrunConfig m_backrun_config;
    builder::RestClient m_builder_rest_client;
    std::optional<uint64_t> m_next_nonce;
};

} // namespace backrun
