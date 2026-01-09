#pragma once

#include <string>
#include <vector>
#include <array>
#include <nlohmann/json.hpp>

namespace clob {
namespace eip712 {

using json = nlohmann::json;

// Keccak-256 hash function
std::array<uint8_t, 32> keccak256(const std::vector<uint8_t>& data);

// Encode EIP712 domain
std::vector<uint8_t> encode_domain(const json& domain);

// Hash EIP712 domain
std::array<uint8_t, 32> hash_domain(const json& domain);

// Encode a struct according to EIP712
std::vector<uint8_t> encode_struct(
    const std::string& primary_type,
    const json& data,
    const json& types
);

// Hash a struct according to EIP712
std::array<uint8_t, 32> hash_struct(
    const std::string& primary_type,
    const json& data,
    const json& types
);

// Encode a single value based on its type
std::vector<uint8_t> encode_value(
    const std::string& type,
    const json& value,
    const json& types
);

// Get the type hash for a struct
std::array<uint8_t, 32> type_hash(
    const std::string& primary_type,
    const json& types
);

// Build the full EIP712 signing hash
std::array<uint8_t, 32> signing_hash(
    const json& domain,
    const std::string& primary_type,
    const json& message,
    const json& types
);

// Helper to convert hex string to bytes
std::vector<uint8_t> hex_to_bytes(const std::string& hex);

// Helper to convert bytes to hex string
std::string bytes_to_hex(const std::vector<uint8_t>& bytes, bool with_prefix = true);

// Helper for uint256 encoding
std::array<uint8_t, 32> encode_uint256(uint64_t value);
std::array<uint8_t, 32> encode_uint256(const std::string& value);

// Helper for address encoding
std::array<uint8_t, 32> encode_address(const std::string& address);

// Helper for string encoding
std::array<uint8_t, 32> encode_string(const std::string& str);

} // namespace eip712
} // namespace clob



