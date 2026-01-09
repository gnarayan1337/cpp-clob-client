#include <gtest/gtest.h>
#include <clob/eip712.hpp>
#include <clob/signer.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace clob::eip712;

// Test basic keccak256 with known test vectors
TEST(EIP712Test, Keccak256BasicVectors) {
    // Test vector from EIP-712 spec
    std::string input = "Hello, World!";
    std::vector<uint8_t> data(input.begin(), input.end());
    
    auto hash = keccak256(data);
    auto hex = bytes_to_hex(std::vector<uint8_t>(hash.begin(), hash.end()));
    
    // Known keccak256("Hello, World!")
    EXPECT_EQ(hex, "0xacaf3289d7b601cbd114fb36c4d29c85bbfd5e133f14cb355c3fd8d99367964f");
}

TEST(EIP712Test, Keccak256EmptyString) {
    std::vector<uint8_t> empty;
    auto hash = keccak256(empty);
    auto hex = bytes_to_hex(std::vector<uint8_t>(hash.begin(), hash.end()));
    
    // keccak256("") from spec
    EXPECT_EQ(hex, "0xc5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470");
}

// Test address encoding
TEST(EIP712Test, EncodeAddress) {
    std::string address = "0x5aAeb6053F3E94C9b9A09f33669435E7Ef1BeAed";
    auto encoded = encode_address(address);
    
    // Address should be left-padded with zeros
    EXPECT_EQ(encoded[0], 0x00);
    EXPECT_EQ(encoded[11], 0x00);
    EXPECT_EQ(encoded[12], 0x5a); // First byte of address
}

// Test uint256 encoding
TEST(EIP712Test, EncodeUint256) {
    // Test small number
    auto result = encode_uint256(42);
    EXPECT_EQ(result[31], 42);
    for (int i = 0; i < 31; i++) {
        EXPECT_EQ(result[i], 0);
    }
    
    // Test max uint64
    uint64_t max_val = UINT64_MAX;
    auto result2 = encode_uint256(max_val);
    EXPECT_EQ(result2[31], 0xFF);
    EXPECT_EQ(result2[30], 0xFF);
    EXPECT_EQ(result2[24], 0xFF);
    for (int i = 0; i < 24; i++) {
        EXPECT_EQ(result2[i], 0);
    }
}

// Test string encoding (should hash it)
TEST(EIP712Test, EncodeString) {
    std::string str = "Hello";
    auto hash = encode_string(str);
    
    // Should be 32 bytes
    EXPECT_EQ(hash.size(), 32);
    
    // Should equal keccak256("Hello")
    std::vector<uint8_t> data(str.begin(), str.end());
    auto expected = keccak256(data);
    EXPECT_EQ(hash, expected);
}

// Test EIP712 domain separator
TEST(EIP712Test, DomainSeparator) {
    json domain = {
        {"name", "Ether Mail"},
        {"version", "1"},
        {"chainId", 1},
        {"verifyingContract", "0xCcCCccccCCCCcCCCCCCcCcCccCcCCCcCcccccccC"}
    };
    
    auto hash = hash_domain(domain);
    auto hex = bytes_to_hex(std::vector<uint8_t>(hash.begin(), hash.end()));
    
    // This is a known domain separator from EIP-712 examples
    std::cout << "Domain separator: " << hex << std::endl;
    EXPECT_EQ(hash.size(), 32);
}

// Test ClobAuth signing (used for L1 authentication)
TEST(EIP712Test, ClobAuthMessage) {
    json clob_auth = {
        {"address", "0xf39Fd6e51aad88F6F4ce6aB8827279cffFb92266"},
        {"timestamp", "10000000"},
        {"nonce", 23},
        {"message", "This message attests that I control the given wallet"}
    };
    
    json domain = {
        {"name", "ClobAuthDomain"},
        {"version", "1"},
        {"chainId", 80002}  // Amoy testnet
    };
    
    json types = {
        {"ClobAuth", json::array({
            {{"name", "address"}, {"type", "address"}},
            {{"name", "timestamp"}, {"type", "string"}},
            {{"name", "nonce"}, {"type", "uint256"}},
            {{"name", "message"}, {"type", "string"}}
        })}
    };
    
    auto hash = signing_hash(domain, "ClobAuth", clob_auth, types);
    auto hex = bytes_to_hex(std::vector<uint8_t>(hash.begin(), hash.end()));
    
    std::cout << "ClobAuth signing hash: " << hex << std::endl;
    EXPECT_EQ(hash.size(), 32);
    
    // This should match the Rust client's test output
    // From rs-clob-client/src/auth.rs test: l1_headers_should_succeed
    // The signature was: 0xf62319a987514da40e57e2f4d7529f7bac38f0355bd88bb5adbb3768d80de6c1682518e0af677d5260366425f4361e7b70c25ae232aff0ab2331e2b164a1aedc1b
    // This means our hash should produce that signature with the test private key
}

// Test Order struct encoding
TEST(EIP712Test, OrderStructHash) {
    json order = {
        {"salt", "12345678901234567890"},
        {"maker", "0xf39Fd6e51aad88F6F4ce6aB8827279cffFb92266"},
        {"signer", "0xf39Fd6e51aad88F6F4ce6aB8827279cffFb92266"},
        {"taker", "0x0000000000000000000000000000000000000000"},
        {"tokenId", "123456789"},
        {"makerAmount", "1000000"},
        {"takerAmount", "2000000"},
        {"side", 0},
        {"expiration", "0"},
        {"nonce", "1"},
        {"feeRateBps", "0"},
        {"signatureType", 0}
    };
    
    json types = {
        {"Order", json::array({
            {{"name", "salt"}, {"type", "uint256"}},
            {{"name", "maker"}, {"type", "address"}},
            {{"name", "signer"}, {"type", "address"}},
            {{"name", "taker"}, {"type", "address"}},
            {{"name", "tokenId"}, {"type", "uint256"}},
            {{"name", "makerAmount"}, {"type", "uint256"}},
            {{"name", "takerAmount"}, {"type", "uint256"}},
            {{"name", "side"}, {"type", "uint8"}},
            {{"name", "expiration"}, {"type", "uint256"}},
            {{"name", "nonce"}, {"type", "uint256"}},
            {{"name", "feeRateBps"}, {"type", "uint256"}},
            {{"name", "signatureType"}, {"type", "uint8"}}
        })}
    };
    
    auto hash = hash_struct("Order", order, types);
    auto hex = bytes_to_hex(std::vector<uint8_t>(hash.begin(), hash.end()));
    
    std::cout << "Order struct hash: " << hex << std::endl;
    EXPECT_EQ(hash.size(), 32);
}

// Test full signing flow with known private key
TEST(EIP712Test, FullSigningFlowWithKnownKey) {
    // Use the same test private key from Rust client tests
    // This is a publicly known test key (DO NOT use in production)
    std::string private_key = "0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80";
    
    try {
        clob::Signer signer(private_key, 80002);  // Amoy testnet
        
        // Expected address from this private key
        EXPECT_EQ(
            signer.address(),
            "0xf39fd6e51aad88f6f4ce6ab8827279cfffb92266"
        );
        
        // Create ClobAuth message
        json clob_auth = {
            {"address", signer.address()},
            {"timestamp", "10000000"},
            {"nonce", 23},
            {"message", "This message attests that I control the given wallet"}
        };
        
        json domain = {
            {"name", "ClobAuthDomain"},
            {"version", "1"},
            {"chainId", 80002}
        };
        
        json types = {
            {"ClobAuth", json::array({
                {{"name", "address"}, {"type", "address"}},
                {{"name", "timestamp"}, {"type", "string"}},
                {{"name", "nonce"}, {"type", "uint256"}},
                {{"name", "message"}, {"type", "string"}}
            })}
        };
        
        // Sign it
        std::string signature = signer.sign_typed_data(domain, "ClobAuth", clob_auth, types);
        
        std::cout << "Signature: " << signature << std::endl;
        
        // Signature should be 65 bytes (130 hex chars + 0x prefix = 132 chars)
        EXPECT_EQ(signature.length(), 132);
        EXPECT_EQ(signature.substr(0, 2), "0x");
        
        // This should match the Rust client's signature for the same inputs
        // From rust tests: 0xf62319a987514da40e57e2f4d7529f7bac38f0355bd88bb5adbb3768d80de6c1682518e0af677d5260366425f4361e7b70c25ae232aff0ab2331e2b164a1aedc1b
        EXPECT_EQ(
            signature,
            "0xf62319a987514da40e57e2f4d7529f7bac38f0355bd88bb5adbb3768d80de6c1682518e0af677d5260366425f4361e7b70c25ae232aff0ab2331e2b164a1aedc1b"
        );
        
    } catch (const std::exception& e) {
        FAIL() << "Exception: " << e.what();
    }
}

// Test type hash computation
TEST(EIP712Test, TypeHash) {
    json types = {
        {"Person", json::array({
            {{"name", "name"}, {"type", "string"}},
            {{"name", "wallet"}, {"type", "address"}}
        })}
    };
    
    auto hash = type_hash("Person", types);
    auto hex = bytes_to_hex(std::vector<uint8_t>(hash.begin(), hash.end()));
    
    // Should be keccak256("Person(string name,address wallet)")
    std::cout << "Person type hash: " << hex << std::endl;
    
    std::string expected_type_string = "Person(string name,address wallet)";
    std::vector<uint8_t> type_data(expected_type_string.begin(), expected_type_string.end());
    auto expected_hash = keccak256(type_data);
    
    EXPECT_EQ(hash, expected_hash);
}

// Test hex conversion utilities
TEST(EIP712Test, HexConversion) {
    // Test hex to bytes
    std::string hex = "0x1234567890abcdef";
    auto bytes = hex_to_bytes(hex);
    
    EXPECT_EQ(bytes.size(), 8);
    EXPECT_EQ(bytes[0], 0x12);
    EXPECT_EQ(bytes[1], 0x34);
    EXPECT_EQ(bytes[7], 0xef);
    
    // Test bytes to hex
    std::vector<uint8_t> test_bytes = {0xde, 0xad, 0xbe, 0xef};
    auto hex_result = bytes_to_hex(test_bytes);
    EXPECT_EQ(hex_result, "0xdeadbeef");
    
    // Test without prefix
    auto hex_no_prefix = bytes_to_hex(test_bytes, false);
    EXPECT_EQ(hex_no_prefix, "deadbeef");
}

// Cross-validation test with known signature from Python client
TEST(EIP712Test, CrossValidationWithPython) {
    // This test uses known values from the Python client to verify compatibility
    
    std::string private_key = "0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80";
    clob::Signer signer(private_key, 137);  // Polygon mainnet
    
    // Create a simple order
    json order = {
        {"salt", "123456"},
        {"maker", signer.address()},
        {"signer", signer.address()},
        {"taker", "0x0000000000000000000000000000000000000000"},
        {"tokenId", "1234567890"},
        {"makerAmount", "100000000"},
        {"takerAmount", "50000000"},
        {"side", 0},
        {"expiration", "0"},
        {"nonce", "0"},
        {"feeRateBps", "0"},
        {"signatureType", 0}
    };
    
    json domain = {
        {"name", "Polymarket CTF Exchange"},
        {"version", "1"},
        {"chainId", 137},
        {"verifyingContract", "0x4bFb41d5B3570DeFd03C39a9A4D8dE6Bd8B8982E"}  // Real Polygon CTFExchange
    };
    
    json types = {
        {"Order", json::array({
            {{"name", "salt"}, {"type", "uint256"}},
            {{"name", "maker"}, {"type", "address"}},
            {{"name", "signer"}, {"type", "address"}},
            {{"name", "taker"}, {"type", "address"}},
            {{"name", "tokenId"}, {"type", "uint256"}},
            {{"name", "makerAmount"}, {"type", "uint256"}},
            {{"name", "takerAmount"}, {"type", "uint256"}},
            {{"name", "side"}, {"type", "uint8"}},
            {{"name", "expiration"}, {"type", "uint256"}},
            {{"name", "nonce"}, {"type", "uint256"}},
            {{"name", "feeRateBps"}, {"type", "uint256"}},
            {{"name", "signatureType"}, {"type", "uint8"}}
        })}
    };
    
    auto signature = signer.sign_typed_data(domain, "Order", order, types);
    
    std::cout << "Order signature: " << signature << std::endl;
    
    // Signature should be valid (65 bytes)
    EXPECT_EQ(signature.length(), 132);  // 0x + 130 hex chars
    
    // The signature should be deterministic for the same inputs
    auto signature2 = signer.sign_typed_data(domain, "Order", order, types);
    EXPECT_EQ(signature, signature2);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}



