#include "runtime/runtime.h"

#include "arbitrage/calculator.h"
#include "backrun/errors.h"
#include "builder/bundle.h"
#include "builder/errors.h"
#include "candidate/candidate.h"
#include "candidate/errors.h"
#include "candidate/state.h"
#include "common/log.h"
#include "decoder/decoder.h"
#include "utils/utils.h"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <variant>

namespace runtime
{
namespace
{

constexpr uint64_t kTokenUnit{1'000'000'000'000'000'000ULL};
constexpr uint64_t kMaxArbitrageAmountInBTokens{1'000'000};

arbitrage::Params make_default_arbitrage_params()
{
    return arbitrage::Params{
        .max_amount_in_b = intx::uint256{kMaxArbitrageAmountInBTokens} * intx::uint256{kTokenUnit},
        .min_profit_b = intx::uint256{0},
        .min_output_bps = 0,
    };
}

} // namespace

Runtime::Runtime(RuntimeFactory& factory)
    : m_io_ctx(),
      m_work_guard(net::make_work_guard(m_io_ctx))
{
    auto components = factory.create(m_io_ctx);
    m_candidate_source = std::move(components.candidate_source);
    m_simulator = std::move(components.simulator);
    m_backrun_tx_composer = std::move(components.backrun_tx_composer);

    if (!m_candidate_source || !m_simulator || !m_backrun_tx_composer) {
        throw std::invalid_argument("runtime factory returned incomplete components");
    }

    auto initialized = m_backrun_tx_composer->initialize();
    if (!initialized) {
        throw std::runtime_error("failed to initialize backrun tx composer: " +
                                 std::string(backrun::error_to_string(initialized.error())));
    }

    log::info("Runtime", "Runtime initialized with all components");
}

Runtime::~Runtime()
{
    stop();
}

void Runtime::run()
{
    log::info("Runtime", "starting...");
    m_running = true;

    m_io_thread = std::thread([this]() {
        m_io_ctx.run();
    });

    m_candidate_source->start();
    run_core_loop();
}

void Runtime::stop()
{
    if (!m_running.exchange(false)) {
        return;
    }

    log::info("Runtime", "stopping...");
    m_candidate_source->stop();
    m_work_guard.reset();
    m_io_ctx.stop();

    if (m_io_thread.joinable()) {
        m_io_thread.join();
    }
}

void Runtime::run_core_loop()
{
    while (m_running) {
        auto state = m_candidate_source->wait_pop_state();
        if (!state) {
            if (state.error() == candidate::Error::CLOSED) {
                break;
            }

            log::warn("Runtime",
                      "failed to receive state snapshot: {}",
                      candidate::error_to_string(state.error()));
            continue;
        }

        const auto block_number_str =
            state->block_number ? std::to_string(*state->block_number) : std::string{"none"};
        if (!state->valid) {
            log::warn("Runtime", "state snapshot invalid: block_number={}", block_number_str);
            continue;
        }

        if (!state->candidate) {
            log::debug(
                "Runtime", "state snapshot has no candidate: block_number={}", block_number_str);
            continue;
        }

        const auto& current_candidate = *state->candidate;
        log::info("Runtime",
                  "candidate state received: block_number={} pools={} mempool_tx_id={} seq_num={} "
                  "swap_kind={}",
                  block_number_str,
                  state->pools.size(),
                  current_candidate.tx.mempool_tx_id,
                  current_candidate.tx.seq_num,
                  candidate::swap_kind_to_string(current_candidate.swap_kind));

        auto swap = decoder::decode_swap(current_candidate);
        if (!swap) {
            log::warn("Runtime",
                      "candidate swap decode failed: mempool_tx_id={}",
                      current_candidate.tx.mempool_tx_id);
            continue;
        }

        std::visit(
            [](const auto& s) {
                log::info("Runtime",
                          "candidate swap: amount_in={} min_amount_out={}",
                          hex_quantity(s.amount_in),
                          hex_quantity(s.min_amount_out));
            },
            *swap);

        const auto arbitrage_params = make_default_arbitrage_params();
        auto opportunity =
            arbitrage::find_arbitrage(*state, current_candidate, *swap, arbitrage_params);
        if (!opportunity) {
            log::debug("Runtime",
                       "arbitrage opportunity not found: block_number={} mempool_tx_id={}",
                       block_number_str,
                       current_candidate.tx.mempool_tx_id);
            continue;
        }

        log::info("Runtime",
                  "arbitrage opportunity found: buy_pool={} sell_pool={} amount_in_b={} "
                  "min_amount_out_a={} min_amount_out_b={} min_profit_b={} "
                  "expected_amount_out_a={} expected_amount_out_b={} expected_profit_b={}",
                  hex_data(opportunity->buy_pool.address),
                  hex_data(opportunity->sell_pool.address),
                  hex_quantity(opportunity->amount_in_b),
                  hex_quantity(opportunity->min_amount_out_a),
                  hex_quantity(opportunity->min_amount_out_b),
                  hex_quantity(opportunity->min_profit_b),
                  hex_quantity(opportunity->expected_amount_out_a),
                  hex_quantity(opportunity->expected_amount_out_b),
                  hex_quantity(opportunity->expected_profit_b));

        auto backrun_tx = m_backrun_tx_composer->compose(*opportunity);
        if (!backrun_tx) {
            log::warn("Runtime",
                      "backrun transaction composition failed: {}",
                      backrun::error_to_string(backrun_tx.error()));
            continue;
        }

        log::info("Runtime",
                  "backrun transaction composed: nonce={} to={} input_size={} gas={}",
                  backrun_tx->nonce,
                  backrun_tx->to ? hex_data(*backrun_tx->to) : std::string{"none"},
                  backrun_tx->input.size(),
                  backrun_tx->gas);

        builder::Bundle bundle{
            .block_number = state->block_number,
            .transactions = {
                builder::MempoolTxRef{current_candidate.tx.mempool_tx_id}, std::move(*backrun_tx)
            }
        };

        auto simulation = m_simulator->simulate(bundle);
        if (!simulation) {
            if (simulation.error() == builder::Error::CANDIDATE_NOT_PENDING) {
                log::info("Runtime",
                          "backrun bundle simulation skipped: mempool_tx_id={} already mined or "
                          "canceled",
                          current_candidate.tx.mempool_tx_id);
            } else {
                log::warn("Runtime",
                          "backrun bundle simulation failed: {}",
                          builder::error_to_string(simulation.error()));
            }
            continue;
        }

        log::info("Runtime",
                  "backrun bundle simulation result: status={} simulated={} tx_count={}",
                  builder::bundle_status_to_string(simulation->status),
                  simulation->simulated,
                  simulation->transactions.size());

        for (const auto& tx_result : simulation->transactions) {
            log::info("Runtime",
                      "backrun bundle simulation tx result: mempool_tx_id={} chain_tx_hash={} "
                      "status={}",
                      tx_result.mempool_tx_id.value_or("-"),
                      hex_data(tx_result.chain_tx_hash),
                      builder::bundle_tx_status_to_string(tx_result.status));
        }
    }

    log::info("Runtime", "core loop stopped");
}

} // namespace runtime
