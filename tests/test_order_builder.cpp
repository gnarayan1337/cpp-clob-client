#include <gtest/gtest.h>
#include <clob/order_builder.hpp>
#include <clob/signer.hpp>
#include <clob/constants.hpp>

// Test order builder initialization
TEST(OrderBuilderTest, Initialization) {
    std::string pk = "0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80";
    auto signer = std::make_shared<clob::Signer>(pk, clob::POLYGON);
    
    EXPECT_NO_THROW({
        clob::OrderBuilder builder(signer);
    });
}

// Test order creation with valid inputs
TEST(OrderBuilderTest, CreateValidOrder) {
    std::string pk = "0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80";
    auto signer = std::make_shared<clob::Signer>(pk, clob::POLYGON);
    clob::OrderBuilder builder(signer);
    
    clob::OrderArgs args;
    args.token_id = "123456789";
    args.price = 0.50;
    args.size = 100.0;
    args.side = clob::Side::BUY;
    
    clob::CreateOrderOptions options;
    options.tick_size = "0.01";
    options.neg_risk = false;
    
    EXPECT_NO_THROW({
        auto order = builder.create_order(args, options);
        EXPECT_EQ(order.order.token_id, "123456789");
        EXPECT_EQ(order.order.side, 0);  // BUY
    });
}

// Test market price calculations
TEST(OrderBuilderTest, CalculateBuyMarketPrice) {
    std::string pk = "0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80";
    auto signer = std::make_shared<clob::Signer>(pk, clob::POLYGON);
    clob::OrderBuilder builder(signer);
    
    // Asks should be in ascending price order (best ask first)
    std::vector<clob::OrderSummary> asks = {
        {"0.50", "100"},
        {"0.51", "200"},
        {"0.52", "300"}
    };
    
    // Need $50 worth, should match at 0.50 (100 shares * 0.50 = $50)
    double price = builder.calculate_buy_market_price(asks, 50.0, clob::OrderType::FOK);
    EXPECT_NEAR(price, 0.50, 0.01);
    
    // Need $100 worth, should match at 0.51 (need to go through first level and into second)
    price = builder.calculate_buy_market_price(asks, 100.0, clob::OrderType::FOK);
    EXPECT_NEAR(price, 0.51, 0.01);
}

TEST(OrderBuilderTest, CalculateSellMarketPrice) {
    std::string pk = "0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80";
    auto signer = std::make_shared<clob::Signer>(pk, clob::POLYGON);
    clob::OrderBuilder builder(signer);
    
    // Bids should be in descending price order (best bid first)
    std::vector<clob::OrderSummary> bids = {
        {"0.50", "100"},
        {"0.49", "200"},
        {"0.48", "300"}
    };
    
    // Sell 50 shares, should match at 0.50
    double price = builder.calculate_sell_market_price(bids, 50.0, clob::OrderType::FOK);
    EXPECT_NEAR(price, 0.50, 0.01);
    
    // Sell 250 shares, should match at 0.49
    price = builder.calculate_sell_market_price(bids, 250.0, clob::OrderType::FOK);
    EXPECT_NEAR(price, 0.49, 0.01);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
