#include "encoder/encoder.h"

#include "test_helpers.h"
#include "utils/utils.h"

#include <gtest/gtest.h>
#include <solabi/utils.h>

#include <string>
#include <string_view>

namespace
{

std::string address_word(std::string_view address)
{
    constexpr std::string_view kHexPrefix{"0x"};
    std::string word;
    word.assign((kWordSize - kAddressLength) * 2, '0');
    word += address.substr(kHexPrefix.size());
    return word;
}

} // namespace

TEST(EncoderTest, EncodesExecuteBackrun)
{
    const evm::SandboxBackrun::ExecuteBackrun call{
        .buy_pool = solabi::from_hex(test::kPool1Address),
        .sell_pool = solabi::from_hex(test::kPool2Address),
        .amount_in_b = intx::uint256{1},
        .min_amount_out_a = intx::uint256{2},
        .min_amount_out_b = intx::uint256{3},
        .min_profit_b = intx::uint256{4},
    };

    auto encoded = encoder::encode_execute_backrun(call);
    ASSERT_TRUE(encoded);

    EXPECT_EQ(hex_data(*encoded),
              std::string{"0x212c22be"} + address_word(test::kPool1Address) +
                  address_word(test::kPool2Address) +
                  "0000000000000000000000000000000000000000000000000000000000000001"
                  "0000000000000000000000000000000000000000000000000000000000000002"
                  "0000000000000000000000000000000000000000000000000000000000000003"
                  "0000000000000000000000000000000000000000000000000000000000000004");
}

TEST(EncoderTest, RejectsMalformedExecuteBackrunAddress)
{
    const evm::SandboxBackrun::ExecuteBackrun call{
        .buy_pool = solabi::from_hex(test::kMalformedAddress),
        .sell_pool = solabi::from_hex(test::kPool2Address),
        .amount_in_b = intx::uint256{1},
        .min_amount_out_a = intx::uint256{2},
        .min_amount_out_b = intx::uint256{3},
        .min_profit_b = intx::uint256{4},
    };

    EXPECT_FALSE(encoder::encode_execute_backrun(call));
}
