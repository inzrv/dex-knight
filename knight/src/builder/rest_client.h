#pragma once

#include "common/config.h"
#include "builder/errors.h"
#include "network/rest_client.h"

#include <chrono>
#include <expected>
#include <memory>
#include <string>
#include <string_view>

namespace builder
{

class RestClient final
{
public:
    RestClient(Config config, net::io_context& io_ctx);

    std::expected<std::string, Error> request_snapshot() const;
    std::expected<std::string, Error> request_chain_head() const;

private:
    std::expected<std::string, Error> get_with_retry(std::string_view target, std::string_view label) const;

private:
    Config m_config;
    std::unique_ptr<network::RestClient> m_rest_client;

    static constexpr std::string_view kPendingSnapshotTarget{"/public/pending"};
    static constexpr std::string_view kChainHeadTarget{"/chain/head"};
    static constexpr int kMaxAttempts{3};
    static constexpr std::chrono::milliseconds kBaseBackoff{200};
};

} // namespace builder
