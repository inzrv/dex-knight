#pragma once

#include "builder/bundle.h"
#include "builder/errors.h"
#include "common/config.h"
#include "common/types.h"
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
    std::expected<std::string, Error> request_chain_call(const boost::json::object& payload) const;
    std::expected<uint64_t, Error> request_nonce(const bytes& address) const;
    std::expected<std::string, Error> simulate_bundle(const Bundle& bundle) const;
    std::expected<std::string, Error> submit_bundle(const Bundle& bundle) const;

private:
    std::expected<std::string, Error> get_with_retry(std::string_view target,
                                                     std::string_view label) const;
    std::expected<std::string, Error> post_with_retry(std::string_view target,
                                                      std::string_view body,
                                                      std::string_view label) const;

private:
    Config m_config;
    std::unique_ptr<network::RestClient> m_rest_client;

    static constexpr std::string_view kPendingSnapshotTarget{"/public/pending"};
    static constexpr std::string_view kChainHeadTarget{"/chain/head"};
    static constexpr std::string_view kChainCallTarget{"/chain/call"};
    static constexpr std::string_view kChainNonceTarget{"/chain/nonce"};
    static constexpr std::string_view kBundleSimulationTarget{"/private/bundle/simulate"};
    static constexpr std::string_view kBundleSubmissionTarget{"/private/bundle"};
    static constexpr int kMaxAttempts{3};
    static constexpr std::chrono::milliseconds kBaseBackoff{200};
};

} // namespace builder
