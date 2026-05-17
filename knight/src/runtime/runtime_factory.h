#pragma once

#include "candidate/candidate_source.h"
#include "common/config.h"

#include <boost/asio/io_context.hpp>

#include <memory>

namespace runtime
{

struct RuntimeComponents
{
    std::unique_ptr<candidate::CandidateSource> candidate_source;
};

class RuntimeFactory final
{
public:
    explicit RuntimeFactory(Config config);

    RuntimeComponents create(boost::asio::io_context& io_ctx);

private:
    Config m_config;
};

} // namespace runtime
