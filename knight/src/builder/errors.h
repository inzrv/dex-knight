#pragma once

#include <string_view>

namespace builder
{

enum class Error
{
    REQUEST_ERROR,
    TIMEOUT,
    INVALID_RESPONSE,
    CANDIDATE_NOT_PENDING
};

inline std::string_view error_to_string(Error error) noexcept
{
    switch (error) {
        case Error::REQUEST_ERROR: return "REQUEST_ERROR";
        case Error::TIMEOUT: return "TIMEOUT";
        case Error::INVALID_RESPONSE: return "INVALID_RESPONSE";
        case Error::CANDIDATE_NOT_PENDING: return "CANDIDATE_NOT_PENDING";
    }

    return "UNKNOWN_BUILDER_ERROR";
}

} // namespace builder
