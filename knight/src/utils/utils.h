#pragma once

#include "common/types.h"

#include <boost/json.hpp>

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

std::optional<boost::json::value> parse_to_json(std::string_view s);
std::optional<boost::json::object> parse_to_json_object(std::string_view s);

std::optional<std::string> json_string(const boost::json::object& object, std::string_view field);
std::optional<bool> json_bool(const boost::json::object& object, std::string_view field);
std::optional<int64_t> json_int64(const boost::json::object& object, std::string_view field);
std::optional<uint64_t> json_uint64(const boost::json::object& object, std::string_view field);
std::optional<uint64_t> parse_hex_quantity(std::string_view value);
std::optional<intx::uint256> parse_hex_uint256(std::string_view value);
std::optional<bytes> parse_hex_bytes(std::string_view value);
std::string hex_quantity(uint64_t value);
std::string hex_quantity(const intx::uint256& value);
std::string hex_data(const bytes& value);

template <unsigned N>
intx::uint<N> integer_sqrt(intx::uint<N> value)
{
    intx::uint<N> result{};
    auto bit = intx::uint<N>{1} << (intx::uint<N>::num_bits - 2);

    while (bit > value) {
        bit >>= 2;
    }

    while (bit != 0) {
        if (value >= result + bit) {
            value -= result + bit;
            result = (result >> 1) + bit;
        } else {
            result >>= 1;
        }

        bit >>= 2;
    }

    return result;
}

std::optional<uint64_t> json_hex_uint64(const boost::json::object& object, std::string_view field);
std::optional<intx::uint256> json_hex_uint256(const boost::json::object& object,
                                              std::string_view field);
std::optional<bytes> json_hex_bytes(const boost::json::object& object, std::string_view field);
std::optional<bytes> json_hex_bytes(const boost::json::object& object,
                                    std::string_view field,
                                    size_t expected_size);
std::optional<std::optional<uint64_t>> json_optional_hex_uint64(const boost::json::object& object,
                                                                std::string_view field);
std::optional<std::optional<uint64_t>> json_optional_uint64(const boost::json::object& object,
                                                            std::string_view field);
std::optional<std::optional<intx::uint256>> json_optional_hex_uint256(
    const boost::json::object& object, std::string_view field);
std::optional<std::optional<bytes>> json_optional_hex_bytes(const boost::json::object& object,
                                                            std::string_view field,
                                                            size_t expected_size);
std::optional<std::optional<std::string>> json_optional_string(const boost::json::object& object,
                                                               std::string_view field);
const boost::json::array* json_array(const boost::json::object& object,
                                     std::string_view field) noexcept;
const boost::json::object* json_object(const boost::json::object& object,
                                       std::string_view field) noexcept;

std::string to_lower(std::string s);

int64_t now_unix_ms() noexcept;
