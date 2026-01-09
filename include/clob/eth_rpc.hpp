#pragma once

#include <string>
#include <optional>
#include <vector>
#include <nlohmann/json.hpp>

namespace clob {

using json = nlohmann::json;

// Contract addresses for Polymarket on Polygon
namespace polygon_contracts {
    // USDC on Polygon
    constexpr const char* USDC = "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174";
    
    // Polymarket Exchange (non-neg-risk)
    constexpr const char* EXCHANGE = "0x4bFb41d5B3570DeFd03C39a9A4D8dE6Bd8B8982E";
    
    // Polymarket Neg-Risk Exchange
    constexpr const char* NEG_RISK_EXCHANGE = "0xC5d563A36AE78145C45a50134d48A1215220f80a";
    
    // Conditional Tokens Framework
    constexpr const char* CTF = "0x4D97DCd97eC945f40cF65F87097ACe5EA0476045";
}

// Ethereum JSON-RPC client for on-chain transactions
class EthRpcClient {
public:
    explicit EthRpcClient(const std::string& rpc_url = "https://polygon-rpc.com");
    
    // Get the current nonce for an address
    uint64_t get_nonce(const std::string& address);
    
    // Get current gas price
    std::string get_gas_price();
    
    // Get chain ID
    uint64_t get_chain_id();
    
    // Send a raw signed transaction
    std::string send_raw_transaction(const std::string& signed_tx_hex);
    
    // Wait for transaction receipt
    json wait_for_receipt(const std::string& tx_hash, int timeout_seconds = 60);
    
    // Check ERC20 allowance
    std::string get_allowance(const std::string& token, const std::string& owner, const std::string& spender);
    
    // Check ERC1155 isApprovedForAll
    bool is_approved_for_all(const std::string& token, const std::string& owner, const std::string& operator_addr);

private:
    std::string rpc_url_;
    int request_id_ = 1;
    
    json rpc_call(const std::string& method, const json& params);
};

// RLP encoding utilities
namespace rlp {
    std::vector<uint8_t> encode_string(const std::vector<uint8_t>& data);
    std::vector<uint8_t> encode_list(const std::vector<std::vector<uint8_t>>& items);
    std::vector<uint8_t> encode_integer(uint64_t value);
}

// Transaction builder and signer
struct Transaction {
    uint64_t nonce;
    std::string gas_price;      // hex
    uint64_t gas_limit;
    std::string to;             // address
    std::string value;          // hex (usually "0x0")
    std::string data;           // hex encoded calldata
    uint64_t chain_id;
    
    // Sign the transaction and return hex-encoded signed tx
    std::string sign(const std::string& private_key) const;
};

// ABI encoding helpers
namespace abi {
    // Encode ERC20 approve(address spender, uint256 amount)
    std::string encode_approve(const std::string& spender, const std::string& amount_hex);
    
    // Encode ERC1155 setApprovalForAll(address operator, bool approved)
    std::string encode_set_approval_for_all(const std::string& operator_addr, bool approved);
    
    // Encode ERC20 allowance(address owner, address spender) call
    std::string encode_allowance(const std::string& owner, const std::string& spender);
    
    // Encode ERC1155 isApprovedForAll(address account, address operator) call
    std::string encode_is_approved_for_all(const std::string& account, const std::string& operator_addr);
}

// High-level approval helper
class ApprovalHelper {
public:
    ApprovalHelper(const std::string& private_key, const std::string& rpc_url = "https://polygon-rpc.com");
    
    // Check and set up all required approvals for Polymarket trading
    // Returns true if all approvals are already set, false if transactions were sent
    bool ensure_approvals();
    
    // Individual approval methods
    std::string approve_usdc_for_exchange();
    std::string approve_usdc_for_neg_risk_exchange();
    std::string approve_ctf_for_exchange();
    std::string approve_ctf_for_neg_risk_exchange();
    
    // Check current approval status
    bool has_usdc_exchange_approval();
    bool has_usdc_neg_risk_approval();
    bool has_ctf_exchange_approval();
    bool has_ctf_neg_risk_approval();

private:
    std::string private_key_;
    std::string address_;
    EthRpcClient rpc_;
};

} // namespace clob

