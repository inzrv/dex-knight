#include "runtime/runtime.h"

#include "builder/errors.h"
#include "candidate/candidate.h"
#include "candidate/errors.h"
#include "candidate/state.h"
#include "common/log.h"
#include "decoder/decoder.h"
#include "utils/utils.h"

#include <string>
#include <stdexcept>
#include <thread>
#include <utility>
#include <variant>

namespace runtime
{

Runtime::Runtime(RuntimeFactory& factory)
    : m_io_ctx()
    , m_work_guard(net::make_work_guard(m_io_ctx))
{
    auto components = factory.create(m_io_ctx);
    m_candidate_source = std::move(components.candidate_source);
    m_simulator = std::move(components.simulator);

    if (!m_candidate_source || !m_simulator) {
        throw std::invalid_argument("runtime factory returned incomplete components");
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
        auto next_state = m_candidate_source->wait_pop_next_state();
        if (!next_state) {
            if (next_state.error() == candidate::Error::CLOSED) {
                break;
            }

            log::warn("Runtime", "failed to receive state snapshot: {}", candidate::error_to_string(next_state.error()));
            continue;
        }

        const auto block_number_str = next_state->block_number ? std::to_string(*next_state->block_number) : std::string{"none"};
        if (!next_state->valid) {
            log::warn("Runtime", "state snapshot invalid: block_number={}", block_number_str);
            continue;
        }

        if (!next_state->candidate) {
            log::debug("Runtime", "state snapshot has no candidate: block_number={}", block_number_str);
            continue;
        }

        const auto& current_candidate = *next_state->candidate;
        log::info("Runtime",
                  "candidate state received: block_number={} pools={} mempool_tx_id={} seq_num={} swap_kind={}",
                  block_number_str,
                  next_state->pools.size(),
                  current_candidate.tx.mempool_tx_id,
                  current_candidate.tx.seq_num,
                  candidate::swap_kind_to_string(current_candidate.swap_kind));

        auto swap = decoder::decode_swap(current_candidate);
        if (swap) {
            std::visit([](const auto& s) {
                log::info("Runtime",
                          "candidate swap: amount_in={} min_amount_out={}",
                          hex_quantity(s.amount_in),
                          hex_quantity(s.min_amount_out));
            }, *swap);
        } else {
            log::warn("Runtime",
                      "candidate swap decode failed: mempool_tx_id={}",
                      current_candidate.tx.mempool_tx_id);
        }

        auto simulation = m_simulator->simulate(current_candidate, next_state->block_number);
        if (!simulation) {
            if (simulation.error() == builder::Error::CANDIDATE_NOT_PENDING) {
                log::info("Runtime",
                          "candidate simulation skipped: mempool_tx_id={} already mined or canceled",
                          current_candidate.tx.mempool_tx_id);
            } else {
                log::warn("Runtime", "candidate simulation failed: {}", builder::error_to_string(simulation.error()));
            }
            continue;
        }

        log::info("Runtime",
                  "candidate simulation result: status={} simulated={} tx_count={}",
                  builder::bundle_status_to_string(simulation->status),
                  simulation->simulated,
                  simulation->transactions.size());

        for (const auto& tx_result : simulation->transactions) {
            log::info("Runtime",
                      "candidate simulation tx result: mempool_tx_id={} chain_tx_hash={} status={}",
                      tx_result.mempool_tx_id.value_or("-"),
                      hex_data(tx_result.chain_tx_hash),
                      builder::bundle_tx_status_to_string(tx_result.status));
        }
    }

    log::info("Runtime", "core loop stopped");
}

} // namespace runtime
