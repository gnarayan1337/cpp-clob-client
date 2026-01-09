#pragma once

#include <string>
#include <vector>
#include <array>
#include <memory>
#include <secp256k1.h>
#include <secp256k1_recovery.h>
#include "types.hpp"

namespace clob {

class Signer {
public:
    // Constructor with hex private key (with or without 0x prefix)
    explicit Signer(const std::string& private_key_hex, uint64_t chain_id);
    ~Signer();
    
    // Get the public address derived from private key
    std::string address() const;
    
    // Get chain ID
    uint64_t get_chain_id() const { return chain_id_; }
    
    // Sign a raw message hash (32 bytes)
    std::string sign(const std::array<uint8_t, 32>& message_hash) const;
    
    // Sign a hash and return r, s, v components separately
    // Used for transaction signing
    struct SignatureComponents {
        std::vector<uint8_t> r;
        std::vector<uint8_t> s;
        uint8_t v;
    };
    SignatureComponents sign_hash(const std::vector<uint8_t>& hash) const;
    
    // Sign EIP712 typed data
    std::string sign_typed_data(
        const json& domain,
        const std::string& primary_type,
        const json& message,
        const json& types
    ) const;

private:
    std::array<uint8_t, 32> private_key_;
    std::string address_;
    uint64_t chain_id_;
    secp256k1_context* ctx_;
    
    // Compute address from public key
    static std::string compute_address(const std::vector<uint8_t>& public_key);
};

} // namespace clob



