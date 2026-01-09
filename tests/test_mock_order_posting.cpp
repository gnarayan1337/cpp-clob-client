#include <gtest/gtest.h>
#include <clob/client.hpp>
#include <clob/signer.hpp>
#include <clob/order_builder.hpp>
#include <httplib.h>
#include <thread>
#include <atomic>
#include <nlohmann/json.hpp>

using namespace clob;
using json = nlohmann::json;

const std::string TEST_PRIVATE_KEY = "0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80";
const std::string TEST_API_KEY = "12345678-1234-1234-1234-123456789012";
const std::string TEST_SECRET = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
const std::string TEST_PASSPHRASE = "test-passphrase";

// Mock server for order posting
class OrderMockServer {
public:
    OrderMockServer() : port_(18081), running_(false) {}
    
    void start() {
        running_ = true;
        
        server_thread_ = std::thread([this]() {
            httplib::Server svr;
            
            // Mock /time
            svr.Get("/time", [](const httplib::Request&, httplib::Response& res) {
                res.set_content("1700000000", "application/json");
                res.status = 200;
            });
            
            // Mock /tick-size
            svr.Get("/tick-size", [](const httplib::Request&, httplib::Response& res) {
                json response = {{"minimum_tick_size", 0.01}};
                res.set_content(response.dump(), "application/json");
                res.status = 200;
            });
            
            // Mock /neg-risk
            svr.Get("/neg-risk", [](const httplib::Request&, httplib::Response& res) {
                json response = {{"neg_risk", false}};
                res.set_content(response.dump(), "application/json");
                res.status = 200;
            });
            
            // Mock /fee-rate
            svr.Get("/fee-rate", [](const httplib::Request&, httplib::Response& res) {
                json response = {{"base_fee", 0}};
                res.set_content(response.dump(), "application/json");
                res.status = 200;
            });
            
            // Mock POST /order (singular) - for single order posting
            svr.Post("/order", [this](const httplib::Request& req, httplib::Response& res) {
                // Capture the request body for inspection
                this->last_request_body = req.body;
                
                // Parse the request
                json request_json = json::parse(req.body);
                
                // Verify it's an object (not an array)
                if (!request_json.is_object()) {
                    res.status = 400;
                    res.set_content("{\"error\": \"Expected object for /order endpoint\"}", "application/json");
                    return;
                }
                
                // Verify required fields
                if (!request_json.contains("order") || 
                    !request_json.contains("orderType") || 
                    !request_json.contains("owner")) {
                    res.status = 400;
                    res.set_content("{\"error\": \"Missing fields\"}", "application/json");
                    return;
                }
                
                // Verify owner is set
                std::string owner = request_json["owner"].get<std::string>();
                if (owner.empty()) {
                    res.status = 400;
                    res.set_content("{\"error\": \"Owner field is empty!\"}", "application/json");
                    return;
                }
                
                // Return success response (single object, not array)
                json response = {
                    {"success", true},
                    {"orderID", "test-order-12345"},
                    {"status", "LIVE"},
                    {"making_amount", "5000000"},
                    {"taking_amount", "10000000"},
                    {"transaction_hashes", json::array()},
                    {"trade_ids", json::array()}
                };
                
                res.set_content(response.dump(), "application/json");
                res.status = 200;
            });
            
            // Mock POST /orders (plural) - for multiple order posting
            svr.Post("/orders", [this](const httplib::Request& req, httplib::Response& res) {
                // Capture the request body for inspection
                this->last_request_body = req.body;
                
                // Parse the request
                json request_json = json::parse(req.body);
                
                // Verify it's an array
                if (!request_json.is_array()) {
                    res.status = 400;
                    res.set_content("{\"error\": \"Expected array for /orders endpoint\"}", "application/json");
                    return;
                }
                
                // Verify first order structure
                if (request_json.empty()) {
                    res.status = 400;
                    res.set_content("{\"error\": \"Empty array\"}", "application/json");
                    return;
                }
                
                auto first_order = request_json[0];
                
                // Verify required fields
                if (!first_order.contains("order") || 
                    !first_order.contains("orderType") || 
                    !first_order.contains("owner")) {
                    res.status = 400;
                    res.set_content("{\"error\": \"Missing fields\"}", "application/json");
                    return;
                }
                
                // Verify owner is set
                std::string owner = first_order["owner"].get<std::string>();
                if (owner.empty()) {
                    res.status = 400;
                    res.set_content("{\"error\": \"Owner field is empty!\"}", "application/json");
                    return;
                }
                
                // Return success response (array for /orders endpoint)
                json response = json::array();
                for (size_t i = 0; i < request_json.size(); i++) {
                response.push_back({
                    {"success", true},
                        {"orderID", "test-order-" + std::to_string(i + 1)},
                    {"status", "LIVE"},
                    {"making_amount", "5000000"},
                    {"taking_amount", "10000000"},
                    {"transaction_hashes", json::array()},
                    {"trade_ids", json::array()}
                });
                }
                
                res.set_content(response.dump(), "application/json");
                res.status = 200;
            });
            
            svr.listen("127.0.0.1", port_);
        });
        
        // Wait for server to start
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    void stop() {
        running_ = false;
        if (server_thread_.joinable()) {
            server_thread_.detach();
        }
    }
    
    std::string url() const {
        return "http://127.0.0.1:" + std::to_string(port_);
    }
    
    std::string get_last_request() const {
        return last_request_body;
    }
    
    ~OrderMockServer() {
        stop();
    }
    
private:
    int port_;
    std::atomic<bool> running_;
    std::thread server_thread_;
    std::string last_request_body;
};

TEST(MockOrderPostingTest, PostOrderShouldSerializeCorrectly) {
    OrderMockServer server;
    server.start();
    
    auto signer = std::make_shared<Signer>(TEST_PRIVATE_KEY, POLYGON);
    ApiCreds creds;
    creds.api_key = TEST_API_KEY;
    creds.api_secret = TEST_SECRET;
    creds.api_passphrase = TEST_PASSPHRASE;
    
    ClobClient client(server.url(), signer, creds);
    
    // Create an order
    OrderArgs args;
    args.token_id = "123456789";
    args.price = 0.50;
    args.size = 10.0;
    args.side = Side::BUY;
    args.nonce = 12345;
    args.expiration = 1700000000;
    
    CreateOrderOptions options;
    options.tick_size = "0.01";
    options.neg_risk = false;
    
    auto signed_order = client.create_order(args, options);
    
    // Post the order (now returns single response, not vector)
    auto response = client.post_order(signed_order, OrderType::GTC);
    
    // Verify response was parsed correctly
    EXPECT_TRUE(response.success);
    EXPECT_EQ(response.order_id, "test-order-12345");
    EXPECT_EQ(response.status, OrderStatusType::LIVE);
    
    // Verify request format
    std::string request_body = server.get_last_request();
    EXPECT_FALSE(request_body.empty());
    
    json request_json = json::parse(request_body);
    
    // Now expects an object (not array) for /order endpoint
    ASSERT_TRUE(request_json.is_object());
    
    // Critical checks - these MUST be correct for API to accept
    EXPECT_TRUE(request_json.contains("order"));
    EXPECT_TRUE(request_json.contains("orderType"));
    EXPECT_TRUE(request_json.contains("owner"));
    
    // CRITICAL: Owner must be set
    std::string owner = request_json["owner"].get<std::string>();
    EXPECT_EQ(owner, TEST_API_KEY) << "Owner field must be set to API key!";
    
    // Verify order structure
    auto order_obj = request_json["order"];
    EXPECT_TRUE(order_obj.contains("signature"));
    EXPECT_TRUE(order_obj.contains("side"));
    
    // Verify side is string
    std::string side = order_obj["side"].get<std::string>();
    EXPECT_EQ(side, "BUY");
}

TEST(MockOrderPostingTest, MultipleOrdersShouldPost) {
    OrderMockServer server;
    server.start();
    
    auto signer = std::make_shared<Signer>(TEST_PRIVATE_KEY, POLYGON);
    ApiCreds creds;
    creds.api_key = TEST_API_KEY;
    creds.api_secret = TEST_SECRET;
    creds.api_passphrase = TEST_PASSPHRASE;
    
    ClobClient client(server.url(), signer, creds);
    
    // Create two orders
    OrderArgs args1;
    args1.token_id = "123456789";
    args1.price = 0.50;
    args1.size = 10.0;
    args1.side = Side::BUY;
    args1.nonce = 12345;
    args1.expiration = 1700000000;
    
    OrderArgs args2;
    args2.token_id = "987654321";
    args2.price = 0.40;
    args2.size = 20.0;
    args2.side = Side::SELL;
    args2.nonce = 12346;
    args2.expiration = 1700000000;
    
    CreateOrderOptions options;
    options.tick_size = "0.01";
    options.neg_risk = false;
    
    auto order1 = client.create_order(args1, options);
    auto order2 = client.create_order(args2, options);
    
    // Post multiple orders (uses /orders endpoint - array)
    std::vector<std::pair<SignedOrder, OrderType>> orders = {
        {order1, OrderType::GTC},
        {order2, OrderType::GTD}
    };
    
    auto responses = client.post_orders(orders);
    
    // Verify responses
    ASSERT_EQ(responses.size(), 2);
    EXPECT_TRUE(responses[0].success);
    EXPECT_TRUE(responses[1].success);
    EXPECT_EQ(responses[0].order_id, "test-order-1");
    EXPECT_EQ(responses[1].order_id, "test-order-2");
    
    // Verify request format
    std::string request_body = server.get_last_request();
    json request_json = json::parse(request_body);
    
    ASSERT_TRUE(request_json.is_array());
    ASSERT_EQ(request_json.size(), 2);
    
    // Both orders should have correct structure
    EXPECT_EQ(request_json[0]["orderType"].get<std::string>(), "GTC");
    EXPECT_EQ(request_json[1]["orderType"].get<std::string>(), "GTD");
    
    // Both should have owner set
    EXPECT_EQ(request_json[0]["owner"].get<std::string>(), TEST_API_KEY);
    EXPECT_EQ(request_json[1]["owner"].get<std::string>(), TEST_API_KEY);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
