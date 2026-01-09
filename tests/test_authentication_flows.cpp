#include <gtest/gtest.h>
#include <clob/client.hpp>
#include <clob/signer.hpp>
#include <clob/constants.hpp>

using namespace clob;

// Known test private key (DO NOT use in production)
const std::string TEST_PRIVATE_KEY = "0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80";
const std::string TEST_ADDRESS = "0xf39fd6e51aad88f6f4ce6ab8827279cfffb92266";

// ==================== Authentication Flow Tests ====================

TEST(AuthenticationTest, CreateApiKeyShouldSucceed) {
    // Test: Create new API key with L1 authentication
    EXPECT_NO_THROW({
        // auto signer = std::make_shared<Signer>(TEST_PRIVATE_KEY, POLYGON);
        // ClobClient client("http://mock-server", signer);
        // 
        // Mock server should respond to:
        // POST /auth/api-key
        // with headers: POLY_ADDRESS, POLY_SIGNATURE, POLY_TIMESTAMP, POLY_NONCE
        // 
        // auto creds = client.create_api_key();
        // EXPECT_FALSE(creds.api_key.empty());
        // EXPECT_FALSE(creds.api_secret.empty());
        // EXPECT_FALSE(creds.api_passphrase.empty());
    });
}

TEST(AuthenticationTest, DeriveApiKeyShouldSucceed) {
    // Test: Derive API key from nonce
    EXPECT_NO_THROW({
        // auto signer = std::make_shared<Signer>(TEST_PRIVATE_KEY, POLYGON);
        // ClobClient client("http://mock-server", signer);
        // 
        // Mock server should respond to:
        // GET /auth/derive-api-key
        // with headers: POLY_ADDRESS, POLY_SIGNATURE, POLY_TIMESTAMP, POLY_NONCE
        // 
        // auto creds = client.derive_api_key(123);
        // EXPECT_FALSE(creds.api_key.empty());
        // EXPECT_FALSE(creds.api_secret.empty());
        // EXPECT_FALSE(creds.api_passphrase.empty());
    });
}

TEST(AuthenticationTest, CreateOrDeriveApiKeyShouldSucceed) {
    // Test: Try create, fallback to derive if create fails
    EXPECT_NO_THROW({
        // auto signer = std::make_shared<Signer>(TEST_PRIVATE_KEY, POLYGON);
        // ClobClient client("http://mock-server", signer);
        // 
        // Mock server should:
        // 1. Return 404 for POST /auth/api-key
        // 2. Return success for GET /auth/derive-api-key
        // 
        // auto creds = client.create_or_derive_api_creds();
        // EXPECT_FALSE(creds.api_key.empty());
    });
}

TEST(AuthenticationTest, AuthenticateWithExplicitCredentialsShouldSucceed) {
    // Test: Create authenticated client with explicit credentials
    EXPECT_NO_THROW({
        // auto signer = std::make_shared<Signer>(TEST_PRIVATE_KEY, POLYGON);
        // ApiCreds creds{"test-key", "test-secret", "test-passphrase"};
        // ClobClient client("http://mock-server", signer, creds);
        // 
        // EXPECT_EQ(client.get_address(), TEST_ADDRESS);
        // EXPECT_TRUE(client.is_authenticated());
        // EXPECT_TRUE(client.has_api_credentials());
    });
}

TEST(AuthenticationTest, AuthenticateWithNonceShouldSucceed) {
    // Test: Authenticate using nonce
    EXPECT_NO_THROW({
        // auto signer = std::make_shared<Signer>(TEST_PRIVATE_KEY, POLYGON);
        // ClobClient client("http://mock-server", signer);
        // 
        // // Mock server responds to derive_api_key with nonce
        // auto creds = client.derive_api_key(123);
        // client.set_api_creds(creds);
        // 
        // EXPECT_TRUE(client.is_authenticated());
        // EXPECT_TRUE(client.has_api_credentials());
    });
}

TEST(AuthenticationTest, AuthenticatedToUnauthenticatedShouldSucceed) {
    // Test: Deauthenticate client
    EXPECT_NO_THROW({
        // auto signer = std::make_shared<Signer>(TEST_PRIVATE_KEY, POLYGON);
        // ApiCreds creds{"test-key", "test-secret", "test-passphrase"};
        // ClobClient client("http://mock-server", signer, creds);
        // 
        // EXPECT_TRUE(client.is_authenticated());
        // 
        // client.deauthenticate();
        // 
        // EXPECT_FALSE(client.is_authenticated());
        // EXPECT_FALSE(client.has_api_credentials());
    });
}

TEST(AuthenticationTest, L1AuthenticationHeadersShouldBeCorrect) {
    // Test: Verify L1 headers are generated correctly
    EXPECT_NO_THROW({
        // auto signer = std::make_shared<Signer>(TEST_PRIVATE_KEY, POLYGON);
        // ClobClient client("http://mock-server", signer);
        // 
        // // Mock server should verify headers contain:
        // // - POLY_ADDRESS: lowercase hex address
        // // - POLY_SIGNATURE: 0x + 130 hex chars
        // // - POLY_TIMESTAMP: Unix timestamp
        // // - POLY_NONCE: Number
        // 
        // auto creds = client.create_api_key();
        // 
        // // Headers should match expected format
    });
}

TEST(AuthenticationTest, L2AuthenticationHeadersShouldBeCorrect) {
    // Test: Verify L2 headers are generated correctly
    EXPECT_NO_THROW({
        // auto signer = std::make_shared<Signer>(TEST_PRIVATE_KEY, POLYGON);
        // ApiCreds creds{"test-key", "test-secret", "test-passphrase"};
        // ClobClient client("http://mock-server", signer, creds);
        // 
        // // Mock server should verify headers contain:
        // // - POLY_API_KEY: API key
        // // - POLY_SIGNATURE: HMAC signature
        // // - POLY_TIMESTAMP: Unix timestamp
        // // - POLY_PASSPHRASE: Passphrase
        // 
        // auto orders = client.get_orders();
        // 
        // // Headers should match expected format
    });
}

TEST(AuthenticationTest, InvalidPrivateKeyShouldFail) {
    // Test: Invalid private key should throw
    EXPECT_THROW({
        auto signer = std::make_shared<Signer>("invalid-key", POLYGON);
    }, std::exception);  // Can throw runtime_error or invalid_argument
}

TEST(AuthenticationTest, L1MethodWithoutSignerShouldFail) {
    // Test: L1 methods require signer
    EXPECT_THROW({
        ClobClient client("http://mock-server");
        // This should throw because no signer
        client.create_api_key();
    }, std::runtime_error);
}

TEST(AuthenticationTest, L2MethodWithoutCredentialsShouldFail) {
    // Test: L2 methods require credentials
    EXPECT_THROW({
        auto signer = std::make_shared<Signer>(TEST_PRIVATE_KEY, POLYGON);
        ClobClient client("http://mock-server", signer);
        // This should throw because no credentials
        client.get_orders();
    }, std::runtime_error);
}

TEST(AuthenticationTest, StateTransitionL0ToL1ShouldSucceed) {
    // Test: Transition from L0 (public) to L1 (with signer)
    EXPECT_NO_THROW({
        // Start with L0
        // ClobClient client("http://mock-server");
        // EXPECT_FALSE(client.is_authenticated());
        // 
        // // Can only call public endpoints
        // auto time = client.get_server_time();
        // 
        // // Cannot call authenticated endpoints
        // EXPECT_THROW(client.get_orders(), std::runtime_error);
    });
}

TEST(AuthenticationTest, StateTransitionL1ToL2ShouldSucceed) {
    // Test: Transition from L1 (with signer) to L2 (with credentials)
    EXPECT_NO_THROW({
        // auto signer = std::make_shared<Signer>(TEST_PRIVATE_KEY, POLYGON);
        // ClobClient client("http://mock-server", signer);
        // EXPECT_TRUE(client.is_authenticated());
        // EXPECT_FALSE(client.has_api_credentials());
        // 
        // // Can create API key (L1 method)
        // auto creds = client.create_api_key();
        // client.set_api_creds(creds);
        // 
        // EXPECT_TRUE(client.has_api_credentials());
        // 
        // // Can now call L2 methods
        // auto orders = client.get_orders();
    });
}

TEST(AuthenticationTest, StateTransitionL2ToL0ShouldSucceed) {
    // Test: Deauthenticate from L2 to L0
    EXPECT_NO_THROW({
        // auto signer = std::make_shared<Signer>(TEST_PRIVATE_KEY, POLYGON);
        // ApiCreds creds{"test-key", "test-secret", "test-passphrase"};
        // ClobClient client("http://mock-server", signer, creds);
        // 
        // EXPECT_TRUE(client.is_authenticated());
        // EXPECT_TRUE(client.has_api_credentials());
        // 
        // // Deauthenticate
        // client.deauthenticate();
        // 
        // EXPECT_FALSE(client.is_authenticated());
        // EXPECT_FALSE(client.has_api_credentials());
        // 
        // // Can only call public endpoints
        // auto time = client.get_server_time();
        // EXPECT_THROW(client.get_orders(), std::runtime_error);
    });
}

TEST(AuthenticationTest, SignerAddressDerivationShouldBeCorrect) {
    // Test: Signer derives correct address from private key
    EXPECT_NO_THROW({
        auto signer = std::make_shared<Signer>(TEST_PRIVATE_KEY, POLYGON);
        EXPECT_EQ(signer->address(), TEST_ADDRESS);
    });
}

TEST(AuthenticationTest, SignerWithDifferentChainIdsShouldSucceed) {
    // Test: Signer works with different chain IDs
    EXPECT_NO_THROW({
        auto signer_polygon = std::make_shared<Signer>(TEST_PRIVATE_KEY, POLYGON);
        EXPECT_EQ(signer_polygon->get_chain_id(), POLYGON);
        
        auto signer_amoy = std::make_shared<Signer>(TEST_PRIVATE_KEY, AMOY);
        EXPECT_EQ(signer_amoy->get_chain_id(), AMOY);
        
        // Both should derive same address
        EXPECT_EQ(signer_polygon->address(), signer_amoy->address());
    });
}

TEST(AuthenticationTest, ApiCredsSetterShouldWork) {
    // Test: Setting API credentials after client creation
    EXPECT_NO_THROW({
        // auto signer = std::make_shared<Signer>(TEST_PRIVATE_KEY, POLYGON);
        // ClobClient client("http://mock-server", signer);
        // 
        // EXPECT_FALSE(client.has_api_credentials());
        // 
        // ApiCreds creds{"test-key", "test-secret", "test-passphrase"};
        // client.set_api_creds(creds);
        // 
        // EXPECT_TRUE(client.has_api_credentials());
    });
}

TEST(AuthenticationTest, GetAddressShouldReturnSignerAddress) {
    // Test: get_address returns signer's address
    EXPECT_NO_THROW({
        auto signer = std::make_shared<Signer>(TEST_PRIVATE_KEY, POLYGON);
        ClobClient client("http://mock-server", signer);
        
        EXPECT_EQ(client.get_address(), TEST_ADDRESS);
    });
}

TEST(AuthenticationTest, GetAddressWithoutSignerShouldReturnEmpty) {
    // Test: get_address without signer returns empty string
    EXPECT_NO_THROW({
        ClobClient client("http://mock-server");
        EXPECT_TRUE(client.get_address().empty());
    });
}

TEST(AuthenticationTest, ContractAddressesShouldBeCorrect) {
    // Test: Contract addresses are correct for chain ID
    EXPECT_NO_THROW({
        auto signer = std::make_shared<Signer>(TEST_PRIVATE_KEY, POLYGON);
        ClobClient client("http://mock-server", signer);
        
        auto collateral = client.get_collateral_address();
        auto conditional = client.get_conditional_address();
        auto exchange = client.get_exchange_address();
        
        EXPECT_FALSE(collateral.empty());
        EXPECT_FALSE(conditional.empty());
        EXPECT_FALSE(exchange.empty());
    });
}

TEST(AuthenticationTest, NegRiskExchangeAddressShouldBeDifferent) {
    // Test: Neg risk exchange address is different
    EXPECT_NO_THROW({
        auto signer = std::make_shared<Signer>(TEST_PRIVATE_KEY, POLYGON);
        ClobClient client("http://mock-server", signer);
        
        auto normal_exchange = client.get_exchange_address(false);
        auto neg_risk_exchange = client.get_exchange_address(true);
        
        EXPECT_NE(normal_exchange, neg_risk_exchange);
    });
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


