#include <gtest/gtest.h>
#include <clob/signer.hpp>

// Test signer initialization with valid private key
TEST(SignerTest, InitializeWithValidKey) {
    std::string private_key = "0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80";
    
    EXPECT_NO_THROW({
        clob::Signer signer(private_key, clob::POLYGON);
    });
}

// Test signer initialization without 0x prefix
TEST(SignerTest, InitializeWithoutPrefix) {
    std::string private_key = "ac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80";
    
    EXPECT_NO_THROW({
        clob::Signer signer(private_key, clob::POLYGON);
    });
}

// Test signer rejects invalid key length
TEST(SignerTest, RejectInvalidKeyLength) {
    std::string short_key = "0x1234";
    
    EXPECT_THROW({
        clob::Signer signer(short_key, clob::POLYGON);
    }, std::runtime_error);
}

// Test address derivation matches known address
TEST(SignerTest, AddressDerivation) {
    // Well-known test private key
    std::string private_key = "0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80";
    clob::Signer signer(private_key, clob::POLYGON);
    
    // Expected address (checksummed)
    std::string expected = "0xf39fd6e51aad88f6f4ce6ab8827279cfffb92266";
    
    EXPECT_EQ(signer.address(), expected);
}

// Test chain ID is stored correctly
TEST(SignerTest, ChainIdStorage) {
    std::string private_key = "0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80";
    
    clob::Signer signer_polygon(private_key, clob::POLYGON);
    EXPECT_EQ(signer_polygon.get_chain_id(), clob::POLYGON);
    
    clob::Signer signer_amoy(private_key, clob::AMOY);
    EXPECT_EQ(signer_amoy.get_chain_id(), clob::AMOY);
}

// Test signing produces valid signature format
TEST(SignerTest, SignatureFormat) {
    std::string private_key = "0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80";
    clob::Signer signer(private_key, clob::POLYGON);
    
    // Create a dummy message hash
    std::array<uint8_t, 32> message_hash{};
    message_hash[31] = 1;
    
    std::string signature = signer.sign(message_hash);
    
    // Should be 0x + 130 hex chars (65 bytes)
    EXPECT_EQ(signature.length(), 132);
    EXPECT_EQ(signature.substr(0, 2), "0x");
}

// Test deterministic signing
TEST(SignerTest, DeterministicSignatures) {
    std::string private_key = "0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80";
    clob::Signer signer(private_key, clob::POLYGON);
    
    std::array<uint8_t, 32> message_hash{};
    message_hash[31] = 42;
    
    // Sign twice
    std::string sig1 = signer.sign(message_hash);
    std::string sig2 = signer.sign(message_hash);
    
    // Should produce identical signatures
    EXPECT_EQ(sig1, sig2);
}

// Test multiple signers don't interfere
TEST(SignerTest, MultipleSigners) {
    std::string pk1 = "0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80";
    std::string pk2 = "0x59c6995e998f97a5a0044966f0945389dc9e86dae88c7a8412f4603b6b78690d";
    
    clob::Signer signer1(pk1, clob::POLYGON);
    clob::Signer signer2(pk2, clob::POLYGON);
    
    // Different addresses
    EXPECT_NE(signer1.address(), signer2.address());
    
    // Different signatures for same message
    std::array<uint8_t, 32> message{};
    message[31] = 1;
    
    auto sig1 = signer1.sign(message);
    auto sig2 = signer2.sign(message);
    
    EXPECT_NE(sig1, sig2);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}



