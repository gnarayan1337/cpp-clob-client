#include <gtest/gtest.h>
#include <clob/utilities.hpp>

using namespace clob::utils;

// Test rounding functions
TEST(UtilitiesTest, RoundDown) {
    EXPECT_DOUBLE_EQ(round_down(1.23456, 2), 1.23);
    EXPECT_DOUBLE_EQ(round_down(1.99999, 2), 1.99);
    EXPECT_DOUBLE_EQ(round_down(1.23456, 4), 1.2345);
}

TEST(UtilitiesTest, RoundUp) {
    EXPECT_DOUBLE_EQ(round_up(1.23456, 2), 1.24);
    EXPECT_DOUBLE_EQ(round_up(1.99999, 2), 2.00);
    EXPECT_DOUBLE_EQ(round_up(1.23456, 4), 1.2346);
}

TEST(UtilitiesTest, RoundNormal) {
    EXPECT_DOUBLE_EQ(round_normal(1.234, 2), 1.23);
    EXPECT_DOUBLE_EQ(round_normal(1.235, 2), 1.24);
    EXPECT_DOUBLE_EQ(round_normal(1.999, 2), 2.00);
}

// Test decimal places counting
TEST(UtilitiesTest, DecimalPlaces) {
    EXPECT_EQ(decimal_places(1.0), 0);
    EXPECT_EQ(decimal_places(1.5), 1);
    EXPECT_EQ(decimal_places(1.23), 2);
    EXPECT_EQ(decimal_places(1.234), 3);
}

// Test token decimals conversion
TEST(UtilitiesTest, ToTokenDecimals) {
    // USDC has 6 decimals
    EXPECT_EQ(to_token_decimals(1.0), 1000000);
    EXPECT_EQ(to_token_decimals(0.5), 500000);
    EXPECT_EQ(to_token_decimals(100.0), 100000000);
    EXPECT_EQ(to_token_decimals(0.000001), 1);
}

// Test price validation
TEST(UtilitiesTest, PriceValid) {
    EXPECT_TRUE(price_valid(0.5, "0.1"));
    EXPECT_TRUE(price_valid(0.1, "0.1"));
    EXPECT_TRUE(price_valid(0.9, "0.1"));
    
    EXPECT_FALSE(price_valid(0.05, "0.1"));   // Too low
    EXPECT_FALSE(price_valid(0.95, "0.1"));   // Too high
    EXPECT_FALSE(price_valid(1.0, "0.1"));    // Too high
}

// Test tick size comparison
TEST(UtilitiesTest, IsTickSizeSmaller) {
    EXPECT_TRUE(is_tick_size_smaller("0.001", "0.01"));
    EXPECT_TRUE(is_tick_size_smaller("0.01", "0.1"));
    EXPECT_FALSE(is_tick_size_smaller("0.1", "0.01"));
    EXPECT_FALSE(is_tick_size_smaller("0.01", "0.01"));
}

// Test checksum address
TEST(UtilitiesTest, ToChecksumAddress) {
    std::string lowercase = "0xf39fd6e51aad88f6f4ce6ab8827279cfffb92266";
    std::string checksum = to_checksum_address(lowercase);
    
    // Should have mixed case
    EXPECT_NE(checksum, lowercase);
    EXPECT_TRUE(checksum.find('F') != std::string::npos || 
                checksum.find('A') != std::string::npos);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}



