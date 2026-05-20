#pragma once

#include "utils/utils.h"

#include <boost/json.hpp>

#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

struct Endpoint final
{
    std::string scheme;
    std::string host;
    std::string port;
    std::string target;
    bool use_tls{false};
};

struct Config final
{
    bool from_json(const boost::json::object& json)
    {
        const auto builder_rest_url_json = json_string(json, "builderRestUrl");
        const auto builder_ws_url_json = json_string(json, "builderWsUrl");
        if (!builder_rest_url_json || !builder_ws_url_json) {
            return false;
        }

        const auto rest_endpoint = parse_rest_endpoint(*builder_rest_url_json);
        const auto ws_endpoint = parse_ws_endpoint(*builder_ws_url_json);
        if (!rest_endpoint || !ws_endpoint) {
            return false;
        }

        builder_rest_url = *builder_rest_url_json;
        builder_rest_endpoint = *rest_endpoint;
        builder_ws_url = *builder_ws_url_json;
        builder_ws_endpoint = *ws_endpoint;
        tls_verify_peer = json_bool(json, "tlsVerifyPeer").value_or(true);
        return true;
    }

    bool from_string(std::string_view s)
    {
        const auto parse_to_json_res = parse_to_json_object(s);
        if (!parse_to_json_res) {
            return false;
        }

        return from_json(*parse_to_json_res);
    }

    std::string builder_rest_url;
    Endpoint builder_rest_endpoint;
    std::string builder_ws_url;
    Endpoint builder_ws_endpoint;
    bool tls_verify_peer{true};

private:
    struct SchemeInfo final
    {
        std::string scheme;
        std::string default_port;
        bool use_tls{false};
    };

    struct UrlParts final
    {
        std::string_view authority;
        std::string target;
    };

    struct HostPort final
    {
        std::string host;
        std::string port;
    };

    static bool is_valid_port(std::string_view port)
    {
        if (port.empty()) {
            return false;
        }

        uint16_t value = 0;
        const auto* begin = port.data();
        const auto* end = port.data() + port.size();
        const auto [ptr, ec] = std::from_chars(begin, end, value);
        return ec == std::errc{} && ptr == end && value > 0;
    }

    static std::optional<SchemeInfo> parse_ws_scheme(std::string_view scheme_raw)
    {
        auto scheme = to_lower(std::string(scheme_raw));
        if (scheme == "ws") {
            return SchemeInfo{
                .scheme = std::move(scheme),
                .default_port = "80",
                .use_tls = false,
            };
        }

        if (scheme == "wss") {
            return SchemeInfo{
                .scheme = std::move(scheme),
                .default_port = "443",
                .use_tls = true,
            };
        }

        return std::nullopt;
    }

    static std::optional<SchemeInfo> parse_rest_scheme(std::string_view scheme_raw)
    {
        auto scheme = to_lower(std::string(scheme_raw));
        if (scheme == "http") {
            return SchemeInfo{
                .scheme = std::move(scheme),
                .default_port = "80",
                .use_tls = false,
            };
        }

        if (scheme == "https") {
            return SchemeInfo{
                .scheme = std::move(scheme),
                .default_port = "443",
                .use_tls = true,
            };
        }

        return std::nullopt;
    }

    static std::optional<UrlParts> parse_url_parts(std::string_view url, size_t authority_start)
    {
        const auto target_start = url.find('/', authority_start);
        const auto authority = target_start == std::string_view::npos
            ? url.substr(authority_start)
            : url.substr(authority_start, target_start - authority_start);
        auto target = target_start == std::string_view::npos
            ? std::string{"/"}
            : std::string{url.substr(target_start)};

        if (authority.empty() || target.empty() || authority.find('@') != std::string_view::npos) {
            return std::nullopt;
        }

        return UrlParts{
            .authority = authority,
            .target = std::move(target),
        };
    }

    static std::optional<HostPort> parse_host_port(std::string_view authority, std::string_view default_port)
    {
        const auto colon = authority.find(':');
        if (colon != std::string_view::npos && authority.find(':', colon + 1) != std::string_view::npos) {
            return std::nullopt;
        }

        auto host = colon == std::string_view::npos
            ? std::string(authority)
            : std::string(authority.substr(0, colon));
        auto port = colon == std::string_view::npos
            ? std::string(default_port)
            : std::string(authority.substr(colon + 1));

        if (host.empty() || !is_valid_port(port)) {
            return std::nullopt;
        }

        return HostPort{
            .host = std::move(host),
            .port = std::move(port),
        };
    }

    static std::optional<Endpoint> parse_endpoint(std::string_view url, const SchemeInfo& scheme)
    {
        const auto scheme_end = url.find("://");
        if (scheme_end == std::string_view::npos || to_lower(std::string(url.substr(0, scheme_end))) != scheme.scheme) {
            return std::nullopt;
        }

        auto parts = parse_url_parts(url, scheme_end + 3);
        if (!parts) {
            return std::nullopt;
        }

        auto host_port = parse_host_port(parts->authority, scheme.default_port);
        if (!host_port) {
            return std::nullopt;
        }

        return Endpoint{
            .scheme = scheme.scheme,
            .host = std::move(host_port->host),
            .port = std::move(host_port->port),
            .target = std::move(parts->target),
            .use_tls = scheme.use_tls,
        };
    }

    static std::optional<Endpoint> parse_ws_endpoint(std::string_view url)
    {
        const auto scheme_end = url.find("://");
        if (scheme_end == std::string_view::npos) {
            return std::nullopt;
        }

        auto scheme = parse_ws_scheme(url.substr(0, scheme_end));
        if (!scheme) {
            return std::nullopt;
        }

        return parse_endpoint(url, *scheme);
    }

    static std::optional<Endpoint> parse_rest_endpoint(std::string_view url)
    {
        const auto scheme_end = url.find("://");
        if (scheme_end == std::string_view::npos) {
            return std::nullopt;
        }

        auto scheme = parse_rest_scheme(url.substr(0, scheme_end));
        if (!scheme) {
            return std::nullopt;
        }

        return parse_endpoint(url, *scheme);
    }
};
