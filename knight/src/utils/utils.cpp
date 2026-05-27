#include "utils.h"

#include <boost/json/src.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/system/error_code.hpp>
#include <solabi/utils.h>

#include <charconv>
#include <string>
#include <system_error>

std::optional<boost::json::value> parse_to_json(std::string_view s)
{
    boost::json::value json_value;
    try {
        json_value = boost::json::parse(s);
    } catch (const std::exception& /*e*/) {
        return std::nullopt;
    }

    return json_value;
}

std::optional<boost::json::object> parse_to_json_object(std::string_view s)
{
    boost::system::error_code ec;
    auto value = boost::json::parse(s, ec);
    if (ec || !value.is_object()) {
        return std::nullopt;
    }

    return value.as_object();
}

std::optional<std::string> json_string(const boost::json::object& object, std::string_view field)
{
    const auto* value = object.if_contains(field);
    if (!value || !value->is_string()) {
        return std::nullopt;
    }

    return std::string(value->as_string().c_str());
}

std::optional<bool> json_bool(const boost::json::object& object, std::string_view field)
{
    const auto* value = object.if_contains(field);
    if (!value || !value->is_bool()) {
        return std::nullopt;
    }

    return value->as_bool();
}

std::optional<int64_t> json_int64(const boost::json::object& object, std::string_view field)
{
    const auto* value = object.if_contains(field);
    if (!value || !value->is_int64()) {
        return std::nullopt;
    }

    return value->as_int64();
}

std::optional<uint64_t> json_uint64(const boost::json::object& object, std::string_view field)
{
    const auto value = json_int64(object, field);
    if (!value || *value < 0) {
        return std::nullopt;
    }

    return static_cast<uint64_t>(*value);
}

std::optional<uint64_t> parse_hex_quantity(std::string_view value)
{
    if (value.size() <= 2 || value[0] != '0' || (value[1] != 'x' && value[1] != 'X')) {
        return std::nullopt;
    }

    uint64_t parsed = 0;
    const auto digits = value.substr(2);
    const auto* begin = digits.data();
    const auto* end = digits.data() + digits.size();
    const auto [ptr, ec] = std::from_chars(begin, end, parsed, 16);
    if (ec != std::errc{} || ptr != end) {
        return std::nullopt;
    }

    return parsed;
}

std::optional<intx::uint256> parse_hex_uint256(std::string_view value)
{
    if (value.size() <= 2 || value[0] != '0' || (value[1] != 'x' && value[1] != 'X')) {
        return std::nullopt;
    }

    try {
        return intx::from_string<intx::uint256>(std::string{value});
    } catch (const std::exception& /*e*/) {
        return std::nullopt;
    }
}

std::optional<bytes> parse_hex_bytes(std::string_view value)
{
    if (value.size() < 2 || value[0] != '0' || (value[1] != 'x' && value[1] != 'X')) {
        return std::nullopt;
    }

    try {
        return solabi::from_hex(value);
    } catch (const std::exception& /*e*/) {
        return std::nullopt;
    }
}

std::string hex_quantity(uint64_t value)
{
    char buffer[18]{};
    auto [ptr, ec] = std::to_chars(buffer, buffer + sizeof(buffer), value, 16);
    if (ec != std::errc{}) {
        return "0x0";
    }

    return "0x" + std::string(buffer, ptr);
}

std::string hex_quantity(const intx::uint256& value)
{
    return "0x" + intx::hex(value);
}

std::string hex_data(const bytes& value)
{
    static constexpr char kHexDigits[] = "0123456789abcdef";

    std::string out;
    out.reserve(2 + value.size() * 2);
    out.append("0x");
    for (const auto byte : value) {
        out.push_back(kHexDigits[(byte >> 4) & 0x0f]);
        out.push_back(kHexDigits[byte & 0x0f]);
    }

    return out;
}

std::optional<uint64_t> json_hex_uint64(const boost::json::object& object, std::string_view field)
{
    const auto raw = json_string(object, field);
    if (!raw) {
        return std::nullopt;
    }

    return parse_hex_quantity(*raw);
}

std::optional<intx::uint256> json_hex_uint256(const boost::json::object& object, std::string_view field)
{
    const auto raw = json_string(object, field);
    if (!raw) {
        return std::nullopt;
    }

    return parse_hex_uint256(*raw);
}

std::optional<bytes> json_hex_bytes(const boost::json::object& object, std::string_view field)
{
    const auto raw = json_string(object, field);
    if (!raw) {
        return std::nullopt;
    }

    return parse_hex_bytes(*raw);
}

std::optional<bytes> json_hex_bytes(const boost::json::object& object, std::string_view field, size_t expected_size)
{
    auto parsed = json_hex_bytes(object, field);
    if (!parsed || parsed->size() != expected_size) {
        return std::nullopt;
    }

    return parsed;
}

std::optional<std::optional<uint64_t>> json_optional_hex_uint64(
    const boost::json::object& object,
    std::string_view field)
{
    if (!object.if_contains(field) || object.at(field).is_null()) {
        return std::optional<uint64_t>{};
    }

    auto parsed = json_hex_uint64(object, field);
    if (!parsed) {
        return std::nullopt;
    }

    return std::optional<uint64_t>{*parsed};
}

std::optional<std::optional<intx::uint256>> json_optional_hex_uint256(
    const boost::json::object& object,
    std::string_view field)
{
    if (!object.if_contains(field) || object.at(field).is_null()) {
        return std::optional<intx::uint256>{};
    }

    auto parsed = json_hex_uint256(object, field);
    if (!parsed) {
        return std::nullopt;
    }

    return std::optional<intx::uint256>{*parsed};
}

std::optional<std::optional<bytes>> json_optional_hex_bytes(
    const boost::json::object& object,
    std::string_view field,
    size_t expected_size)
{
    if (!object.if_contains(field) || object.at(field).is_null()) {
        return std::optional<bytes>{};
    }

    auto parsed = json_hex_bytes(object, field, expected_size);
    if (!parsed) {
        return std::nullopt;
    }

    return std::optional<bytes>{std::move(*parsed)};
}

const boost::json::array* json_array(const boost::json::object& object, std::string_view field) noexcept
{
    const auto* value = object.if_contains(field);
    if (!value || !value->is_array()) {
        return nullptr;
    }

    return &value->as_array();
}

const boost::json::object* json_object(const boost::json::object& object, std::string_view field) noexcept
{
    const auto* value = object.if_contains(field);
    if (!value || !value->is_object()) {
        return nullptr;
    }

    return &value->as_object();
}

std::string to_lower(std::string s)
{
    boost::algorithm::to_lower(s);
    return s;
}

int64_t now_unix_ms() noexcept
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}
