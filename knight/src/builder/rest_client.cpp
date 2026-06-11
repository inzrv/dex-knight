#include "builder/rest_client.h"

#include "common/log.h"
#include "utils/utils.h"

#include <boost/json/serialize.hpp>

#include <string>
#include <thread>
#include <utility>

namespace builder
{

RestClient::RestClient(Config config, net::io_context& io_ctx)
    : m_config(std::move(config))
{
    const auto& endpoint = m_config.builder_rest_endpoint;
    m_rest_client = std::make_unique<network::RestClient>(
        io_ctx, endpoint.use_tls, m_config.tls_verify_peer, endpoint.host, endpoint.port);
}

std::expected<std::string, Error> RestClient::request_snapshot() const
{
    return get_with_retry(kPendingSnapshotTarget, "pending snapshot");
}

std::expected<std::string, Error> RestClient::request_chain_head() const
{
    return get_with_retry(kChainHeadTarget, "chain head");
}

std::expected<std::string, Error> RestClient::request_chain_call(
    const boost::json::object& payload) const
{
    const auto body = boost::json::serialize(payload);
    return post_with_retry(kChainCallTarget, body, "chain call");
}

std::expected<uint64_t, Error> RestClient::request_nonce(const bytes& address) const
{
    const std::string target = std::string{kChainNonceTarget} + "/" + hex_data(address);
    const auto response = get_with_retry(target, "chain nonce");
    if (!response) {
        return std::unexpected(response.error());
    }

    const auto json = parse_to_json_object(*response);
    if (!json) {
        return std::unexpected(Error::INVALID_RESPONSE);
    }

    const auto nonce = json_hex_uint64(*json, "nonce");
    if (!nonce) {
        return std::unexpected(Error::INVALID_RESPONSE);
    }

    return *nonce;
}

std::expected<std::string, Error> RestClient::simulate_bundle(const Bundle& bundle) const
{
    const auto body = boost::json::serialize(bundle.to_json());
    return post_with_retry(kBundleSimulationTarget, body, "bundle simulation");
}

std::expected<std::string, Error> RestClient::get_with_retry(std::string_view target,
                                                             std::string_view label) const
{
    auto last_error = network::RestError::UNKNOWN_ERROR;

    for (int attempt = 1; attempt <= kMaxAttempts; ++attempt) {
        log::debug("BuilderRestClient",
                   "requesting {} {} (attempt {}/{})",
                   label,
                   target,
                   attempt,
                   kMaxAttempts);

        const auto res = m_rest_client->get(target);
        if (res) {
            log::debug("BuilderRestClient", "{} request succeeded", label);
            return *res;
        }

        last_error = res.error();
        if (attempt == kMaxAttempts) {
            break;
        }

        const auto backoff = kBaseBackoff * (1 << (attempt - 1));
        log::warn("BuilderRestClient",
                  "{} request failed: {}, retrying in {} ms",
                  label,
                  network::error_to_string(res.error()),
                  backoff.count());
        std::this_thread::sleep_for(backoff);
    }

    log::error("BuilderRestClient",
               "{} request failed after {} attempts: {}",
               label,
               kMaxAttempts,
               network::error_to_string(last_error));
    return std::unexpected(Error::REQUEST_ERROR);
}

std::expected<std::string, Error> RestClient::post_with_retry(std::string_view target,
                                                              std::string_view body,
                                                              std::string_view label) const
{
    auto last_error = network::RestError::UNKNOWN_ERROR;

    for (int attempt = 1; attempt <= kMaxAttempts; ++attempt) {
        log::debug("BuilderRestClient",
                   "posting {} {} (attempt {}/{})",
                   label,
                   target,
                   attempt,
                   kMaxAttempts);

        const auto res = m_rest_client->post(target, body);
        if (res) {
            log::debug("BuilderRestClient", "{} request succeeded", label);
            return *res;
        }

        if (res.error() == network::RestError::CONFLICT) {
            return std::unexpected(Error::CANDIDATE_NOT_PENDING);
        }

        last_error = res.error();
        if (attempt == kMaxAttempts) {
            break;
        }

        const auto backoff = kBaseBackoff * (1 << (attempt - 1));
        log::warn("BuilderRestClient",
                  "{} request failed: {}, retrying in {} ms",
                  label,
                  network::error_to_string(res.error()),
                  backoff.count());
        std::this_thread::sleep_for(backoff);
    }

    log::error("BuilderRestClient",
               "{} request failed after {} attempts: {}",
               label,
               kMaxAttempts,
               network::error_to_string(last_error));
    return std::unexpected(Error::REQUEST_ERROR);
}

} // namespace builder
