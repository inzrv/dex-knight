#include "tx_composer.h"

#include "encoder/encoder.h"
#include "evm/contracts/sandbox_backrun.h"

#include <intx/intx.hpp>
#include <utility>

namespace backrun
{

TxComposer::TxComposer(Config config, BackrunConfig backrun_config, boost::asio::io_context& io_ctx)
    : m_backrun_config(std::move(backrun_config)),
      m_builder_rest_client(std::move(config), io_ctx)
{}

std::expected<void, Error> TxComposer::initialize()
{
    return refresh_nonce();
}

std::expected<void, Error> TxComposer::refresh_nonce()
{
    auto nonce = m_builder_rest_client.request_nonce(m_backrun_config.bot_address);
    if (!nonce) {
        return std::unexpected(Error::NONCE_REQUEST_FAILED);
    }

    m_next_nonce = *nonce;
    return {};
}

std::expected<evm::Transaction, Error> TxComposer::compose(
    const arbitrage::Opportunity& opportunity) const
{
    if (!m_next_nonce) {
        return std::unexpected(Error::NOT_INITIALIZED);
    }

    const evm::SandboxBackrun::ExecuteBackrun call{
        .buy_pool = opportunity.buy_pool.address,
        .sell_pool = opportunity.sell_pool.address,
        .amount_in_b = opportunity.amount_in_b,
        .min_amount_out_a = opportunity.min_amount_out_a,
        .min_amount_out_b = opportunity.min_amount_out_b,
        .min_profit_b = opportunity.min_profit_b
    };

    auto calldata = encoder::encode_execute_backrun(call);
    if (!calldata) {
        return std::unexpected(Error::CALL_ENCODING_FAILED);
    }

    evm::Transaction tx{
        .input = std::move(*calldata),
        .from = m_backrun_config.bot_address,
        .chain_id = m_backrun_config.chain_id,
        .value = intx::uint256{0},
        .gas = m_backrun_config.gas,
        .max_fee_per_gas = m_backrun_config.max_fee_per_gas,
        .max_priority_fee_per_gas = m_backrun_config.max_priority_fee_per_gas,
        .nonce = *m_next_nonce,
        .type = 2,
        .to = m_backrun_config.contract_address
    };

    return tx;
}

void TxComposer::mark_nonce_used()
{
    if (m_next_nonce) {
        ++(*m_next_nonce);
    }
}

} // namespace backrun
