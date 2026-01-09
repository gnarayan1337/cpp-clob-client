#include "clob/client.hpp"
#include "clob/constants.hpp"
#include "clob/utilities.hpp"
#include "clob/eip712.hpp"
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cstring>

namespace clob {

// ========== Constructors ==========

ClobClient::ClobClient(const std::string& host)
    : host_(host.back() == '/' ? host.substr(0, host.length() - 1) : host),
      http_(std::make_unique<HttpClient>(host_)),
      mode_(AuthLevel::L0) {}

ClobClient::ClobClient(const std::string& host, std::shared_ptr<Signer> signer)
    : host_(host.back() == '/' ? host.substr(0, host.length() - 1) : host),
      http_(std::make_unique<HttpClient>(host_)),
      signer_(signer),
      builder_(std::make_unique<OrderBuilder>(signer)),
      mode_(AuthLevel::L1) {}

ClobClient::ClobClient(
    const std::string& host,
    std::shared_ptr<Signer> signer,
    const ApiCreds& creds
) : host_(host.back() == '/' ? host.substr(0, host.length() - 1) : host),
    http_(std::make_unique<HttpClient>(host_)),
    signer_(signer),
    creds_(creds),
    builder_(std::make_unique<OrderBuilder>(signer)),
    mode_(AuthLevel::L2) {}

// ========== Helper Methods ==========

void ClobClient::assert_level_1_auth() const {
    if (mode_ < AuthLevel::L1) {
        throw std::runtime_error("L1 authentication required");
    }
}

void ClobClient::assert_level_2_auth() const {
    if (mode_ < AuthLevel::L2) {
        throw std::runtime_error("L2 authentication required");
    }
}

AuthLevel ClobClient::get_client_mode() const {
    if (signer_ && creds_.has_value()) {
        return AuthLevel::L2;
    }
    if (signer_) {
        return AuthLevel::L1;
    }
    return AuthLevel::L0;
}

std::string ClobClient::get_address() const {
    return signer_ ? signer_->address() : "";
}

std::string ClobClient::get_collateral_address() const {
    if (!signer_) return "";
    auto config = get_contract_config(signer_->get_chain_id());
    return config.collateral;
}

std::string ClobClient::get_conditional_address() const {
    if (!signer_) return "";
    auto config = get_contract_config(signer_->get_chain_id());
    return config.conditional_tokens;
}

std::string ClobClient::get_exchange_address(bool neg_risk) const {
    if (!signer_) return "";
    auto config = get_contract_config(signer_->get_chain_id(), neg_risk);
    return config.exchange;
}

// ========== L1 Header Creation ==========

Headers ClobClient::create_l1_headers(std::optional<uint32_t> nonce) {
    assert_level_1_auth();
    
    // Get timestamp
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()
    ).count();
    
    // Build ClobAuth message
    json clob_auth = {
        {"address", signer_->address()},
        {"timestamp", std::to_string(timestamp)},
        {"nonce", nonce.value_or(0)},
        {"message", "This message attests that I control the given wallet"}
    };
    
    // EIP712 domain
    json domain = {
        {"name", "ClobAuthDomain"},
        {"version", "1"},
        {"chainId", signer_->get_chain_id()}
    };
    
    // EIP712 types
    json types = {
        {"ClobAuth", json::array({
            {{"name", "address"}, {"type", "address"}},
            {{"name", "timestamp"}, {"type", "string"}},
            {{"name", "nonce"}, {"type", "uint256"}},
            {{"name", "message"}, {"type", "string"}}
        })}
    };
    
    // Sign
    std::string signature = signer_->sign_typed_data(domain, "ClobAuth", clob_auth, types);
    
    Headers headers;
    headers["POLY_ADDRESS"] = signer_->address();
    headers["POLY_SIGNATURE"] = signature;
    headers["POLY_TIMESTAMP"] = std::to_string(timestamp);
    headers["POLY_NONCE"] = std::to_string(nonce.value_or(0));
    
    return headers;
}

// ========== L2 Header Creation ==========

Headers ClobClient::create_l2_headers(
    const std::string& method,
    const std::string& request_path,
    const std::string& body
) {
    assert_level_2_auth();
    
    // Get timestamp
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()
    ).count();
    
    // Build message: timestamp + method + path + body
    std::string message = std::to_string(timestamp) + method + request_path + body;
    
    // HMAC-SHA256
    unsigned char hmac_result[32];
    
    // Decode URL-safe base64 secret (API secret uses URL-safe base64: - and _ instead of + and /)
    std::string secret_copy = creds_->api_secret;
    // Convert URL-safe base64 to standard base64
    for (char& c : secret_copy) {
        if (c == '-') c = '+';
        if (c == '_') c = '/';
    }
    // Add padding if needed
    while (secret_copy.length() % 4 != 0) {
        secret_copy += '=';
    }
    
    BIO* bio_decode = BIO_new_mem_buf(secret_copy.data(), secret_copy.length());
    BIO* b64_decode = BIO_new(BIO_f_base64());
    BIO_set_flags(b64_decode, BIO_FLAGS_BASE64_NO_NL);
    bio_decode = BIO_push(b64_decode, bio_decode);
    
    std::vector<uint8_t> decoded_secret(256);  // Max size
    int decoded_len = BIO_read(bio_decode, decoded_secret.data(), 256);
    BIO_free_all(bio_decode);
    
    if (decoded_len <= 0) {
        throw std::runtime_error("Failed to decode API secret");
    }
    decoded_secret.resize(decoded_len);
    
    // Compute HMAC
    unsigned int hmac_len = 32;
    HMAC(EVP_sha256(),
         decoded_secret.data(), decoded_secret.size(),
         reinterpret_cast<const unsigned char*>(message.data()), message.length(),
         hmac_result, &hmac_len);
    
    // Base64 encode the HMAC result (URL-safe base64)
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
    // NOTE: Must keep base64 "=" padding suffix (required by API)
    for (char& c : signature) {
        if (c == '+') c = '-';
        if (c == '/') c = '_';
    }
    
    // Convert address to lowercase (API requires lowercase)
    std::string address = signer_->address();
    std::transform(address.begin(), address.end(), address.begin(), ::tolower);
    
    Headers headers;
    headers["POLY_ADDRESS"] = address;
    headers["POLY_API_KEY"] = creds_->api_key;
    headers["POLY_PASSPHRASE"] = creds_->api_passphrase;
    headers["POLY_SIGNATURE"] = signature;
    headers["POLY_TIMESTAMP"] = std::to_string(timestamp);
    
    return headers;
}

// ========== Public Endpoints (L0) ==========

std::string ClobClient::get_ok() {
    auto response = http_->get("/");
    return response.get<std::string>();
}

Timestamp ClobClient::get_server_time() {
    // Server time returns plain number, not JSON object
    auto elem = http_->get_simd(endpoints::TIME);
    return elem.get_int64().value();
}

Page<MarketResponse> ClobClient::get_markets(const std::string& next_cursor) {
    json params = {{"next_cursor", next_cursor}};
    // Use SIMD JSON for 20x faster parsing
    auto elem = http_->get_simd(endpoints::GET_MARKETS, std::nullopt, params);
    return utils::parse_page_simd<MarketResponse>(elem, utils::parse_market_simd);
}

MarketResponse ClobClient::get_market(const std::string& condition_id) {
    // Use SIMD JSON for 20x faster parsing
    auto elem = http_->get_simd(std::string(endpoints::GET_MARKET) + condition_id);
    return utils::parse_market_simd(elem);
}

OrderBookSummaryResponse ClobClient::get_order_book(const std::string& token_id) {
    json params = {{"token_id", token_id}};
    // Use SIMD JSON for 20x faster parsing
    auto elem = http_->get_simd(endpoints::GET_ORDER_BOOK, std::nullopt, params);
    return utils::parse_orderbook_simd(elem);
}

TickSizeResponse ClobClient::get_tick_size(const std::string& token_id) {
    // Check cache
    auto it = tick_sizes_.find(token_id);
    if (it != tick_sizes_.end()) {
        return it->second;
    }
    
    // Fetch from API
    json params = {{"token_id", token_id}};
    auto elem = http_->get_simd(endpoints::GET_TICK_SIZE, std::nullopt, params);
    auto response = utils::parse_tick_size_simd(elem);
    
    tick_sizes_[token_id] = response;
    
    return response;
}

NegRiskResponse ClobClient::get_neg_risk(const std::string& token_id) {
    // Check cache
    auto it = neg_risk_.find(token_id);
    if (it != neg_risk_.end()) {
        return it->second;
    }
    
    // Fetch from API
    json params = {{"token_id", token_id}};
    auto elem = http_->get_simd(endpoints::GET_NEG_RISK, std::nullopt, params);
    auto response = utils::parse_neg_risk_simd(elem);
    
    neg_risk_[token_id] = response;
    
    return response;
}

FeeRateResponse ClobClient::get_fee_rate_bps(const std::string& token_id) {
    // Check cache
    auto it = fee_rates_.find(token_id);
    if (it != fee_rates_.end()) {
        return it->second;
    }
    
    // Fetch from API
    json params = {{"token_id", token_id}};
    auto elem = http_->get_simd(endpoints::GET_FEE_RATE, std::nullopt, params);
    auto response = utils::parse_fee_rate_simd(elem);
    
    fee_rates_[token_id] = response;
    
    return response;
}

MidpointResponse ClobClient::get_midpoint(const std::string& token_id) {
    json params = {{"token_id", token_id}};
    auto elem = http_->get_simd(endpoints::MID_POINT, std::nullopt, params);
    return utils::parse_midpoint_simd(elem);
}

PriceResponse ClobClient::get_price(const std::string& token_id, Side side) {
    json side_str;
    to_json(side_str, side);
    json params = {{"token_id", token_id}, {"side", side_str}};
    auto elem = http_->get_simd(endpoints::PRICE, std::nullopt, params);
    return utils::parse_price_simd(elem);
}

SpreadResponse ClobClient::get_spread(const std::string& token_id) {
    json params = {{"token_id", token_id}};
    auto elem = http_->get_simd(endpoints::GET_SPREAD, std::nullopt, params);
    return utils::parse_spread_simd(elem);
}

LastTradePriceResponse ClobClient::get_last_trade_price(const std::string& token_id) {
    json params = {{"token_id", token_id}};
    auto elem = http_->get_simd(endpoints::GET_LAST_TRADE_PRICE, std::nullopt, params);
    return utils::parse_last_trade_price_simd(elem);
}

// ========== State Transitions ==========

void ClobClient::deauthenticate() {
    // Clear all authentication state
    signer_.reset();
    creds_.reset();
    builder_.reset();
    
    // Reset to public-only mode
    mode_ = AuthLevel::L0;
    
    // Clear caches (they might contain authenticated data)
    tick_sizes_.clear();
    neg_risk_.clear();
    fee_rates_.clear();
}

// ========== L1 Authenticated Endpoints ==========

ApiCreds ClobClient::create_api_key(std::optional<uint32_t> nonce) {
    assert_level_1_auth();
    
    auto headers = create_l1_headers(nonce);
    auto response = http_->post(endpoints::CREATE_API_KEY, std::nullopt, headers);
    
    ApiCreds creds;
    response.get_to(creds);
    return creds;
}

ApiCreds ClobClient::derive_api_key(std::optional<uint32_t> nonce) {
    assert_level_1_auth();
    
    auto headers = create_l1_headers(nonce);
    auto response = http_->get(endpoints::DERIVE_API_KEY, headers);
    
    ApiCreds creds;
    response.get_to(creds);
    return creds;
}

ApiCreds ClobClient::create_or_derive_api_creds(std::optional<uint32_t> nonce) {
    try {
        return create_api_key(nonce);
    } catch (...) {
        return derive_api_key(nonce);
    }
}

// ========== L2 Authenticated Endpoints ==========

void ClobClient::set_api_creds(const ApiCreds& creds) {
    creds_ = creds;
    mode_ = get_client_mode();
}

ApiKeysResponse ClobClient::get_api_keys() {
    assert_level_2_auth();
    
    auto headers = create_l2_headers("GET", endpoints::GET_API_KEYS);
    auto elem = http_->get_simd(endpoints::GET_API_KEYS, headers);
    return utils::parse_api_keys_simd(elem);
}

SignedOrder ClobClient::create_order(
    const OrderArgs& args,
    const CreateOrderOptions& options
) {
    assert_level_1_auth();
    
    // Validate price
    if (!utils::price_valid(args.price, options.tick_size)) {
        throw std::runtime_error("Invalid price for tick size");
    }
    
    return builder_->create_order(args, options);
}

SignedOrder ClobClient::create_market_order(
    const MarketOrderArgs& args,
    const CreateOrderOptions& options
) {
    assert_level_1_auth();
    return builder_->create_market_order(args, options);
}

PostOrderResponse ClobClient::post_order(const SignedOrder& order, OrderType order_type) {
    assert_level_2_auth();
    
    // Single order uses /order endpoint (not wrapped in array)
    json order_json = utils::order_to_json(order, creds_->api_key, order_type);
    
    std::string body = order_json.dump();
    
    auto headers = create_l2_headers("POST", endpoints::POST_ORDER, body);
    
    // Use SIMD JSON for faster parsing
    auto elem = http_->post_simd(endpoints::POST_ORDER, order_json, headers);
    return utils::parse_post_order_simd(elem);
}

Page<OpenOrderResponse> ClobClient::get_orders(
    const std::optional<OpenOrderParams>& params,
    const std::string& next_cursor
) {
    assert_level_2_auth();
    
    auto headers = create_l2_headers("GET", endpoints::ORDERS);
    json query_params = {{"next_cursor", next_cursor}};
    
    if (params.has_value()) {
        if (params->asset_id.has_value()) {
            query_params["asset_id"] = *params->asset_id;
        }
        if (params->market.has_value()) {
            query_params["market"] = *params->market;
        }
    }
    
    // Use SIMD JSON for faster parsing
    auto elem = http_->get_simd(endpoints::ORDERS, headers, query_params);
    return utils::parse_page_simd<OpenOrderResponse>(elem, utils::parse_open_order_simd);
}

CancelOrdersResponse ClobClient::cancel(const std::string& order_id) {
    assert_level_2_auth();
    
    json data = {{"orderID", order_id}};
    std::string body = data.dump();
    
    auto headers = create_l2_headers("DELETE", endpoints::CANCEL, body);
    auto elem = http_->del_simd(endpoints::CANCEL, data, headers);
    return utils::parse_cancel_orders_simd(elem);
}

CancelOrdersResponse ClobClient::cancel_all() {
    assert_level_2_auth();
    
    auto headers = create_l2_headers("DELETE", endpoints::CANCEL_ALL);
    auto elem = http_->del_simd(endpoints::CANCEL_ALL, std::nullopt, headers);
    return utils::parse_cancel_orders_simd(elem);
}

double ClobClient::calculate_market_price(
    const std::string& token_id,
    Side side,
    double amount,
    OrderType order_type
) {
    auto book = get_order_book(token_id);
    
    if (side == Side::BUY) {
        if (book.asks.empty()) {
            throw std::runtime_error("No match");
        }
        return builder_->calculate_buy_market_price(book.asks, amount, order_type);
    } else {
        if (book.bids.empty()) {
            throw std::runtime_error("No match");
        }
        return builder_->calculate_sell_market_price(book.bids, amount, order_type);
    }
}

OpenOrderResponse ClobClient::get_order(const std::string& order_id) {
    assert_level_2_auth();
    
    std::string path = std::string(endpoints::GET_ORDER) + order_id;
    auto headers = create_l2_headers("GET", path);
    auto elem = http_->get_simd(path, headers);
    return utils::parse_open_order_simd(elem);
}

CancelOrdersResponse ClobClient::cancel_orders(const std::vector<std::string>& order_ids) {
    assert_level_2_auth();
    
    json data = order_ids;
    std::string body = data.dump();
    
    auto headers = create_l2_headers("DELETE", endpoints::CANCEL_ORDERS, body);
    auto elem = http_->del_simd(endpoints::CANCEL_ORDERS, data, headers);
    return utils::parse_cancel_orders_simd(elem);
}

CancelOrdersResponse ClobClient::cancel_market_orders(const std::string& market, const std::string& asset_id) {
    assert_level_2_auth();
    
    json data = {{"market", market}, {"asset_id", asset_id}};
    std::string body = data.dump();
    
    auto headers = create_l2_headers("DELETE", endpoints::CANCEL_MARKET_ORDERS, body);
    auto elem = http_->del_simd(endpoints::CANCEL_MARKET_ORDERS, data, headers);
    return utils::parse_cancel_orders_simd(elem);
}

Page<TradeResponse> ClobClient::get_trades(
    const std::optional<TradeParams>& params,
    const std::string& next_cursor
) {
    assert_level_2_auth();
    
    auto headers = create_l2_headers("GET", endpoints::TRADES);
    json query_params = {{"next_cursor", next_cursor}};
    
    if (params.has_value()) {
        if (params->asset_id.has_value()) {
            query_params["asset_id"] = *params->asset_id;
        }
        if (params->market.has_value()) {
            query_params["market"] = *params->market;
        }
        if (params->before.has_value()) {
            query_params["before"] = *params->before;
        }
        if (params->after.has_value()) {
            query_params["after"] = *params->after;
        }
    }
    
    // Use SIMD JSON for faster parsing
    auto elem = http_->get_simd(endpoints::TRADES, headers, query_params);
    return utils::parse_page_simd<TradeResponse>(elem, utils::parse_trade_simd);
}

BalanceAllowanceResponse ClobClient::get_balance_allowance(const std::optional<BalanceAllowanceParams>& params) {
    assert_level_2_auth();
    
    auto headers = create_l2_headers("GET", endpoints::GET_BALANCE_ALLOWANCE);
    json query_params;
    
    if (params.has_value()) {
        if (params->asset_type.has_value()) {
            query_params["asset_type"] = *params->asset_type;
        }
        int sig_type = params->signature_type >= 0 ? params->signature_type : static_cast<int>(builder_->get_signature_type());
        query_params["signature_type"] = sig_type;
    } else {
        query_params["signature_type"] = static_cast<int>(builder_->get_signature_type());
    }
    
    auto elem = http_->get_simd(endpoints::GET_BALANCE_ALLOWANCE, headers, query_params);
    return utils::parse_balance_allowance_simd(elem);
}

void ClobClient::update_balance_allowance(const std::optional<BalanceAllowanceParams>& params) {
    assert_level_2_auth();
    
    auto headers = create_l2_headers("GET", endpoints::UPDATE_BALANCE_ALLOWANCE);
    json query_params;
    
    if (params.has_value()) {
        if (params->asset_type.has_value()) {
            query_params["asset_type"] = *params->asset_type;
        }
        int sig_type = params->signature_type >= 0 ? params->signature_type : static_cast<int>(builder_->get_signature_type());
        query_params["signature_type"] = sig_type;
    } else {
        query_params["signature_type"] = static_cast<int>(builder_->get_signature_type());
    }
    
    http_->get(endpoints::UPDATE_BALANCE_ALLOWANCE, headers, query_params);
}

std::vector<NotificationResponse> ClobClient::get_notifications() {
    assert_level_2_auth();
    
    auto headers = create_l2_headers("GET", endpoints::GET_NOTIFICATIONS);
    json query_params = {
        {"signature_type", static_cast<int>(builder_->get_signature_type())}
    };
    
    auto elem = http_->get_simd(endpoints::GET_NOTIFICATIONS, headers, query_params);
    auto arr = elem.get_array().value();
    return utils::parse_vector_simd<NotificationResponse>(arr, utils::parse_notification_simd);
}

void ClobClient::drop_notifications() {
    assert_level_2_auth();
    
    auto headers = create_l2_headers("DELETE", endpoints::DROP_NOTIFICATIONS);
    http_->del(endpoints::DROP_NOTIFICATIONS, std::nullopt, headers);
}

OrderScoringResponse ClobClient::is_order_scoring(const std::string& order_id) {
    assert_level_2_auth();
    
    auto headers = create_l2_headers("GET", endpoints::IS_ORDER_SCORING);
    json query_params = {{"order_id", order_id}};
    
    auto elem = http_->get_simd(endpoints::IS_ORDER_SCORING, headers, query_params);
    return utils::parse_order_scoring_simd(elem);
}

OrdersScoringResponse ClobClient::are_orders_scoring(const std::vector<std::string>& order_ids) {
    assert_level_2_auth();
    
    json data = order_ids;
    std::string body = data.dump();
    
    auto headers = create_l2_headers("POST", endpoints::ARE_ORDERS_SCORING, body);
    return http_->post_typed<OrdersScoringResponse>(endpoints::ARE_ORDERS_SCORING, data, headers);
}

Page<SimplifiedMarketResponse> ClobClient::get_simplified_markets(const std::string& next_cursor) {
    json params = {{"next_cursor", next_cursor}};
    auto elem = http_->get_simd(endpoints::GET_SIMPLIFIED_MARKETS, std::nullopt, params);
    return utils::parse_page_simd<SimplifiedMarketResponse>(elem, utils::parse_simplified_market_simd);
}

Page<MarketResponse> ClobClient::get_sampling_markets(const std::string& next_cursor) {
    json params = {{"next_cursor", next_cursor}};
    auto elem = http_->get_simd(endpoints::GET_SAMPLING_MARKETS, std::nullopt, params);
    return utils::parse_page_simd<MarketResponse>(elem, utils::parse_market_simd);
}

Page<SimplifiedMarketResponse> ClobClient::get_sampling_simplified_markets(const std::string& next_cursor) {
    json params = {{"next_cursor", next_cursor}};
    auto elem = http_->get_simd(endpoints::GET_SAMPLING_SIMPLIFIED_MARKETS, std::nullopt, params);
    return utils::parse_page_simd<SimplifiedMarketResponse>(elem, utils::parse_simplified_market_simd);
}

std::vector<OrderBookSummaryResponse> ClobClient::get_order_books(const std::vector<std::string>& token_ids) {
    json body = json::array();
    for (const auto& token_id : token_ids) {
        body.push_back({{"token_id", token_id}});
    }
    
    auto elem = http_->post_simd(endpoints::GET_ORDER_BOOKS, body);
    auto arr = elem.get_array().value();
    return utils::parse_vector_simd<OrderBookSummaryResponse>(arr, utils::parse_orderbook_simd);
}

MidpointsResponse ClobClient::get_midpoints(const std::vector<std::string>& token_ids) {
    json body = json::array();
    for (const auto& token_id : token_ids) {
        body.push_back({{"token_id", token_id}});
    }
    return http_->post_typed<MidpointsResponse>(endpoints::MID_POINTS, body);
}

PricesResponse ClobClient::get_prices(const std::vector<PriceRequest>& requests) {
    json body;
    to_json(body, requests);
    return http_->post_typed<PricesResponse>(endpoints::GET_PRICES, body);
}

SpreadsResponse ClobClient::get_spreads(const std::vector<std::string>& token_ids) {
    json body = json::array();
    for (const auto& token_id : token_ids) {
        body.push_back({{"token_id", token_id}});
    }
    return http_->post_typed<SpreadsResponse>(endpoints::GET_SPREADS, body);
}

std::vector<LastTradesPricesResponse> ClobClient::get_last_trades_prices(const std::vector<std::string>& token_ids) {
    json body = json::array();
    for (const auto& token_id : token_ids) {
        body.push_back({{"token_id", token_id}});
    }
    auto elem = http_->post_simd(endpoints::GET_LAST_TRADES_PRICES, body);
    auto arr = elem.get_array().value();
    return utils::parse_vector_simd<LastTradesPricesResponse>(arr, utils::parse_last_trades_prices_simd);
}

BanStatusResponse ClobClient::get_closed_only_mode() {
    assert_level_2_auth();
    
    auto headers = create_l2_headers("GET", endpoints::CLOSED_ONLY);
    auto elem = http_->get_simd(endpoints::CLOSED_ONLY, headers);
    return utils::parse_ban_status_simd(elem);
}

json ClobClient::delete_api_key() {
    assert_level_2_auth();
    
    auto headers = create_l2_headers("DELETE", endpoints::DELETE_API_KEY);
    return http_->del(endpoints::DELETE_API_KEY, std::nullopt, headers);
}

std::vector<PostOrderResponse> ClobClient::post_orders(const std::vector<std::pair<SignedOrder, OrderType>>& orders) {
    assert_level_2_auth();
    
    json orders_array = json::array();
    for (const auto& [order, order_type] : orders) {
        orders_array.push_back(utils::order_to_json(order, creds_->api_key, order_type));
    }
    
    std::string body = orders_array.dump();
    auto headers = create_l2_headers("POST", endpoints::POST_ORDERS, body);
    
    auto elem = http_->post_simd(endpoints::POST_ORDERS, orders_array, headers);
    auto arr = elem.get_array().value();
    return utils::parse_vector_simd<PostOrderResponse>(arr, utils::parse_post_order_simd);
}

PostOrderResponse ClobClient::create_and_post_order(
    const OrderArgs& args,
    const CreateOrderOptions& options
) {
    auto order = create_order(args, options);
    return post_order(order);
}

TickSizeResponse ClobClient::resolve_tick_size(
    const std::string& token_id,
    const std::optional<std::string>& tick_size
) {
    auto min_tick_size_resp = get_tick_size(token_id);
    
    if (tick_size.has_value()) {
        // Convert TickSize enum to string for comparison
        std::string min_tick_str;
        switch (min_tick_size_resp.minimum_tick_size) {
            case TickSize::TENTH: min_tick_str = "0.1"; break;
            case TickSize::HUNDREDTH: min_tick_str = "0.01"; break;
            case TickSize::THOUSANDTH: min_tick_str = "0.001"; break;
            case TickSize::TEN_THOUSANDTH: min_tick_str = "0.0001"; break;
        }
        
        if (utils::is_tick_size_smaller(*tick_size, min_tick_str)) {
            throw std::runtime_error(
                "Invalid tick size (" + *tick_size + "), minimum for the market is " + min_tick_str
            );
        }
    }
    
    return min_tick_size_resp;
}

FeeRateResponse ClobClient::resolve_fee_rate(
    const std::string& token_id,
    std::optional<uint32_t> user_fee_rate
) {
    auto market_fee_rate_resp = get_fee_rate_bps(token_id);
    
    if (market_fee_rate_resp.base_fee > 0 && user_fee_rate.has_value() && *user_fee_rate != market_fee_rate_resp.base_fee) {
        throw std::runtime_error(
            "Invalid user provided fee rate: " + std::to_string(*user_fee_rate) +
            ", fee rate for the market must be " + std::to_string(market_fee_rate_resp.base_fee)
        );
    }
    
    return market_fee_rate_resp;
}

// ========== Rewards/Earnings API Implementation ==========

Page<UserEarningResponse> ClobClient::get_earnings_for_user_for_day(
    const std::string& date,
    const std::string& next_cursor
) {
    assert_level_2_auth();
    
    std::string path = "/rewards/user";
    std::string query = "?date=" + date;
    if (next_cursor != INITIAL_CURSOR) {
        query += "&next_cursor=" + next_cursor;
    }
    
    auto headers = create_l2_headers("GET", path + query);
    auto elem = http_->get_simd(path + query, headers);
    return utils::parse_page_simd<UserEarningResponse>(elem, utils::parse_user_earning_simd);
}

std::vector<TotalUserEarningResponse> ClobClient::get_total_earnings_for_user_for_day(
    const std::string& date
) {
    assert_level_2_auth();
    
    std::string path = "/rewards/user/total?date=" + date;
    auto headers = create_l2_headers("GET", path);
    auto elem = http_->get_simd(path, headers);
    auto arr = elem.get_array().value();
    return utils::parse_vector_simd<TotalUserEarningResponse>(arr, utils::parse_total_user_earning_simd);
}

std::vector<UserRewardsEarningResponse> ClobClient::get_user_earnings_and_markets_config(
    const UserRewardsEarningRequest& request
) {
    assert_level_2_auth();
    
    std::string path = "/rewards/user/total";
    std::string query = "?start_date=" + request.start_date + 
                       "&end_date=" + request.end_date;
    
    auto headers = create_l2_headers("GET", path + query);
    return http_->get_typed<std::vector<UserRewardsEarningResponse>>(path + query, headers);
}

RewardsPercentagesResponse ClobClient::get_reward_percentages() {
    assert_level_2_auth();
    
    std::string path = "/rewards/user/percentages";
    auto headers = create_l2_headers("GET", path);
    auto elem = http_->get_simd(path, headers);
    return utils::parse_rewards_percentages_simd(elem);
}

Page<CurrentRewardResponse> ClobClient::get_current_rewards(
    const std::string& next_cursor
) {
    assert_level_2_auth();
    
    std::string path = "/rewards/markets/current";
    std::string query = next_cursor != INITIAL_CURSOR ? "?next_cursor=" + next_cursor : "";
    
    auto headers = create_l2_headers("GET", path + query);
    auto elem = http_->get_simd(path + query, headers);
    return utils::parse_page_simd<CurrentRewardResponse>(elem, utils::parse_current_reward_simd);
}

Page<MarketRewardResponse> ClobClient::get_raw_rewards_for_market(
    const std::string& condition_id,
    const std::string& next_cursor
) {
    assert_level_2_auth();
    
    std::string path = "/rewards/markets/" + condition_id;
    std::string query = next_cursor != INITIAL_CURSOR ? "?next_cursor=" + next_cursor : "";
    
    auto headers = create_l2_headers("GET", path + query);
    auto elem = http_->get_simd(path + query, headers);
    return utils::parse_page_simd<MarketRewardResponse>(elem, utils::parse_market_reward_simd);
}

// ========== Low-Latency Optimization Methods ==========

bool ClobClient::warm_connection() {
    return http_->warm_connection();
}

void ClobClient::start_heartbeat(int interval_seconds) {
    http_->start_heartbeat(interval_seconds);
}

void ClobClient::stop_heartbeat() {
    http_->stop_heartbeat();
}

bool ClobClient::is_heartbeat_running() const {
    return http_->is_heartbeat_running();
}

ConnectionStats ClobClient::get_connection_stats() const {
    return http_->get_stats();
}

} // namespace clob






