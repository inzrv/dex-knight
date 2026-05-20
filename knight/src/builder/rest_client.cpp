#include "builder/rest_client.h"

#include "common/log.h"

#include <thread>
#include <utility>

namespace builder
{

RestClient::RestClient(Config config, net::io_context& io_ctx)
    : m_config(std::move(config))
{
    const auto& endpoint = m_config.builder_rest_endpoint;
    m_rest_client = std::make_unique<network::RestClient>(
        io_ctx,
        endpoint.use_tls,
        m_config.tls_verify_peer,
        endpoint.host,
        endpoint.port);
}

std::expected<std::string, Error> RestClient::request_snapshot() const
{
    return get_with_retry(kPendingSnapshotTarget, "pending snapshot");
}

std::expected<std::string, Error> RestClient::request_chain_head() const
{
    return get_with_retry(kChainHeadTarget, "chain head");
}

std::expected<std::string, Error> RestClient::get_with_retry(std::string_view target, std::string_view label) const
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

} // namespace builder
