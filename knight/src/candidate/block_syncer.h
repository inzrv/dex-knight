#pragma once

#include "candidate/errors.h"
#include "builder/rest_client.h"
#include "common/config.h"
#include "common/worker.h"

#include <boost/asio.hpp>

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <expected>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string_view>

namespace net = boost::asio;

namespace candidate
{

class BlockSyncer final : public Worker
{
public:
    enum class State
    {
        STOPPED,
        STARTING,
        READY,
        FAILED
    };

    using new_block_handler_t = std::function<void(uint64_t)>;

    BlockSyncer(Config config, net::io_context& io_ctx, new_block_handler_t on_new_block);
    ~BlockSyncer() override;

    BlockSyncer(const BlockSyncer&) = delete;
    BlockSyncer& operator=(const BlockSyncer&) = delete;

    void start();
    void stop();
    std::expected<void, Error> wait_until_ready(std::chrono::milliseconds timeout);
    [[nodiscard]] State state() const;

private:
    void run() override;
    void run_core_loop();
    std::optional<uint64_t> fetch_last_block() const;
    bool publish_new_block(uint64_t block_number);
    void set_state(State state);

    static std::optional<uint64_t> parse_last_block(std::string_view body);

private:
    std::optional<uint64_t> m_current_block;
    std::unique_ptr<builder::RestClient> m_builder_rest_client;
    new_block_handler_t m_on_new_block;

    mutable std::mutex m_state_mutex;
    std::condition_variable m_state_cv;
    State m_state{State::STOPPED};

    static constexpr std::chrono::milliseconds kPollInterval{200};
};

} // namespace candidate
