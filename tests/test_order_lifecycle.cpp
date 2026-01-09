#include <gtest/gtest.h>
#include <clob/client.hpp>
#include <clob/signer.hpp>
#include <clob/order_builder.hpp>
#include <clob/constants.hpp>

using namespace clob;

const std::string TEST_PRIVATE_KEY = "0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80";

// ==================== Order Lifecycle Tests ====================
// Note: These are working unit tests for the order builder
// For integration tests with mock HTTP server, see test_endpoints_comprehensive.cpp

TEST(OrderLifecycleTest, CreateLimitOrderShouldSucceed) {
    auto signer = std::make_shared<Signer>(TEST_PRIVATE_KEY, POLYGON);
    OrderBuilder builder(signer);
    
    OrderArgs args;
    args.token_id = "123456789";
    args.price = 0.50;
    args.size = 100.0;
    args.side = Side::BUY;
    
    CreateOrderOptions options;
    options.tick_size = "0.01";
    options.neg_risk = false;
    
    auto order = builder.create_order(args, options);
    
    EXPECT_EQ(order.order.token_id, "123456789");
    EXPECT_EQ(order.order.side, 0); // BUY
    EXPECT_FALSE(order.signature.empty());
}

TEST(OrderLifecycleTest, OrderSignatureShouldBeValid) {
    auto signer = std::make_shared<Signer>(TEST_PRIVATE_KEY, POLYGON);
    OrderBuilder builder(signer);
    
    OrderArgs args;
    args.token_id = "123456789";
    args.price = 0.50;
    args.size = 100.0;
    args.side = Side::BUY;
    
    CreateOrderOptions options;
    options.tick_size = "0.01";
    options.neg_risk = false;
    
    auto order = builder.create_order(args, options);
    
    // Signature should be valid (65 bytes = 132 hex chars + 0x prefix)
    EXPECT_EQ(order.signature.length(), 132);
    EXPECT_EQ(order.signature.substr(0, 2), "0x");
}

TEST(MarketOrderTest, BuyMarketPriceCalculationShouldSucceed) {
    auto signer = std::make_shared<Signer>(TEST_PRIVATE_KEY, POLYGON);
    OrderBuilder builder(signer);
    
    // Orderbook with ascending ask prices
    std::vector<OrderSummary> asks = {
        {"0.50", "100"},  // 100 shares at $0.50 = $50
        {"0.51", "200"},  // 200 shares at $0.51 = $102
        {"0.52", "300"}   // 300 shares at $0.52 = $156
    };
    
    // Need $50, should match at 0.50
    double price1 = builder.calculate_buy_market_price(asks, 50.0, OrderType::FOK);
    EXPECT_NEAR(price1, 0.50, 0.01);
    
    // Need $100, should match at 0.51
    double price2 = builder.calculate_buy_market_price(asks, 100.0, OrderType::FOK);
    EXPECT_NEAR(price2, 0.51, 0.01);
}

TEST(MarketOrderTest, SellMarketPriceCalculationShouldSucceed) {
    auto signer = std::make_shared<Signer>(TEST_PRIVATE_KEY, POLYGON);
    OrderBuilder builder(signer);
    
    // Orderbook with descending bid prices
    std::vector<OrderSummary> bids = {
        {"0.50", "100"},  // Best bid
        {"0.49", "200"},
        {"0.48", "300"}
    };
    
    // Sell 50 shares, should match at 0.50
    double price1 = builder.calculate_sell_market_price(bids, 50.0, OrderType::FOK);
    EXPECT_NEAR(price1, 0.50, 0.01);
    
    // Sell 250 shares, should match at 0.49
    double price2 = builder.calculate_sell_market_price(bids, 250.0, OrderType::FOK);
    EXPECT_NEAR(price2, 0.49, 0.01);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
