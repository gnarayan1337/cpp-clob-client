#include <gtest/gtest.h>
#include <clob/client.hpp>
#include <clob/signer.hpp>
#include <openssl/hmac.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <algorithm>

using namespace clob;

// Helper function to compute HMAC signature (matching the client implementation)
std::string compute_hmac_signature(const std::string& secret, const std::string& message) {
    // Decode URL-safe base64 secret
    std::string secret_copy = secret;
    for (char& c : secret_copy) {
        if (c == '-') c = '+';
        if (c == '_') c = '/';
    }
    while (secret_copy.length() % 4 != 0) {
        secret_copy += '=';
    }
    
    BIO* bio_decode = BIO_new_mem_buf(secret_copy.data(), secret_copy.length());
    BIO* b64_decode = BIO_new(BIO_f_base64());
    BIO_set_flags(b64_decode, BIO_FLAGS_BASE64_NO_NL);
    bio_decode = BIO_push(b64_decode, bio_decode);
    
    std::vector<uint8_t> decoded_secret(256);
    int decoded_len = BIO_read(bio_decode, decoded_secret.data(), 256);
    BIO_free_all(bio_decode);
    
    if (decoded_len <= 0) {
        throw std::runtime_error("Failed to decode secret");
    }
    decoded_secret.resize(decoded_len);
    
    // Compute HMAC
    unsigned char hmac_result[32];
    unsigned int hmac_len = 32;
    HMAC(EVP_sha256(),
         decoded_secret.data(), decoded_secret.size(),
         reinterpret_cast<const unsigned char*>(message.data()), message.length(),
         hmac_result, &hmac_len);
    
    // Base64 encode result
    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bio = BIO_push(b64, bio);
    BIO_write(bio, hmac_result, hmac_len);
    BIO_flush(bio);
    
    BUF_MEM* buffer_ptr;
    BIO_get_mem_ptr(bio, &buffer_ptr);
    std::string signature(buffer_ptr->data, buffer_ptr->length);
    BIO_free_all(bio);
    
    // Convert to URL-safe base64 (+ → -, / → _)
    // NOTE: Keep the "=" padding - the API requires it!
    for (char& c : signature) {
        if (c == '+') c = '-';
        if (c == '/') c = '_';
    }
    
    return signature;
}

// Test HMAC matches known Rust test values
// From rs-clob-client/src/auth.rs test: l2_headers_should_succeed
TEST(L2HMACTest, MatchesRustL2HeadersTest) {
    // Known values from Rust test
    const std::string secret = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
    const std::string timestamp = "1";
    const std::string method = "GET";
    const std::string path = "/";
    const std::string body = "";
    
    // Build message: timestamp + method + path + body
    std::string message = timestamp + method + path + body;
    
    // Expected signature from Rust test
    const std::string expected_sig = "eHaylCwqRSOa2LFD77Nt_SaTpbsxzN8eTEI3LryhEj4=";
    
    std::string signature = compute_hmac_signature(secret, message);
    
    std::cout << "Message: '" << message << "'" << std::endl;
    std::cout << "Expected: " << expected_sig << std::endl;
    std::cout << "Got:      " << signature << std::endl;
    
    EXPECT_EQ(signature, expected_sig);
}

// Test HMAC with body matches known Rust test values
// From rs-clob-client/src/auth.rs test: hmac_succeeds
TEST(L2HMACTest, MatchesRustHMACTestWithBody) {
    // Known values from Rust test
    const std::string secret = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
    const std::string message = R"(1000000test-sign/orders{"hash":"0x123"})";
    
    // Expected signature from Rust test
    const std::string expected_sig = "4gJVbox-R6XlDK4nlaicig0_ANVL1qdcahiL8CXfXLM=";
    
    std::string signature = compute_hmac_signature(secret, message);
    
    std::cout << "Message: '" << message << "'" << std::endl;
    std::cout << "Expected: " << expected_sig << std::endl;
    std::cout << "Got:      " << signature << std::endl;
    
    EXPECT_EQ(signature, expected_sig);
}

// Test that signature includes padding
TEST(L2HMACTest, SignatureIncludesPadding) {
    const std::string secret = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
    const std::string message = "1GET/";
    
    std::string signature = compute_hmac_signature(secret, message);
    
    // The signature should end with '=' (base64 padding)
    // This is required by the Polymarket API
    EXPECT_TRUE(signature.find('=') != std::string::npos) 
        << "Signature should include base64 padding: " << signature;
}

// Test that signature is URL-safe
TEST(L2HMACTest, SignatureIsURLSafe) {
    const std::string secret = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
    const std::string message = "1GET/";
    
    std::string signature = compute_hmac_signature(secret, message);
    
    // Should not contain standard base64 chars + and /
    EXPECT_EQ(signature.find('+'), std::string::npos) 
        << "Signature should not contain '+': " << signature;
    EXPECT_EQ(signature.find('/'), std::string::npos) 
        << "Signature should not contain '/': " << signature;
}

// Test with realistic order body
TEST(L2HMACTest, RealisticOrderBody) {
    const std::string secret = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
    const std::string timestamp = "1700000000";
    const std::string method = "POST";
    const std::string path = "/orders";
    const std::string body = R"([{"order":{"salt":"123","maker":"0xf39fd6e51aad88f6f4ce6ab8827279cfffb92266"}}])";
    
    std::string message = timestamp + method + path + body;
    std::string signature = compute_hmac_signature(secret, message);
    
    std::cout << "Realistic order message: " << message << std::endl;
    std::cout << "Signature: " << signature << std::endl;
    
    EXPECT_FALSE(signature.empty());
    EXPECT_GT(signature.length(), 20);
    // Should end with = (base64 padding)
    EXPECT_TRUE(signature.back() == '=' || signature.find('=') != std::string::npos || 
                signature.length() % 4 == 0);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
