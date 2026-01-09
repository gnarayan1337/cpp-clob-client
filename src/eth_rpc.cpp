#include "clob/eth_rpc.hpp"
#include "clob/signer.hpp"
#include "clob/eip712.hpp"
#include <httplib.h>
#include <thread>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>

namespace clob {

// Helper to convert hex string to bytes
// Handles "0x0" and "0x00" as empty (for RLP zero encoding)
static std::vector<uint8_t> hex_to_bytes(const std::string& hex, bool strip_leading_zeros = false) {
    std::string h = hex;
    if (h.substr(0, 2) == "0x") h = h.substr(2);
    
    // Handle odd-length hex strings
    if (h.length() % 2 == 1) h = "0" + h;
    
    // For RLP, "0" or "00" should become empty bytes
    if (strip_leading_zeros) {
        while (h.length() >= 2 && h.substr(0, 2) == "00") {
            h = h.substr(2);
        }
        if (h.empty() || h == "0") return {};
    }
    
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i < h.length(); i += 2) {
        bytes.push_back(static_cast<uint8_t>(std::stoul(h.substr(i, 2), nullptr, 16)));
    }
    return bytes;
}

// Helper to convert bytes to hex string
static std::string bytes_to_hex(const std::vector<uint8_t>& bytes, bool prefix = true) {
    std::ostringstream oss;
    if (prefix) oss << "0x";
    for (uint8_t b : bytes) {
        oss << std::hex << std::setfill('0') << std::setw(2) << static_cast<int>(b);
    }
    return oss.str();
}

// Helper to pad address to 32 bytes
static std::string pad_address(const std::string& addr) {
    std::string a = addr;
    if (a.substr(0, 2) == "0x") a = a.substr(2);
    return std::string(64 - a.length(), '0') + a;
}

// Helper to pad uint256
static std::string pad_uint256(const std::string& hex_value) {
    std::string v = hex_value;
    if (v.substr(0, 2) == "0x") v = v.substr(2);
    if (v.empty()) v = "0";
    return std::string(64 - v.length(), '0') + v;
}

// ==================== RLP Encoding ====================

namespace rlp {

std::vector<uint8_t> encode_string(const std::vector<uint8_t>& data) {
    std::vector<uint8_t> result;
    
    if (data.size() == 1 && data[0] < 0x80) {
        // Single byte < 0x80 is its own RLP encoding
        result.push_back(data[0]);
    } else if (data.size() < 56) {
        // Short string: 0x80 + length
        result.push_back(0x80 + data.size());
        result.insert(result.end(), data.begin(), data.end());
    } else {
        // Long string: 0xb7 + length of length, then length, then data
        std::vector<uint8_t> len_bytes;
        size_t len = data.size();
        while (len > 0) {
            len_bytes.insert(len_bytes.begin(), len & 0xff);
            len >>= 8;
        }
        result.push_back(0xb7 + len_bytes.size());
        result.insert(result.end(), len_bytes.begin(), len_bytes.end());
        result.insert(result.end(), data.begin(), data.end());
    }
    
    return result;
}

std::vector<uint8_t> encode_list(const std::vector<std::vector<uint8_t>>& items) {
    std::vector<uint8_t> payload;
    for (const auto& item : items) {
        payload.insert(payload.end(), item.begin(), item.end());
    }
    
    std::vector<uint8_t> result;
    if (payload.size() < 56) {
        result.push_back(0xc0 + payload.size());
    } else {
        std::vector<uint8_t> len_bytes;
        size_t len = payload.size();
        while (len > 0) {
            len_bytes.insert(len_bytes.begin(), len & 0xff);
            len >>= 8;
        }
        result.push_back(0xf7 + len_bytes.size());
        result.insert(result.end(), len_bytes.begin(), len_bytes.end());
    }
    result.insert(result.end(), payload.begin(), payload.end());
    
    return result;
}

std::vector<uint8_t> encode_integer(uint64_t value) {
    if (value == 0) {
        return encode_string({});
    }
    
    std::vector<uint8_t> bytes;
    while (value > 0) {
        bytes.insert(bytes.begin(), value & 0xff);
        value >>= 8;
    }
    return encode_string(bytes);
}

} // namespace rlp

// ==================== ABI Encoding ====================

namespace abi {

std::string encode_approve(const std::string& spender, const std::string& amount_hex) {
    // approve(address,uint256) = 0x095ea7b3
    return "0x095ea7b3" + pad_address(spender) + pad_uint256(amount_hex);
}

std::string encode_set_approval_for_all(const std::string& operator_addr, bool approved) {
    // setApprovalForAll(address,bool) = 0xa22cb465
    return "0xa22cb465" + pad_address(operator_addr) + pad_uint256(approved ? "1" : "0");
}

std::string encode_allowance(const std::string& owner, const std::string& spender) {
    // allowance(address,address) = 0xdd62ed3e
    return "0xdd62ed3e" + pad_address(owner) + pad_address(spender);
}

std::string encode_is_approved_for_all(const std::string& account, const std::string& operator_addr) {
    // isApprovedForAll(address,address) = 0xe985e9c5
    return "0xe985e9c5" + pad_address(account) + pad_address(operator_addr);
}

} // namespace abi

// ==================== EthRpcClient ====================

EthRpcClient::EthRpcClient(const std::string& rpc_url) : rpc_url_(rpc_url) {}

json EthRpcClient::rpc_call(const std::string& method, const json& params) {
    json request = {
        {"jsonrpc", "2.0"},
        {"method", method},
        {"params", params},
        {"id", request_id_++}
    };
    
    // Parse URL
    std::string host, path;
    int port = 443;
    
    if (rpc_url_.find("https://") == 0) {
        std::string rest = rpc_url_.substr(8);
        size_t slash = rest.find('/');
        if (slash != std::string::npos) {
            host = rest.substr(0, slash);
            path = rest.substr(slash);
        } else {
            host = rest;
            path = "/";
        }
    }
    
    httplib::SSLClient cli(host, port);
    cli.set_connection_timeout(30);
    cli.set_read_timeout(30);
    
    auto res = cli.Post(path.c_str(), request.dump(), "application/json");
    
    if (!res) {
        throw std::runtime_error("RPC request failed");
    }
    
    if (res->status != 200) {
        throw std::runtime_error("RPC error: " + res->body);
    }
    
    json response = json::parse(res->body);
    if (response.contains("error")) {
        throw std::runtime_error("RPC error: " + response["error"].dump());
    }
    
    return response["result"];
}

uint64_t EthRpcClient::get_nonce(const std::string& address) {
    auto result = rpc_call("eth_getTransactionCount", {address, "pending"});
    return std::stoull(result.get<std::string>(), nullptr, 16);
}

std::string EthRpcClient::get_gas_price() {
    return rpc_call("eth_gasPrice", json::array()).get<std::string>();
}

uint64_t EthRpcClient::get_chain_id() {
    auto result = rpc_call("eth_chainId", json::array());
    return std::stoull(result.get<std::string>(), nullptr, 16);
}

std::string EthRpcClient::send_raw_transaction(const std::string& signed_tx_hex) {
    return rpc_call("eth_sendRawTransaction", {signed_tx_hex}).get<std::string>();
}

json EthRpcClient::wait_for_receipt(const std::string& tx_hash, int timeout_seconds) {
    auto start = std::chrono::steady_clock::now();
    
    while (true) {
        try {
            auto result = rpc_call("eth_getTransactionReceipt", {tx_hash});
            if (!result.is_null()) {
                return result;
            }
        } catch (...) {
            // Receipt not available yet
        }
        
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start
        ).count();
        
        if (elapsed >= timeout_seconds) {
            throw std::runtime_error("Timeout waiting for transaction receipt");
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

std::string EthRpcClient::get_allowance(const std::string& token, const std::string& owner, const std::string& spender) {
    json params = {
        {{"to", token}, {"data", abi::encode_allowance(owner, spender)}},
        "latest"
    };
    return rpc_call("eth_call", params).get<std::string>();
}

bool EthRpcClient::is_approved_for_all(const std::string& token, const std::string& owner, const std::string& operator_addr) {
    json params = {
        {{"to", token}, {"data", abi::encode_is_approved_for_all(owner, operator_addr)}},
        "latest"
    };
    std::string result = rpc_call("eth_call", params).get<std::string>();
    // Result is 32 bytes, check if non-zero
    return result != "0x0000000000000000000000000000000000000000000000000000000000000000";
}

// ==================== Transaction ====================

std::string Transaction::sign(const std::string& private_key) const {
    using namespace rlp;
    
    // EIP-155 transaction structure for signing:
    // [nonce, gasPrice, gasLimit, to, value, data, chainId, 0, 0]
    
    auto gas_price_bytes = hex_to_bytes(gas_price);
    auto to_bytes = hex_to_bytes(to);
    auto value_bytes = hex_to_bytes(value, true);  // strip leading zeros for RLP
    auto data_bytes = hex_to_bytes(data);
    
    std::vector<std::vector<uint8_t>> tx_items = {
        encode_integer(nonce),
        encode_string(gas_price_bytes),
        encode_integer(gas_limit),
        encode_string(to_bytes),
        encode_string(value_bytes),
        encode_string(data_bytes),
        encode_integer(chain_id),
        encode_string({}),  // empty for EIP-155
        encode_string({})   // empty for EIP-155
    };
    
    auto unsigned_tx = encode_list(tx_items);
    
    // Keccak-256 hash of unsigned transaction
    auto hash_arr = eip712::keccak256(unsigned_tx);
    std::vector<uint8_t> hash_vec(hash_arr.begin(), hash_arr.end());
    
    // Sign with secp256k1
    Signer signer(private_key, chain_id);
    auto sig = signer.sign_hash(hash_vec);
    
    // Adjust v for EIP-155
    uint64_t v_adjusted = sig.v + chain_id * 2 + 35;
    
    // RLP encode signed transaction
    std::vector<std::vector<uint8_t>> signed_items = {
        encode_integer(nonce),
        encode_string(gas_price_bytes),
        encode_integer(gas_limit),
        encode_string(to_bytes),
        encode_string(value_bytes),
        encode_string(data_bytes),
        encode_integer(v_adjusted),
        encode_string(sig.r),
        encode_string(sig.s)
    };
    
    auto signed_tx = encode_list(signed_items);
    return bytes_to_hex(signed_tx);
}

// ==================== ApprovalHelper ====================

ApprovalHelper::ApprovalHelper(const std::string& private_key, const std::string& rpc_url)
    : private_key_(private_key), rpc_(rpc_url) {
    Signer signer(private_key, 137);
    address_ = signer.address();
}

bool ApprovalHelper::has_usdc_exchange_approval() {
    auto allowance = rpc_.get_allowance(polygon_contracts::USDC, address_, polygon_contracts::EXCHANGE);
    return allowance != "0x0000000000000000000000000000000000000000000000000000000000000000";
}

bool ApprovalHelper::has_usdc_neg_risk_approval() {
    auto allowance = rpc_.get_allowance(polygon_contracts::USDC, address_, polygon_contracts::NEG_RISK_EXCHANGE);
    return allowance != "0x0000000000000000000000000000000000000000000000000000000000000000";
}

bool ApprovalHelper::has_ctf_exchange_approval() {
    return rpc_.is_approved_for_all(polygon_contracts::CTF, address_, polygon_contracts::EXCHANGE);
}

bool ApprovalHelper::has_ctf_neg_risk_approval() {
    return rpc_.is_approved_for_all(polygon_contracts::CTF, address_, polygon_contracts::NEG_RISK_EXCHANGE);
}

std::string ApprovalHelper::approve_usdc_for_exchange() {
    Transaction tx;
    tx.nonce = rpc_.get_nonce(address_);
    tx.gas_price = rpc_.get_gas_price();
    tx.gas_limit = 100000;
    tx.to = polygon_contracts::USDC;
    tx.value = "0x0";
    tx.data = abi::encode_approve(polygon_contracts::EXCHANGE, 
        "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");  // max uint256
    tx.chain_id = 137;
    
    std::string signed_tx = tx.sign(private_key_);
    std::string tx_hash = rpc_.send_raw_transaction(signed_tx);
    rpc_.wait_for_receipt(tx_hash);
    return tx_hash;
}

std::string ApprovalHelper::approve_usdc_for_neg_risk_exchange() {
    Transaction tx;
    tx.nonce = rpc_.get_nonce(address_);
    tx.gas_price = rpc_.get_gas_price();
    tx.gas_limit = 100000;
    tx.to = polygon_contracts::USDC;
    tx.value = "0x0";
    tx.data = abi::encode_approve(polygon_contracts::NEG_RISK_EXCHANGE,
        "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
    tx.chain_id = 137;
    
    std::string signed_tx = tx.sign(private_key_);
    std::string tx_hash = rpc_.send_raw_transaction(signed_tx);
    rpc_.wait_for_receipt(tx_hash);
    return tx_hash;
}

std::string ApprovalHelper::approve_ctf_for_exchange() {
    Transaction tx;
    tx.nonce = rpc_.get_nonce(address_);
    tx.gas_price = rpc_.get_gas_price();
    tx.gas_limit = 100000;
    tx.to = polygon_contracts::CTF;
    tx.value = "0x0";
    tx.data = abi::encode_set_approval_for_all(polygon_contracts::EXCHANGE, true);
    tx.chain_id = 137;
    
    std::string signed_tx = tx.sign(private_key_);
    std::string tx_hash = rpc_.send_raw_transaction(signed_tx);
    rpc_.wait_for_receipt(tx_hash);
    return tx_hash;
}

std::string ApprovalHelper::approve_ctf_for_neg_risk_exchange() {
    Transaction tx;
    tx.nonce = rpc_.get_nonce(address_);
    tx.gas_price = rpc_.get_gas_price();
    tx.gas_limit = 100000;
    tx.to = polygon_contracts::CTF;
    tx.value = "0x0";
    tx.data = abi::encode_set_approval_for_all(polygon_contracts::NEG_RISK_EXCHANGE, true);
    tx.chain_id = 137;
    
    std::string signed_tx = tx.sign(private_key_);
    std::string tx_hash = rpc_.send_raw_transaction(signed_tx);
    rpc_.wait_for_receipt(tx_hash);
    return tx_hash;
}

bool ApprovalHelper::ensure_approvals() {
    bool all_set = true;
    
    if (!has_usdc_exchange_approval()) {
        all_set = false;
        approve_usdc_for_exchange();
    }
    
    if (!has_usdc_neg_risk_approval()) {
        all_set = false;
        approve_usdc_for_neg_risk_exchange();
    }
    
    if (!has_ctf_exchange_approval()) {
        all_set = false;
        approve_ctf_for_exchange();
    }
    
    if (!has_ctf_neg_risk_approval()) {
        all_set = false;
        approve_ctf_for_neg_risk_exchange();
    }
    
    return all_set;
}

} // namespace clob

