#include <gtest/gtest.h>
#include <clob/client.hpp>
#include <clob/signer.hpp>
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

// Simple mock HTTP server for testing
class MockServer {
public:
    MockServer() : port_(0), running_(false) {}
    
    void start() {
        // Find available port
        port_ = 18080;
        running_ = true;
        
        server_thread_ = std::thread([this]() {
            httplib::Server svr;
            
            // Mock /auth/derive-api-key
            svr.Get("/auth/derive-api-key", [](const httplib::Request& req, httplib::Response& res) {
                // Verify L1 headers present
                if (req.has_header("POLY_ADDRESS") && 
                    req.has_header("POLY_SIGNATURE") &&
                    req.has_header("POLY_TIMESTAMP") &&
                    req.has_header("POLY_NONCE")) {
                    
                    json response = {
                        {"apiKey", TEST_API_KEY},
                        {"secret", TEST_SECRET},
                        {"passphrase", TEST_PASSPHRASE}
                    };
                    res.set_content(response.dump(), "application/json");
                    res.status = 200;
                } else {
                    res.status = 401;
                    res.set_content("{\"error\": \"Unauthorized\"}", "application/json");
                }
            });
            
            // Mock /time
            svr.Get("/time", [](const httplib::Request& req, httplib::Response& res) {
                res.set_content("1700000000", "application/json");
                res.status = 200;
            });
            
            // Mock /auth/api-keys
            svr.Get("/auth/api-keys", [](const httplib::Request& req, httplib::Response& res) {
                // Verify L2 headers present
                if (req.has_header("POLY_ADDRESS") && 
                    req.has_header("POLY_API_KEY") &&
                    req.has_header("POLY_PASSPHRASE") &&
                    req.has_header("POLY_SIGNATURE") &&
                    req.has_header("POLY_TIMESTAMP")) {
                    
                    json response = {
                        {"apiKeys", json::array({TEST_API_KEY})}
                    };
                    res.set_content(response.dump(), "application/json");
                    res.status = 200;
                } else {
                    res.status = 401;
                    res.set_content("{\"error\": \"Unauthorized\"}", "application/json");
                }
            });
            
            svr.listen("127.0.0.1", port_);
        });
        
        // Wait for server to start
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    void stop() {
        running_ = false;
        if (server_thread_.joinable()) {
            // Server will stop on next request or timeout
            server_thread_.detach();
        }
    }
    
    std::string url() const {
        return "http://127.0.0.1:" + std::to_string(port_);
    }
    
    ~MockServer() {
        stop();
    }
    
private:
    int port_;
    std::atomic<bool> running_;
    std::thread server_thread_;
};

TEST(MockAuthenticationTest, L1HeadersShouldBeCreated) {
    MockServer server;
    server.start();
    
    auto signer = std::make_shared<Signer>(TEST_PRIVATE_KEY, POLYGON);
    ClobClient client(server.url(), signer);
    
    // This should create L1 headers and call /auth/derive-api-key
    EXPECT_NO_THROW({
        auto creds = client.derive_api_key();
        EXPECT_EQ(creds.api_key, TEST_API_KEY);
        EXPECT_EQ(creds.api_secret, TEST_SECRET);
        EXPECT_EQ(creds.api_passphrase, TEST_PASSPHRASE);
    });
}

TEST(MockAuthenticationTest, L2HeadersShouldBeCreated) {
    MockServer server;
    server.start();
    
    auto signer = std::make_shared<Signer>(TEST_PRIVATE_KEY, POLYGON);
    ApiCreds creds;
    creds.api_key = TEST_API_KEY;
    creds.api_secret = TEST_SECRET;
    creds.api_passphrase = TEST_PASSPHRASE;
    
    ClobClient client(server.url(), signer, creds);
    
    // This should create L2 headers and call /auth/api-keys
    EXPECT_NO_THROW({
        auto api_keys = client.get_api_keys();
        EXPECT_TRUE(api_keys.keys.has_value());
        if (api_keys.keys.has_value()) {
            EXPECT_FALSE(api_keys.keys->empty());
        }
    });
}

TEST(MockAuthenticationTest, StateTransitionsShouldWork) {
    MockServer server;
    server.start();
    
    auto signer = std::make_shared<Signer>(TEST_PRIVATE_KEY, POLYGON);
    
    // Start unauthenticated
    ClobClient client(server.url(), signer);
    EXPECT_TRUE(client.is_authenticated());
    EXPECT_FALSE(client.has_api_credentials());
    
    // Get credentials
    auto creds = client.derive_api_key();
    
    // Set credentials
    client.set_api_creds(creds);
    EXPECT_TRUE(client.is_authenticated());
    EXPECT_TRUE(client.has_api_credentials());
    
    // Deauthenticate
    client.deauthenticate();
    EXPECT_FALSE(client.is_authenticated());
    EXPECT_FALSE(client.has_api_credentials());
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

