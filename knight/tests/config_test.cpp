#include "common/config.h"

#include "test_helpers.h"

#include <boost/json/serialize.hpp>
#include <gtest/gtest.h>
#include <solabi/utils.h>

#include <string>
#include <string_view>

namespace
{

constexpr std::string_view kBuilderRestUrl{"https://builder.example:9443/rest/api"};
constexpr std::string_view kBuilderWsUrl{"wss://pending.example/ws/pending"};
constexpr uint64_t kChainId{31'337};
constexpr uint64_t kGas{123'456};
constexpr intx::uint256 kMaxFeePerGas{100};
constexpr intx::uint256 kMaxPriorityFeePerGas{2};

boost::json::object make_pool_json(std::string_view address)
{
    boost::json::object pool;
    pool["address"] = std::string{address};
    pool["tokenA"] = std::string{test::kTokenAAddress};
    pool["tokenB"] = std::string{test::kTokenBAddress};
    return pool;
}

boost::json::object make_full_config_json()
{
    boost::json::array pools;
    pools.emplace_back(make_pool_json(test::kPool1Address));
    pools.emplace_back(make_pool_json(test::kPool2Address));

    boost::json::object backrun;
    backrun["botAddress"] = std::string{test::kBotAddress};
    backrun["contractAddress"] = std::string{test::kBackrunAddress};
    backrun["chainId"] = kChainId;
    backrun["gas"] = kGas;
    backrun["maxFeePerGas"] = "0x64";
    backrun["maxPriorityFeePerGas"] = "0x2";

    boost::json::object json;
    json["builderRestUrl"] = std::string{kBuilderRestUrl};
    json["builderWsUrl"] = std::string{kBuilderWsUrl};
    json["tlsVerifyPeer"] = false;
    json["backrun"] = std::move(backrun);
    json["pools"] = std::move(pools);
    return json;
}

void expect_pool(const PoolConfig& pool, std::string_view address)
{
    EXPECT_EQ(pool.address, solabi::from_hex(address));
    EXPECT_EQ(pool.token_a, solabi::from_hex(test::kTokenAAddress));
    EXPECT_EQ(pool.token_b, solabi::from_hex(test::kTokenBAddress));
}

} // namespace

TEST(ConfigTest, ParsesFullConfig)
{
    Config config;
    ASSERT_TRUE(config.from_string(boost::json::serialize(make_full_config_json())));

    EXPECT_EQ(config.builder_rest_url, kBuilderRestUrl);
    EXPECT_EQ(config.builder_rest_endpoint.scheme, "https");
    EXPECT_EQ(config.builder_rest_endpoint.host, "builder.example");
    EXPECT_EQ(config.builder_rest_endpoint.port, "9443");
    EXPECT_EQ(config.builder_rest_endpoint.target, "/rest/api");
    EXPECT_TRUE(config.builder_rest_endpoint.use_tls);

    EXPECT_EQ(config.builder_ws_url, kBuilderWsUrl);
    EXPECT_EQ(config.builder_ws_endpoint.scheme, "wss");
    EXPECT_EQ(config.builder_ws_endpoint.host, "pending.example");
    EXPECT_EQ(config.builder_ws_endpoint.port, "443");
    EXPECT_EQ(config.builder_ws_endpoint.target, "/ws/pending");
    EXPECT_TRUE(config.builder_ws_endpoint.use_tls);

    EXPECT_FALSE(config.tls_verify_peer);

    ASSERT_EQ(config.pools.size(), 2);
    expect_pool(config.pools[0], test::kPool1Address);
    expect_pool(config.pools[1], test::kPool2Address);

    EXPECT_EQ(config.backrun.bot_address, solabi::from_hex(test::kBotAddress));
    EXPECT_EQ(config.backrun.contract_address, solabi::from_hex(test::kBackrunAddress));
    EXPECT_EQ(config.backrun.chain_id, kChainId);
    EXPECT_EQ(config.backrun.gas, kGas);
    EXPECT_EQ(config.backrun.max_fee_per_gas, kMaxFeePerGas);
    EXPECT_EQ(config.backrun.max_priority_fee_per_gas, kMaxPriorityFeePerGas);
}
