#include "network/rest_client.h"

#include "common/log.h"

#include <boost/asio/connect.hpp>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include <utility>

namespace network
{

RestClient::RestClient(net::io_context& io_ctx,
                       bool use_tls,
                       bool verify_tls_peer,
                       std::string host,
                       std::string port)
    : m_io_ctx(io_ctx)
    , m_use_tls(use_tls)
    , m_verify_tls_peer(verify_tls_peer)
    , m_host(std::move(host))
    , m_port(std::move(port))
{}

std::expected<std::string, RestError> RestClient::get(std::string_view target) const
{
    if (m_use_tls) {
        return get_tls(target);
    }

    return get_plain(target);
}

std::expected<std::string, RestError> RestClient::post(std::string_view target, std::string_view body) const
{
    if (m_use_tls) {
        return post_tls(target, body);
    }

    return post_plain(target, body);
}

std::expected<std::string, RestError> RestClient::get_plain(std::string_view target) const
{
    log::debug("RestClient", "GET http://{}:{}{}", m_host, m_port, target);
    beast::error_code ec;

    tcp::resolver resolver{net::make_strand(m_io_ctx)};
    beast::tcp_stream stream{net::make_strand(m_io_ctx)};

    const auto results = resolver.resolve(m_host, m_port, ec);
    if (ec) {
        log::error("RestClient", "resolve failed: {}", ec.message());
        return std::unexpected(RestError::RESOLVE_ERROR);
    }

    stream.connect(results, ec);
    if (ec) {
        log::error("RestClient", "connect failed: {}", ec.message());
        return std::unexpected(RestError::CONNECT_ERROR);
    }

    http::request<http::empty_body> req{http::verb::get, std::string(target), 11};
    req.set(http::field::host, m_host);
    req.set(http::field::user_agent, "knight-rest-client");

    http::write(stream, req, ec);
    if (ec) {
        log::error("RestClient", "HTTP write failed: {}", ec.message());
        return std::unexpected(RestError::HTTP_WRITE_ERROR);
    }

    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(stream, buffer, res, ec);
    if (ec) {
        log::error("RestClient", "HTTP read failed: {}", ec.message());
        return std::unexpected(RestError::HTTP_READ_ERROR);
    }

    if (res.result() != http::status::ok) {
        log::warn("RestClient", "bad status: {}", static_cast<int>(res.result()));
        return std::unexpected(RestError::BAD_STATUS);
    }

    stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    if (ec && ec != beast::errc::not_connected) {
        log::warn("RestClient", "socket shutdown failed: {}", ec.message());
    }

    log::debug("RestClient", "GET succeeded: {}", target);
    return std::move(res.body());
}

std::expected<std::string, RestError> RestClient::get_tls(std::string_view target) const
{
    log::debug("RestClient", "GET https://{}:{}{}", m_host, m_port, target);
    beast::error_code ec;

    ssl::context ssl_ctx{ssl::context::tls_client};
    ssl_ctx.set_default_verify_paths(ec);
    if (ec) {
        log::warn("RestClient", "could not load default TLS verify paths: {}", ec.message());
    }
    ssl_ctx.set_verify_mode(m_verify_tls_peer ? ssl::verify_peer : ssl::verify_none);

    tcp::resolver resolver{net::make_strand(m_io_ctx)};
    beast::ssl_stream<beast::tcp_stream> stream{net::make_strand(m_io_ctx), ssl_ctx};

    auto* native_tls = stream.native_handle();
    if (!SSL_set_tlsext_host_name(native_tls, m_host.c_str())) {
        log::error("RestClient", "TLS SNI setup failed");
        return std::unexpected(RestError::SSL_HANDSHAKE_ERROR);
    }

    if (m_verify_tls_peer && !SSL_set1_host(native_tls, m_host.c_str())) {
        log::error("RestClient", "TLS hostname verification setup failed");
        return std::unexpected(RestError::SSL_HANDSHAKE_ERROR);
    }

    const auto results = resolver.resolve(m_host, m_port, ec);
    if (ec) {
        log::error("RestClient", "resolve failed: {}", ec.message());
        return std::unexpected(RestError::RESOLVE_ERROR);
    }

    beast::get_lowest_layer(stream).connect(results, ec);
    if (ec) {
        log::error("RestClient", "connect failed: {}", ec.message());
        return std::unexpected(RestError::CONNECT_ERROR);
    }

    stream.handshake(ssl::stream_base::client, ec);
    if (ec) {
        log::error("RestClient", "SSL handshake failed: {}", ec.message());
        return std::unexpected(RestError::SSL_HANDSHAKE_ERROR);
    }

    http::request<http::empty_body> req{http::verb::get, std::string(target), 11};
    req.set(http::field::host, m_host);
    req.set(http::field::user_agent, "knight-rest-client");

    http::write(stream, req, ec);
    if (ec) {
        log::error("RestClient", "HTTP write failed: {}", ec.message());
        return std::unexpected(RestError::HTTP_WRITE_ERROR);
    }

    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(stream, buffer, res, ec);
    if (ec) {
        log::error("RestClient", "HTTP read failed: {}", ec.message());
        return std::unexpected(RestError::HTTP_READ_ERROR);
    }

    if (res.result() != http::status::ok) {
        log::warn("RestClient", "bad status: {}", static_cast<int>(res.result()));
        return std::unexpected(RestError::BAD_STATUS);
    }

    stream.shutdown(ec);
    if (ec && ec != beast::errc::not_connected) {
        log::warn("RestClient", "TLS shutdown failed: {}", ec.message());
    }

    log::debug("RestClient", "GET succeeded: {}", target);
    return std::move(res.body());
}

std::expected<std::string, RestError> RestClient::post_plain(std::string_view target, std::string_view body) const
{
    log::debug("RestClient", "POST http://{}:{}{}", m_host, m_port, target);
    beast::error_code ec;

    tcp::resolver resolver{net::make_strand(m_io_ctx)};
    beast::tcp_stream stream{net::make_strand(m_io_ctx)};

    const auto results = resolver.resolve(m_host, m_port, ec);
    if (ec) {
        log::error("RestClient", "resolve failed: {}", ec.message());
        return std::unexpected(RestError::RESOLVE_ERROR);
    }

    stream.connect(results, ec);
    if (ec) {
        log::error("RestClient", "connect failed: {}", ec.message());
        return std::unexpected(RestError::CONNECT_ERROR);
    }

    http::request<http::string_body> req{http::verb::post, std::string(target), 11};
    req.set(http::field::host, m_host);
    req.set(http::field::user_agent, "knight-rest-client");
    req.set(http::field::content_type, "application/json");
    req.body() = std::string(body);
    req.prepare_payload();

    http::write(stream, req, ec);
    if (ec) {
        log::error("RestClient", "HTTP write failed: {}", ec.message());
        return std::unexpected(RestError::HTTP_WRITE_ERROR);
    }

    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(stream, buffer, res, ec);
    if (ec) {
        log::error("RestClient", "HTTP read failed: {}", ec.message());
        return std::unexpected(RestError::HTTP_READ_ERROR);
    }

    if (res.result() != http::status::ok) {
        log::warn("RestClient", "status: {}", static_cast<int>(res.result()));
        const auto error = res.result() == http::status::conflict
            ? RestError::CONFLICT
            : RestError::BAD_STATUS;
        return std::unexpected(error);
    }

    stream.socket().shutdown(tcp::socket::shutdown_both, ec);
    if (ec && ec != beast::errc::not_connected) {
        log::warn("RestClient", "socket shutdown failed: {}", ec.message());
    }

    log::debug("RestClient", "POST succeeded: {}", target);
    return std::move(res.body());
}

std::expected<std::string, RestError> RestClient::post_tls(std::string_view target, std::string_view body) const
{
    log::debug("RestClient", "POST https://{}:{}{}", m_host, m_port, target);
    beast::error_code ec;

    ssl::context ssl_ctx{ssl::context::tls_client};
    ssl_ctx.set_default_verify_paths(ec);
    if (ec) {
        log::warn("RestClient", "could not load default TLS verify paths: {}", ec.message());
    }
    ssl_ctx.set_verify_mode(m_verify_tls_peer ? ssl::verify_peer : ssl::verify_none);

    tcp::resolver resolver{net::make_strand(m_io_ctx)};
    beast::ssl_stream<beast::tcp_stream> stream{net::make_strand(m_io_ctx), ssl_ctx};

    auto* native_tls = stream.native_handle();
    if (!SSL_set_tlsext_host_name(native_tls, m_host.c_str())) {
        log::error("RestClient", "TLS SNI setup failed");
        return std::unexpected(RestError::SSL_HANDSHAKE_ERROR);
    }

    if (m_verify_tls_peer && !SSL_set1_host(native_tls, m_host.c_str())) {
        log::error("RestClient", "TLS hostname verification setup failed");
        return std::unexpected(RestError::SSL_HANDSHAKE_ERROR);
    }

    const auto results = resolver.resolve(m_host, m_port, ec);
    if (ec) {
        log::error("RestClient", "resolve failed: {}", ec.message());
        return std::unexpected(RestError::RESOLVE_ERROR);
    }

    beast::get_lowest_layer(stream).connect(results, ec);
    if (ec) {
        log::error("RestClient", "connect failed: {}", ec.message());
        return std::unexpected(RestError::CONNECT_ERROR);
    }

    stream.handshake(ssl::stream_base::client, ec);
    if (ec) {
        log::error("RestClient", "SSL handshake failed: {}", ec.message());
        return std::unexpected(RestError::SSL_HANDSHAKE_ERROR);
    }

    http::request<http::string_body> req{http::verb::post, std::string(target), 11};
    req.set(http::field::host, m_host);
    req.set(http::field::user_agent, "knight-rest-client");
    req.set(http::field::content_type, "application/json");
    req.body() = std::string(body);
    req.prepare_payload();

    http::write(stream, req, ec);
    if (ec) {
        log::error("RestClient", "HTTP write failed: {}", ec.message());
        return std::unexpected(RestError::HTTP_WRITE_ERROR);
    }

    beast::flat_buffer buffer;
    http::response<http::string_body> res;
    http::read(stream, buffer, res, ec);
    if (ec) {
        log::error("RestClient", "HTTP read failed: {}", ec.message());
        return std::unexpected(RestError::HTTP_READ_ERROR);
    }

    if (res.result() != http::status::ok) {
        log::warn("RestClient", "status: {}", static_cast<int>(res.result()));
        const auto error = res.result() == http::status::conflict
            ? RestError::CONFLICT
            : RestError::BAD_STATUS;
        return std::unexpected(error);
    }

    stream.shutdown(ec);
    if (ec && ec != beast::errc::not_connected) {
        log::warn("RestClient", "TLS shutdown failed: {}", ec.message());
    }

    log::debug("RestClient", "POST succeeded: {}", target);
    return std::move(res.body());
}

} // namespace network
