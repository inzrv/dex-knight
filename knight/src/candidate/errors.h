#pragma once

#include <string_view>

namespace candidate
{

enum class Error
{
    REQUEST_ERROR,
    TIMEOUT,
    CLOSED
};

inline std::string_view error_to_string(Error error) noexcept
{
    switch (error) {
    case Error::REQUEST_ERROR:
        return "REQUEST_ERROR";
    case Error::TIMEOUT:
        return "TIMEOUT";
    case Error::CLOSED:
        return "CLOSED";
    }

    return "UNKNOWN_CANDIDATE_ERROR";
}

} // namespace candidate
