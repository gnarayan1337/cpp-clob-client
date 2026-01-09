#include "clob/eip712.hpp"
#include "clob/keccak.hpp"
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <cstring>

namespace clob {
namespace eip712 {

// Keccak-256 using bundled implementation (works on all platforms)
std::array<uint8_t, 32> keccak256(const std::vector<uint8_t>& data) {
    return keccak::hash256(data);
}

std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
    std::string clean_hex = hex;
    if (clean_hex.substr(0, 2) == "0x" || clean_hex.substr(0, 2) == "0X") {
        clean_hex = clean_hex.substr(2);
    }
    
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < clean_hex.length(); i += 2) {
        std::string byte_str = clean_hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16));
        bytes.push_back(byte);
    }
    
    return bytes;
}

std::string bytes_to_hex(const std::vector<uint8_t>& bytes, bool with_prefix) {
    std::ostringstream oss;
    if (with_prefix) {
        oss << "0x";
    }
    oss << std::hex << std::setfill('0');
    for (uint8_t byte : bytes) {
        oss << std::setw(2) << static_cast<int>(byte);
    }
    return oss.str();
}

std::array<uint8_t, 32> encode_uint256(uint64_t value) {
    std::array<uint8_t, 32> result{};
    for (int i = 0; i < 8; ++i) {
        result[31 - i] = (value >> (i * 8)) & 0xFF;
    }
    return result;
}

std::array<uint8_t, 32> encode_uint256(const std::string& value) {
    // Handle large 256-bit numbers that don't fit in uint64_t
    // Parse as big integer using hex if needed
    
    std::array<uint8_t, 32> result{};
    
    // Try to parse as uint64_t first (for small numbers)
    try {
        uint64_t num = std::stoull(value);
        return encode_uint256(num);
    } catch (...) {
        // Number too large for uint64_t, parse as big integer
        // Convert decimal string to bytes manually
        
        // Simple approach: convert to hex first, then to bytes
        // For very large numbers, we'll use a basic big integer conversion
        
        std::string decimal_str = value;
        std::vector<uint8_t> bytes;
        
        // Convert decimal string to bytes (big-endian)
        while (decimal_str != "0" && !decimal_str.empty()) {
            // Divide by 256 and get remainder
            uint64_t remainder = 0;
            std::string quotient;
            
            for (char c : decimal_str) {
                uint64_t digit = c - '0';
                uint64_t current = remainder * 10 + digit;
                if (!quotient.empty() || current >= 256) {
                    quotient += char('0' + (current / 256));
                }
                remainder = current % 256;
            }
            
            bytes.insert(bytes.begin(), static_cast<uint8_t>(remainder));
            decimal_str = quotient.empty() ? "0" : quotient;
        }
        
        // Pad to 32 bytes
        if (bytes.size() > 32) {
            throw std::runtime_error("Number too large for uint256");
        }
        
        // Copy to result (right-aligned, big-endian)
        size_t offset = 32 - bytes.size();
        std::copy(bytes.begin(), bytes.end(), result.begin() + offset);
        
        return result;
    }
}

std::array<uint8_t, 32> encode_address(const std::string& address) {
    std::array<uint8_t, 32> result{};
    auto bytes = hex_to_bytes(address);
    
    if (bytes.size() != 20) {
        throw std::runtime_error("Invalid address length");
    }
    
    // Addresses are left-padded to 32 bytes
    std::copy(bytes.begin(), bytes.end(), result.begin() + 12);
    return result;
}

std::array<uint8_t, 32> encode_string(const std::string& str) {
    std::vector<uint8_t> data(str.begin(), str.end());
    return keccak256(data);
}

std::array<uint8_t, 32> type_hash(const std::string& primary_type, const json& types) {
    // Build the type string, e.g., "Order(uint256 salt,address maker,...)"
    std::string type_str = primary_type + "(";
    
    if (!types.contains(primary_type)) {
        throw std::runtime_error("Type not found: " + primary_type);
    }
    
    const auto& fields = types[primary_type];
    for (size_t i = 0; i < fields.size(); ++i) {
        if (i > 0) type_str += ",";
        type_str += fields[i]["type"].get<std::string>() + " " + 
                    fields[i]["name"].get<std::string>();
    }
    type_str += ")";
    
    std::vector<uint8_t> data(type_str.begin(), type_str.end());
    return keccak256(data);
}

std::vector<uint8_t> encode_value(const std::string& type, const json& value, const json& types) {
    std::vector<uint8_t> result;
    
    if (type == "string") {
        auto hash = encode_string(value.get<std::string>());
        result.insert(result.end(), hash.begin(), hash.end());
    }
    else if (type == "address") {
        auto encoded = encode_address(value.get<std::string>());
        result.insert(result.end(), encoded.begin(), encoded.end());
    }
    else if (type.find("uint") == 0) {
        std::array<uint8_t, 32> encoded;
        if (value.is_number()) {
            encoded = encode_uint256(value.get<uint64_t>());
        } else {
            encoded = encode_uint256(value.get<std::string>());
        }
        result.insert(result.end(), encoded.begin(), encoded.end());
    }
    else if (type.find("int") == 0) {
        // For simplicity, treat as uint
        std::array<uint8_t, 32> encoded;
        if (value.is_number()) {
            encoded = encode_uint256(value.get<uint64_t>());
        } else {
            encoded = encode_uint256(value.get<std::string>());
        }
        result.insert(result.end(), encoded.begin(), encoded.end());
    }
    else if (type == "bytes") {
        auto bytes = hex_to_bytes(value.get<std::string>());
        auto hash = keccak256(bytes);
        result.insert(result.end(), hash.begin(), hash.end());
    }
    else if (types.contains(type)) {
        // Nested struct
        auto hash = hash_struct(type, value, types);
        result.insert(result.end(), hash.begin(), hash.end());
    }
    else {
        throw std::runtime_error("Unsupported type: " + type);
    }
    
    return result;
}

std::vector<uint8_t> encode_struct(
    const std::string& primary_type,
    const json& data,
    const json& types
) {
    std::vector<uint8_t> result;
    
    // Add type hash
    auto th = type_hash(primary_type, types);
    result.insert(result.end(), th.begin(), th.end());
    
    // Add encoded values in order
    const auto& fields = types[primary_type];
    for (const auto& field : fields) {
        std::string field_type = field["type"].get<std::string>();
        std::string field_name = field["name"].get<std::string>();
        
        auto encoded = encode_value(field_type, data[field_name], types);
        result.insert(result.end(), encoded.begin(), encoded.end());
    }
    
    return result;
}

std::array<uint8_t, 32> hash_struct(
    const std::string& primary_type,
    const json& data,
    const json& types
) {
    auto encoded = encode_struct(primary_type, data, types);
    return keccak256(encoded);
}

std::vector<uint8_t> encode_domain(const json& domain) {
    std::vector<uint8_t> result;
    
    // Build domain type string based on fields present
    std::string domain_type = "EIP712Domain(";
    std::vector<std::string> fields;
    
    if (domain.contains("name")) fields.push_back("string name");
    if (domain.contains("version")) fields.push_back("string version");
    if (domain.contains("chainId")) fields.push_back("uint256 chainId");
    if (domain.contains("verifyingContract")) fields.push_back("address verifyingContract");
    if (domain.contains("salt")) fields.push_back("bytes32 salt");
    
    for (size_t i = 0; i < fields.size(); ++i) {
        if (i > 0) domain_type += ",";
        domain_type += fields[i];
    }
    domain_type += ")";
    
    // Hash the type string
    std::vector<uint8_t> type_data(domain_type.begin(), domain_type.end());
    auto th = keccak256(type_data);
    result.insert(result.end(), th.begin(), th.end());
    
    // Encode values in the same order as the type string
    if (domain.contains("name")) {
        auto name_hash = encode_string(domain["name"].get<std::string>());
        result.insert(result.end(), name_hash.begin(), name_hash.end());
    }
    
    if (domain.contains("version")) {
        auto version_hash = encode_string(domain["version"].get<std::string>());
        result.insert(result.end(), version_hash.begin(), version_hash.end());
    }
    
    if (domain.contains("chainId")) {
        auto chain_id = encode_uint256(domain["chainId"].get<uint64_t>());
        result.insert(result.end(), chain_id.begin(), chain_id.end());
    }
    
    if (domain.contains("verifyingContract")) {
        auto contract = encode_address(domain["verifyingContract"].get<std::string>());
        result.insert(result.end(), contract.begin(), contract.end());
    }
    
    if (domain.contains("salt")) {
        auto salt = hex_to_bytes(domain["salt"].get<std::string>());
        if (salt.size() != 32) {
            throw std::runtime_error("Domain salt must be 32 bytes");
        }
        result.insert(result.end(), salt.begin(), salt.end());
    }
    
    return result;
}

std::array<uint8_t, 32> hash_domain(const json& domain) {
    auto encoded = encode_domain(domain);
    return keccak256(encoded);
}

std::array<uint8_t, 32> signing_hash(
    const json& domain,
    const std::string& primary_type,
    const json& message,
    const json& types
) {
    std::vector<uint8_t> data;
    
    // Add "\x19\x01" prefix
    data.push_back(0x19);
    data.push_back(0x01);
    
    // Add domain separator
    auto domain_sep = hash_domain(domain);
    data.insert(data.end(), domain_sep.begin(), domain_sep.end());
    
    // Add struct hash
    auto struct_h = hash_struct(primary_type, message, types);
    data.insert(data.end(), struct_h.begin(), struct_h.end());
    
    // Hash everything
    return keccak256(data);
}

} // namespace eip712
} // namespace clob
