#include "clob/signer.hpp"
#include "clob/eip712.hpp"
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace clob {

Signer::Signer(const std::string& private_key_hex, uint64_t chain_id)
    : chain_id_(chain_id) {
    
    // Create secp256k1 context
    ctx_ = secp256k1_context_create(SECP256K1_CONTEXT_SIGN);
    if (!ctx_) {
        throw std::runtime_error("Failed to create secp256k1 context");
    }
    
    // Parse private key
    auto key_bytes = eip712::hex_to_bytes(private_key_hex);
    if (key_bytes.size() != 32) {
        secp256k1_context_destroy(ctx_);
        throw std::runtime_error("Invalid private key length");
    }
    
    std::copy(key_bytes.begin(), key_bytes.end(), private_key_.begin());
    
    // Verify private key is valid
    if (!secp256k1_ec_seckey_verify(ctx_, private_key_.data())) {
        secp256k1_context_destroy(ctx_);
        throw std::runtime_error("Invalid private key");
    }
    
    // Derive public key and address
    secp256k1_pubkey pubkey;
    if (!secp256k1_ec_pubkey_create(ctx_, &pubkey, private_key_.data())) {
        secp256k1_context_destroy(ctx_);
        throw std::runtime_error("Failed to derive public key");
    }
    
    // Serialize public key (uncompressed, 65 bytes)
    size_t pub_len = 65;
    std::vector<uint8_t> pub_bytes(pub_len);
    secp256k1_ec_pubkey_serialize(
        ctx_, pub_bytes.data(), &pub_len, &pubkey, 
        SECP256K1_EC_UNCOMPRESSED
    );
    
    address_ = compute_address(pub_bytes);
}

Signer::~Signer() {
    if (ctx_) {
        secp256k1_context_destroy(ctx_);
    }
}

std::string Signer::address() const {
    return address_;
}

std::string Signer::sign(const std::array<uint8_t, 32>& message_hash) const {
    secp256k1_ecdsa_recoverable_signature sig;
    
    if (!secp256k1_ecdsa_sign_recoverable(
            ctx_, &sig, message_hash.data(), private_key_.data(),
            nullptr, nullptr)) {
        throw std::runtime_error("Failed to sign message");
    }
    
    // Serialize signature
    std::array<uint8_t, 64> compact;
    int recid;
    secp256k1_ecdsa_recoverable_signature_serialize_compact(
        ctx_, compact.data(), &recid, &sig
    );
    
    // Format as Ethereum signature (r + s + v)
    std::vector<uint8_t> signature;
    signature.insert(signature.end(), compact.begin(), compact.end());
    signature.push_back(static_cast<uint8_t>(recid + 27));  // v = recid + 27
    
    return eip712::bytes_to_hex(signature);
}

Signer::SignatureComponents Signer::sign_hash(const std::vector<uint8_t>& hash) const {
    if (hash.size() != 32) {
        throw std::runtime_error("Hash must be 32 bytes");
    }
    
    secp256k1_ecdsa_recoverable_signature sig;
    
    if (!secp256k1_ecdsa_sign_recoverable(
            ctx_, &sig, hash.data(), private_key_.data(),
            nullptr, nullptr)) {
        throw std::runtime_error("Failed to sign hash");
    }
    
    // Serialize signature
    std::array<uint8_t, 64> compact;
    int recid;
    secp256k1_ecdsa_recoverable_signature_serialize_compact(
        ctx_, compact.data(), &recid, &sig
    );
    
    SignatureComponents result;
    result.r = std::vector<uint8_t>(compact.begin(), compact.begin() + 32);
    result.s = std::vector<uint8_t>(compact.begin() + 32, compact.end());
    result.v = static_cast<uint8_t>(recid);
    
    return result;
}

std::string Signer::sign_typed_data(
    const json& domain,
    const std::string& primary_type,
    const json& message,
    const json& types
) const {
    auto hash = eip712::signing_hash(domain, primary_type, message, types);
    return sign(hash);
}

std::string Signer::compute_address(const std::vector<uint8_t>& public_key) {
    if (public_key.size() != 65 || public_key[0] != 0x04) {
        throw std::runtime_error("Invalid public key format");
    }
    
    // Hash the public key (excluding the 0x04 prefix)
    std::vector<uint8_t> key_without_prefix(public_key.begin() + 1, public_key.end());
    auto hash = eip712::keccak256(key_without_prefix);
    
    // Take last 20 bytes
    std::vector<uint8_t> addr_bytes(hash.end() - 20, hash.end());
    
    return eip712::bytes_to_hex(addr_bytes);
}

} // namespace clob



