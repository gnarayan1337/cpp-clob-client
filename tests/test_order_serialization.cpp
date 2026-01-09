#include <gtest/gtest.h>
#include <clob/client.hpp>
#include <clob/signer.hpp>
#include <clob/order_builder.hpp>
#include <clob/utilities.hpp>
#include <nlohmann/json.hpp>

using namespace clob;
using json = nlohmann::json;

const std::string TEST_PRIVATE_KEY = "0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80";
const std::string TEST_API_KEY = "00000000-0000-0000-0000-000000000000";

// Test that order serialization matches expected format
TEST(OrderSerializationTest, SignedOrderToJsonShouldMatchExpectedFormat) {
    auto signer = std::make_shared<Signer>(TEST_PRIVATE_KEY, POLYGON);
    OrderBuilder builder(signer);
    
    OrderArgs args;
    args.token_id = "123456789";
    args.price = 0.50;
    args.size = 100.0;
    args.side = Side::BUY;
    args.nonce = 12345;
    args.expiration = 1700000000;
    
    CreateOrderOptions options;
    options.tick_size = "0.01";
    options.neg_risk = false;
    
    auto signed_order = builder.create_order(args, options);
    
    // Serialize using our function
    auto order_json = utils::order_to_json(signed_order, TEST_API_KEY, OrderType::GTC);
    
    // Verify structure
    ASSERT_TRUE(order_json.contains("order"));
    ASSERT_TRUE(order_json.contains("orderType"));
    ASSERT_TRUE(order_json.contains("owner"));
    
    // Verify owner was set correctly
    EXPECT_EQ(order_json["owner"].get<std::string>(), TEST_API_KEY);
    
    // Verify orderType
    std::string order_type_str = order_json["orderType"].get<std::string>();
    EXPECT_EQ(order_type_str, "GTC");
    
    // Verify order structure
    auto order_obj = order_json["order"];
    ASSERT_TRUE(order_obj.contains("salt"));
    ASSERT_TRUE(order_obj.contains("maker"));
    ASSERT_TRUE(order_obj.contains("signer"));
    ASSERT_TRUE(order_obj.contains("taker"));
    ASSERT_TRUE(order_obj.contains("tokenId"));
    ASSERT_TRUE(order_obj.contains("makerAmount"));
    ASSERT_TRUE(order_obj.contains("takerAmount"));
    ASSERT_TRUE(order_obj.contains("side"));
    ASSERT_TRUE(order_obj.contains("expiration"));
    ASSERT_TRUE(order_obj.contains("nonce"));
    ASSERT_TRUE(order_obj.contains("feeRateBps"));
    ASSERT_TRUE(order_obj.contains("signatureType"));
    ASSERT_TRUE(order_obj.contains("signature"));
    
    // Verify side is a string (API expects "BUY" or "SELL")
    std::string side_str = order_obj["side"].get<std::string>();
    EXPECT_EQ(side_str, "BUY");
    
    // Verify signature format
    std::string sig = order_obj["signature"].get<std::string>();
    EXPECT_EQ(sig.substr(0, 2), "0x");
    EXPECT_EQ(sig.length(), 132); // 65 bytes = 130 hex chars + "0x"
}

TEST(OrderSerializationTest, SellOrderSideShouldBeSELL) {
    auto signer = std::make_shared<Signer>(TEST_PRIVATE_KEY, POLYGON);
    OrderBuilder builder(signer);
    
    OrderArgs args;
    args.token_id = "123456789";
    args.price = 0.50;
    args.size = 100.0;
    args.side = Side::SELL;
    
    CreateOrderOptions options;
    options.tick_size = "0.01";
    options.neg_risk = false;
    
    auto signed_order = builder.create_order(args, options);
    auto order_json = utils::order_to_json(signed_order, TEST_API_KEY, OrderType::GTC);
    
    std::string side_str = order_json["order"]["side"].get<std::string>();
    EXPECT_EQ(side_str, "SELL");
}

TEST(OrderSerializationTest, OrderTypesShouldSerializeCorrectly) {
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
    
    auto signed_order = builder.create_order(args, options);
    
    // Test different order types
    auto gtc_json = utils::order_to_json(signed_order, TEST_API_KEY, OrderType::GTC);
    EXPECT_EQ(gtc_json["orderType"].get<std::string>(), "GTC");
    
    auto fok_json = utils::order_to_json(signed_order, TEST_API_KEY, OrderType::FOK);
    EXPECT_EQ(fok_json["orderType"].get<std::string>(), "FOK");
    
    auto fak_json = utils::order_to_json(signed_order, TEST_API_KEY, OrderType::FAK);
    EXPECT_EQ(fak_json["orderType"].get<std::string>(), "FAK");
    
    auto gtd_json = utils::order_to_json(signed_order, TEST_API_KEY, OrderType::GTD);
    EXPECT_EQ(gtd_json["orderType"].get<std::string>(), "GTD");
}

TEST(OrderSerializationTest, AmountsShouldBeStrings) {
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
    
    auto signed_order = builder.create_order(args, options);
    auto order_json = utils::order_to_json(signed_order, TEST_API_KEY, OrderType::GTC);
    
    auto order_obj = order_json["order"];
    
    // API expects amounts as strings (for arbitrary precision)
    EXPECT_TRUE(order_obj["makerAmount"].is_string());
    EXPECT_TRUE(order_obj["takerAmount"].is_string());
    EXPECT_TRUE(order_obj["tokenId"].is_string());
    EXPECT_TRUE(order_obj["nonce"].is_string());
    EXPECT_TRUE(order_obj["expiration"].is_string());
    EXPECT_TRUE(order_obj["feeRateBps"].is_string());
}

TEST(OrderSerializationTest, SaltShouldBeNumber) {
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
    
    auto signed_order = builder.create_order(args, options);
    auto order_json = utils::order_to_json(signed_order, TEST_API_KEY, OrderType::GTC);
    
    // Salt should be a number (the API expects this as JSON number, not string)
    auto order_obj = order_json["order"];
    EXPECT_TRUE(order_obj["salt"].is_number());
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

