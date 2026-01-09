#pragma once

#include <string>
#include <optional>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <chrono>
#include <nlohmann/json.hpp>

namespace clob {

using json = nlohmann::json;
using Timestamp = int64_t;
using Decimal = std::string; // Using string for arbitrary precision decimals

// ==================== Chain IDs ====================

constexpr uint64_t POLYGON = 137;
constexpr uint64_t AMOY = 80002;

// ==================== Enums ====================

enum class AuthLevel {
    L0 = 0,  // Public endpoints
    L1 = 1,  // Signature required
    L2 = 2   // API key required
};

enum class Side : uint8_t {
    BUY = 0,
    SELL = 1,
    UNKNOWN = 255
};

NLOHMANN_JSON_SERIALIZE_ENUM(Side, {
    {Side::BUY, "BUY"},
    {Side::SELL, "SELL"},
    {Side::UNKNOWN, nullptr},
})

enum class OrderType {
    GTC,  // Good Till Cancel
    FOK,  // Fill Or Kill
    GTD,  // Good Till Date
    FAK,  // Fill And Kill
    UNKNOWN
};

NLOHMANN_JSON_SERIALIZE_ENUM(OrderType, {
    {OrderType::GTC, "GTC"},
    {OrderType::FOK, "FOK"},
    {OrderType::GTD, "GTD"},
    {OrderType::FAK, "FAK"},
    {OrderType::GTC, "gtc"},
    {OrderType::FOK, "fok"},
    {OrderType::GTD, "gtd"},
    {OrderType::FAK, "fak"},
    {OrderType::UNKNOWN, nullptr},
})

enum class SignatureType : uint8_t {
    EOA = 0,
    PROXY = 1,
    GNOSIS_SAFE = 2
};

enum class OrderStatusType {
    LIVE,
    MATCHED,
    CANCELED,
    DELAYED,
    UNMATCHED,
    UNKNOWN
};

NLOHMANN_JSON_SERIALIZE_ENUM(OrderStatusType, {
    {OrderStatusType::LIVE, "LIVE"},
    {OrderStatusType::MATCHED, "MATCHED"},
    {OrderStatusType::CANCELED, "CANCELED"},
    {OrderStatusType::DELAYED, "DELAYED"},
    {OrderStatusType::UNMATCHED, "UNMATCHED"},
    {OrderStatusType::LIVE, "live"},
    {OrderStatusType::MATCHED, "matched"},
    {OrderStatusType::CANCELED, "canceled"},
    {OrderStatusType::DELAYED, "delayed"},
    {OrderStatusType::UNMATCHED, "unmatched"},
    {OrderStatusType::UNKNOWN, nullptr},
})

enum class AssetType {
    COLLATERAL,
    CONDITIONAL,
    UNKNOWN
};

NLOHMANN_JSON_SERIALIZE_ENUM(AssetType, {
    {AssetType::COLLATERAL, "COLLATERAL"},
    {AssetType::CONDITIONAL, "CONDITIONAL"},
    {AssetType::UNKNOWN, nullptr},
})

enum class TraderSide {
    TAKER,
    MAKER,
    UNKNOWN
};

NLOHMANN_JSON_SERIALIZE_ENUM(TraderSide, {
    {TraderSide::TAKER, "TAKER"},
    {TraderSide::MAKER, "MAKER"},
    {TraderSide::UNKNOWN, nullptr},
})

enum class TickSize {
    TENTH,       // 0.1
    HUNDREDTH,   // 0.01
    THOUSANDTH,  // 0.001
    TEN_THOUSANDTH // 0.0001
};

// ==================== Basic Structures ====================

struct ApiCreds {
    std::string api_key;
    std::string api_secret;
    std::string api_passphrase;
};

inline void to_json(json& j, const ApiCreds& creds) {
    j = json{
        {"apiKey", creds.api_key},
        {"secret", creds.api_secret},
        {"passphrase", creds.api_passphrase}
    };
}

inline void from_json(const json& j, ApiCreds& creds) {
    j.at("apiKey").get_to(creds.api_key);
    j.at("secret").get_to(creds.api_secret);
    j.at("passphrase").get_to(creds.api_passphrase);
}

struct ContractConfig {
    std::string exchange;
    std::string collateral;
    std::string conditional_tokens;
};

// ==================== Order Structures ====================

struct Order {
    std::string salt;
    std::string maker;
    std::string signer;
    std::string taker;
    std::string token_id;
    std::string maker_amount;
    std::string taker_amount;
    std::string expiration;
    std::string nonce;
    std::string fee_rate_bps;
    uint8_t side;
    uint8_t signature_type;
};

inline void to_json(json& j, const Order& order) {
    j = json{
        {"salt", std::stoull(order.salt)},  // salt as number
        {"maker", order.maker},
        {"signer", order.signer},
        {"taker", order.taker},
        {"tokenId", order.token_id},
        {"makerAmount", order.maker_amount},
        {"takerAmount", order.taker_amount},
        {"expiration", order.expiration},
        {"nonce", order.nonce},
        {"feeRateBps", order.fee_rate_bps},
        {"side", order.side},
        {"signatureType", order.signature_type}
    };
}

struct SignableOrder {
    Order order;
    OrderType order_type;
};

struct SignedOrder {
    Order order;
    std::string signature;
    OrderType order_type;
    std::string owner;  // ApiKey (UUID as string)
};

inline void to_json(json& j, const SignedOrder& signed_order) {
    json order_json;
    to_json(order_json, signed_order.order);
    order_json["signature"] = signed_order.signature;
    
    // Convert side to string
    if (signed_order.order.side == 0) {
        order_json["side"] = "BUY";
    } else if (signed_order.order.side == 1) {
        order_json["side"] = "SELL";
    }
    
    j = json{
        {"order", order_json},
        {"orderType", signed_order.order_type},
        {"owner", signed_order.owner}
    };
}

struct OrderSummary {
    Decimal price;
    Decimal size;
};

inline void from_json(const json& j, OrderSummary& summary) {
    j.at("price").get_to(summary.price);
    j.at("size").get_to(summary.size);
}

inline void to_json(json& j, const OrderSummary& summary) {
    j = json{
        {"price", summary.price},
        {"size", summary.size}
    };
}

// ==================== Request Structures ====================

struct MidpointRequest {
    std::string token_id;
};

inline void to_json(json& j, const MidpointRequest& req) {
    j = json{{"token_id", req.token_id}};
}

struct PriceRequest {
    std::string token_id;
    Side side;
};

inline void to_json(json& j, const PriceRequest& req) {
    j = json{
        {"token_id", req.token_id},
        {"side", req.side}
    };
}

struct SpreadRequest {
    std::string token_id;
};

inline void to_json(json& j, const SpreadRequest& req) {
    j = json{{"token_id", req.token_id}};
}

struct OrderBookSummaryRequest {
    std::string token_id;
};

inline void to_json(json& j, const OrderBookSummaryRequest& req) {
    j = json{{"token_id", req.token_id}};
}

struct LastTradePriceRequest {
    std::string token_id;
};

inline void to_json(json& j, const LastTradePriceRequest& req) {
    j = json{{"token_id", req.token_id}};
}

struct TradeParams {
    std::optional<std::string> asset_id;
    std::optional<std::string> market;
    std::optional<uint64_t> before;
    std::optional<uint64_t> after;
};

struct OpenOrderParams {
    std::optional<std::string> asset_id;
    std::optional<std::string> market;
};

struct BalanceAllowanceParams {
    std::optional<std::string> asset_type;
    int signature_type = -1;
};

struct CancelMarketOrderRequest {
    std::optional<std::string> market;
    std::optional<std::string> asset_id;
};

inline void to_json(json& j, const CancelMarketOrderRequest& req) {
    j = json::object();
    if (req.market) j["market"] = *req.market;
    if (req.asset_id) j["asset_id"] = *req.asset_id;
}

struct OrdersRequest {
    std::optional<std::string> order_id;
    std::optional<std::string> market;
    std::optional<std::string> asset_id;
};

struct TradesRequest {
    std::optional<std::string> id;
    std::optional<std::string> maker_address;
    std::optional<std::string> market;
    std::optional<std::string> asset_id;
    std::optional<int64_t> before;
    std::optional<int64_t> after;
};

struct BalanceAllowanceRequest {
    AssetType asset_type;
    std::optional<std::string> token_id;
    std::optional<SignatureType> signature_type;
};

struct DeleteNotificationsRequest {
    std::optional<std::vector<std::string>> notification_ids;
};

// ==================== Response Structures ====================

struct MidpointResponse {
    Decimal mid;
};

inline void from_json(const json& j, MidpointResponse& resp) {
    j.at("mid").get_to(resp.mid);
}

struct MidpointsResponse {
    std::unordered_map<std::string, Decimal> midpoints;
};

inline void from_json(const json& j, MidpointsResponse& resp) {
    resp.midpoints = j.get<std::unordered_map<std::string, Decimal>>();
}

struct PriceResponse {
    Decimal price;
};

inline void from_json(const json& j, PriceResponse& resp) {
    j.at("price").get_to(resp.price);
}

struct PricesResponse {
    std::optional<std::unordered_map<std::string, std::unordered_map<Side, Decimal>>> prices;
};

inline void from_json(const json& j, PricesResponse& resp) {
    if (!j.is_null() && j.is_object()) {
        std::unordered_map<std::string, std::unordered_map<Side, Decimal>> prices_map;
        for (auto& [token_id, sides_obj] : j.items()) {
            if (sides_obj.is_object()) {
                std::unordered_map<Side, Decimal> sides_map;
                for (auto& [side_str, price] : sides_obj.items()) {
                    Side side = (side_str == "BUY" || side_str == "buy") ? Side::BUY : Side::SELL;
                    sides_map[side] = price.get<Decimal>();
                }
                prices_map[token_id] = sides_map;
            }
        }
        resp.prices = prices_map;
    }
}

struct SpreadResponse {
    Decimal spread;
};

inline void from_json(const json& j, SpreadResponse& resp) {
    j.at("spread").get_to(resp.spread);
}

struct SpreadsResponse {
    std::optional<std::unordered_map<std::string, Decimal>> spreads;
};

inline void from_json(const json& j, SpreadsResponse& resp) {
    if (!j.is_null()) {
        resp.spreads = j.get<std::unordered_map<std::string, Decimal>>();
    }
}

struct TickSizeResponse {
    TickSize minimum_tick_size;
};

inline void from_json(const json& j, TickSizeResponse& resp) {
    // Parse tick_size - can be number or string
    double tick_size_val;
    if (j["minimum_tick_size"].is_number()) {
        tick_size_val = j.at("minimum_tick_size").get<double>();
    } else {
        tick_size_val = std::stod(j.at("minimum_tick_size").get<std::string>());
    }
    
    if (tick_size_val == 0.1) resp.minimum_tick_size = TickSize::TENTH;
    else if (tick_size_val == 0.01) resp.minimum_tick_size = TickSize::HUNDREDTH;
    else if (tick_size_val == 0.001) resp.minimum_tick_size = TickSize::THOUSANDTH;
    else if (tick_size_val == 0.0001) resp.minimum_tick_size = TickSize::TEN_THOUSANDTH;
    else resp.minimum_tick_size = TickSize::HUNDREDTH;  // Default
}

struct NegRiskResponse {
    bool neg_risk;
};

inline void from_json(const json& j, NegRiskResponse& resp) {
    j.at("neg_risk").get_to(resp.neg_risk);
}

struct FeeRateResponse {
    uint32_t base_fee;
};

inline void from_json(const json& j, FeeRateResponse& resp) {
    j.at("base_fee").get_to(resp.base_fee);
}

struct OrderBookSummaryResponse {
    std::string market;
    std::string asset_id;
    std::string timestamp;  // API returns timestamp as string
    std::optional<std::string> hash;
    std::vector<OrderSummary> bids;
    std::vector<OrderSummary> asks;
    Decimal min_order_size;
    bool neg_risk;
    TickSize tick_size;
};

inline void from_json(const json& j, OrderBookSummaryResponse& resp) {
    j.at("market").get_to(resp.market);
    j.at("asset_id").get_to(resp.asset_id);
    j.at("timestamp").get_to(resp.timestamp);
    
    if (j.contains("hash") && !j["hash"].is_null()) {
        resp.hash = j["hash"].get<std::string>();
    }
    
    if (j.contains("bids") && !j["bids"].is_null()) {
        resp.bids = j["bids"].get<std::vector<OrderSummary>>();
    }
    
    if (j.contains("asks") && !j["asks"].is_null()) {
        resp.asks = j["asks"].get<std::vector<OrderSummary>>();
    }
    
    j.at("min_order_size").get_to(resp.min_order_size);
    j.at("neg_risk").get_to(resp.neg_risk);
    
    // Parse tick_size from decimal
    Decimal tick_size_str = j.at("tick_size").get<Decimal>();
    if (tick_size_str == "0.1") resp.tick_size = TickSize::TENTH;
    else if (tick_size_str == "0.01") resp.tick_size = TickSize::HUNDREDTH;
    else if (tick_size_str == "0.001") resp.tick_size = TickSize::THOUSANDTH;
    else if (tick_size_str == "0.0001") resp.tick_size = TickSize::TEN_THOUSANDTH;
}

struct LastTradePriceResponse {
    Decimal price;
    Side side;
};

inline void from_json(const json& j, LastTradePriceResponse& resp) {
    j.at("price").get_to(resp.price);
    j.at("side").get_to(resp.side);
}

struct LastTradesPricesResponse {
    std::string token_id;
    Decimal price;
    Side side;
};

inline void from_json(const json& j, LastTradesPricesResponse& resp) {
    j.at("token_id").get_to(resp.token_id);
    j.at("price").get_to(resp.price);
    j.at("side").get_to(resp.side);
}

struct Token {
    std::string token_id;
    std::string outcome;
    double price;
    bool winner = false;
};

inline void from_json(const json& j, Token& token) {
    j.at("token_id").get_to(token.token_id);
    j.at("outcome").get_to(token.outcome);
    j.at("price").get_to(token.price);
    if (j.contains("winner")) {
        j.at("winner").get_to(token.winner);
    }
}

inline void to_json(json& j, const Token& token) {
    j = json{
        {"token_id", token.token_id},
        {"outcome", token.outcome},
        {"price", token.price},
        {"winner", token.winner}
    };
}

struct RewardRate {
    std::string asset_address;
    double rewards_daily_rate;
};

inline void from_json(const json& j, RewardRate& rate) {
    j.at("asset_address").get_to(rate.asset_address);
    j.at("rewards_daily_rate").get_to(rate.rewards_daily_rate);
}

inline void to_json(json& j, const RewardRate& rate) {
    j = json{
        {"asset_address", rate.asset_address},
        {"rewards_daily_rate", rate.rewards_daily_rate}
    };
}

struct Rewards {
    std::vector<RewardRate> rates;
    double min_size;
    double max_spread;
};

inline void from_json(const json& j, Rewards& rewards) {
    if (j.contains("rates") && !j["rates"].is_null()) {
        rewards.rates = j["rates"].get<std::vector<RewardRate>>();
    }
    j.at("min_size").get_to(rewards.min_size);
    j.at("max_spread").get_to(rewards.max_spread);
}

inline void to_json(json& j, const Rewards& rewards) {
    j = json{
        {"rates", rewards.rates},
        {"min_size", rewards.min_size},
        {"max_spread", rewards.max_spread}
    };
}

struct MarketResponse {
    bool enable_order_book;
    bool active;
    bool closed;
    bool archived;
    bool accepting_orders;
    std::optional<std::string> accepting_order_timestamp;  // ISO string
    double minimum_order_size;  // Numeric
    double minimum_tick_size;   // Numeric
    std::string condition_id;
    std::string question_id;
    std::string question;
    std::string description;
    std::string market_slug;
    std::optional<std::string> end_date_iso;  // ISO string
    std::optional<std::string> game_start_time;  // ISO string
    uint64_t seconds_delay;
    std::string fpmm;
    double maker_base_fee;  // Numeric
    double taker_base_fee;  // Numeric
    bool notifications_enabled;
    bool neg_risk;
    std::string neg_risk_market_id;
    std::string neg_risk_request_id;
    std::string icon;
    std::string image;
    Rewards rewards;
    bool is_50_50_outcome;
    std::vector<Token> tokens;
    std::vector<std::string> tags;
};

inline void from_json(const json& j, MarketResponse& market) {
    j.at("enable_order_book").get_to(market.enable_order_book);
    j.at("active").get_to(market.active);
    j.at("closed").get_to(market.closed);
    j.at("archived").get_to(market.archived);
    j.at("accepting_orders").get_to(market.accepting_orders);
    
    if (j.contains("accepting_order_timestamp") && !j["accepting_order_timestamp"].is_null()) {
        market.accepting_order_timestamp = j["accepting_order_timestamp"].get<std::string>();
    }
    
    j.at("minimum_order_size").get_to(market.minimum_order_size);
    j.at("minimum_tick_size").get_to(market.minimum_tick_size);
    j.at("condition_id").get_to(market.condition_id);
    j.at("question_id").get_to(market.question_id);
    j.at("question").get_to(market.question);
    j.at("description").get_to(market.description);
    j.at("market_slug").get_to(market.market_slug);
    
    if (j.contains("end_date_iso") && !j["end_date_iso"].is_null()) {
        market.end_date_iso = j["end_date_iso"].get<std::string>();
    }
    
    if (j.contains("game_start_time") && !j["game_start_time"].is_null()) {
        market.game_start_time = j["game_start_time"].get<std::string>();
    }
    
    j.at("seconds_delay").get_to(market.seconds_delay);
    j.at("fpmm").get_to(market.fpmm);
    j.at("maker_base_fee").get_to(market.maker_base_fee);
    j.at("taker_base_fee").get_to(market.taker_base_fee);
    j.at("notifications_enabled").get_to(market.notifications_enabled);
    j.at("neg_risk").get_to(market.neg_risk);
    j.at("neg_risk_market_id").get_to(market.neg_risk_market_id);
    j.at("neg_risk_request_id").get_to(market.neg_risk_request_id);
    j.at("icon").get_to(market.icon);
    j.at("image").get_to(market.image);
    j.at("rewards").get_to(market.rewards);
    j.at("is_50_50_outcome").get_to(market.is_50_50_outcome);
    
    if (j.contains("tokens") && !j["tokens"].is_null()) {
        market.tokens = j["tokens"].get<std::vector<Token>>();
    }
    
    if (j.contains("tags") && !j["tags"].is_null()) {
        market.tags = j["tags"].get<std::vector<std::string>>();
    }
}

struct SimplifiedMarketResponse {
    std::string condition_id;
    std::vector<Token> tokens;
    Rewards rewards;
    bool active;
    bool closed;
    bool archived;
    bool accepting_orders;
};

inline void from_json(const json& j, SimplifiedMarketResponse& market) {
    j.at("condition_id").get_to(market.condition_id);
    
    if (j.contains("tokens") && !j["tokens"].is_null()) {
        market.tokens = j["tokens"].get<std::vector<Token>>();
    }
    
    j.at("rewards").get_to(market.rewards);
    j.at("active").get_to(market.active);
    j.at("closed").get_to(market.closed);
    j.at("archived").get_to(market.archived);
    j.at("accepting_orders").get_to(market.accepting_orders);
}

struct ApiKeysResponse {
    std::optional<std::vector<std::string>> keys;
};

inline void from_json(const json& j, ApiKeysResponse& resp) {
    if (j.contains("apiKeys") && !j["apiKeys"].is_null()) {
        resp.keys = j["apiKeys"].get<std::vector<std::string>>();
    }
}

struct BanStatusResponse {
    bool closed_only;
};

inline void from_json(const json& j, BanStatusResponse& resp) {
    j.at("closed_only").get_to(resp.closed_only);
}

struct PostOrderResponse {
    std::optional<std::string> error_msg;
    Decimal making_amount;
    Decimal taking_amount;
    std::string order_id;
    OrderStatusType status;
    bool success;
    std::vector<std::string> transaction_hashes;
    std::vector<std::string> trade_ids;
};

inline void from_json(const json& j, PostOrderResponse& resp) {
    if (j.contains("error_msg") && !j["error_msg"].is_null()) {
        resp.error_msg = j["error_msg"].get<std::string>();
    }
    
    // Handle empty strings as zero
    if (j.contains("making_amount")) {
        auto making = j["making_amount"].get<std::string>();
        resp.making_amount = making.empty() ? "0" : making;
    }
    
    if (j.contains("taking_amount")) {
        auto taking = j["taking_amount"].get<std::string>();
        resp.taking_amount = taking.empty() ? "0" : taking;
    }
    
    j.at("orderID").get_to(resp.order_id);
    j.at("status").get_to(resp.status);
    j.at("success").get_to(resp.success);
    
    if (j.contains("transaction_hashes") && !j["transaction_hashes"].is_null()) {
        resp.transaction_hashes = j["transaction_hashes"].get<std::vector<std::string>>();
    }
    
    if (j.contains("trade_ids") && !j["trade_ids"].is_null()) {
        resp.trade_ids = j["trade_ids"].get<std::vector<std::string>>();
    }
}

struct OpenOrderResponse {
    std::string id;
    OrderStatusType status;
    std::string owner;  // ApiKey (UUID)
    std::string maker_address;
    std::string market;
    std::string asset_id;
    Side side;
    Decimal original_size;
    Decimal size_matched;
    Decimal price;
    std::vector<std::string> associate_trades;
    std::string outcome;
    Timestamp created_at;
    Timestamp expiration;
    OrderType order_type;
};

inline void from_json(const json& j, OpenOrderResponse& resp) {
    j.at("id").get_to(resp.id);
    j.at("status").get_to(resp.status);
    j.at("owner").get_to(resp.owner);
    j.at("maker_address").get_to(resp.maker_address);
    j.at("market").get_to(resp.market);
    j.at("asset_id").get_to(resp.asset_id);
    j.at("side").get_to(resp.side);
    j.at("original_size").get_to(resp.original_size);
    j.at("size_matched").get_to(resp.size_matched);
    j.at("price").get_to(resp.price);
    
    if (j.contains("associate_trades") && !j["associate_trades"].is_null()) {
        resp.associate_trades = j["associate_trades"].get<std::vector<std::string>>();
    }
    
    j.at("outcome").get_to(resp.outcome);
    j.at("created_at").get_to(resp.created_at);
    j.at("expiration").get_to(resp.expiration);
    j.at("order_type").get_to(resp.order_type);
}

struct CancelOrdersResponse {
    std::vector<std::string> canceled;
    std::unordered_map<std::string, std::string> not_canceled;
};

inline void from_json(const json& j, CancelOrdersResponse& resp) {
    if (j.contains("canceled") && !j["canceled"].is_null()) {
        resp.canceled = j["canceled"].get<std::vector<std::string>>();
    }
    
    if (j.contains("notCanceled") && !j["notCanceled"].is_null()) {
        resp.not_canceled = j["notCanceled"].get<std::unordered_map<std::string, std::string>>();
    }
}

struct MakerOrder {
    std::string order_id;
    std::string owner;  // ApiKey (UUID)
    std::string maker_address;
    Decimal matched_amount;
    Decimal price;
    Decimal fee_rate_bps;
    std::string asset_id;
    std::string outcome;
    Side side;
};

inline void from_json(const json& j, MakerOrder& order) {
    j.at("order_id").get_to(order.order_id);
    j.at("owner").get_to(order.owner);
    j.at("maker_address").get_to(order.maker_address);
    j.at("matched_amount").get_to(order.matched_amount);
    j.at("price").get_to(order.price);
    j.at("fee_rate_bps").get_to(order.fee_rate_bps);
    j.at("asset_id").get_to(order.asset_id);
    j.at("outcome").get_to(order.outcome);
    j.at("side").get_to(order.side);
}

struct TradeResponse {
    std::string id;
    std::string taker_order_id;
    std::string market;
    std::string asset_id;
    Side side;
    Decimal size;
    Decimal fee_rate_bps;
    Decimal price;
    OrderStatusType status;
    Timestamp match_time;
    Timestamp last_update;
    std::string outcome;
    uint32_t bucket_index;
    std::string owner;  // ApiKey (UUID)
    std::string maker_address;
    std::vector<MakerOrder> maker_orders;
    std::string transaction_hash;
    TraderSide trader_side;
    std::optional<std::string> error_msg;
};

inline void from_json(const json& j, TradeResponse& resp) {
    j.at("id").get_to(resp.id);
    j.at("taker_order_id").get_to(resp.taker_order_id);
    j.at("market").get_to(resp.market);
    j.at("asset_id").get_to(resp.asset_id);
    j.at("side").get_to(resp.side);
    j.at("size").get_to(resp.size);
    j.at("fee_rate_bps").get_to(resp.fee_rate_bps);
    j.at("price").get_to(resp.price);
    j.at("status").get_to(resp.status);
    j.at("match_time").get_to(resp.match_time);
    j.at("last_update").get_to(resp.last_update);
    j.at("outcome").get_to(resp.outcome);
    j.at("bucket_index").get_to(resp.bucket_index);
    j.at("owner").get_to(resp.owner);
    j.at("maker_address").get_to(resp.maker_address);
    
    if (j.contains("maker_orders") && !j["maker_orders"].is_null()) {
        resp.maker_orders = j["maker_orders"].get<std::vector<MakerOrder>>();
    }
    
    j.at("transaction_hash").get_to(resp.transaction_hash);
    j.at("trader_side").get_to(resp.trader_side);
    
    if (j.contains("error_msg") && !j["error_msg"].is_null()) {
        resp.error_msg = j["error_msg"].get<std::string>();
    }
}

struct NotificationPayload {
    std::string asset_id;
    std::string condition_id;
    std::string event_slug;
    std::string icon;
    std::string image;
    std::string market;
    std::string market_slug;
    Decimal matched_size;
    std::string order_id;
    Decimal original_size;
    std::string outcome;
    uint64_t outcome_index;
    std::string owner;  // ApiKey (UUID)
    Decimal price;
    std::string question;
    Decimal remaining_size;
    std::string series_slug;
    Side side;
    std::string trade_id;
    std::string transaction_hash;
    OrderType order_type;
};

inline void from_json(const json& j, NotificationPayload& payload) {
    j.at("asset_id").get_to(payload.asset_id);
    j.at("condition_id").get_to(payload.condition_id);
    j.at("eventSlug").get_to(payload.event_slug);
    j.at("icon").get_to(payload.icon);
    j.at("image").get_to(payload.image);
    j.at("market").get_to(payload.market);
    j.at("market_slug").get_to(payload.market_slug);
    j.at("matched_size").get_to(payload.matched_size);
    j.at("order_id").get_to(payload.order_id);
    j.at("original_size").get_to(payload.original_size);
    j.at("outcome").get_to(payload.outcome);
    j.at("outcome_index").get_to(payload.outcome_index);
    j.at("owner").get_to(payload.owner);
    j.at("price").get_to(payload.price);
    j.at("question").get_to(payload.question);
    j.at("remaining_size").get_to(payload.remaining_size);
    j.at("seriesSlug").get_to(payload.series_slug);
    j.at("side").get_to(payload.side);
    j.at("trade_id").get_to(payload.trade_id);
    j.at("transaction_hash").get_to(payload.transaction_hash);
    
    if (j.contains("type")) {
        j.at("type").get_to(payload.order_type);
    }
}

inline void to_json(json& j, const NotificationPayload& payload) {
    j = json{
        {"asset_id", payload.asset_id},
        {"condition_id", payload.condition_id},
        {"eventSlug", payload.event_slug},
        {"icon", payload.icon},
        {"image", payload.image},
        {"market", payload.market},
        {"market_slug", payload.market_slug},
        {"matched_size", payload.matched_size},
        {"order_id", payload.order_id},
        {"original_size", payload.original_size},
        {"outcome", payload.outcome},
        {"outcome_index", payload.outcome_index},
        {"owner", payload.owner},
        {"price", payload.price},
        {"question", payload.question},
        {"remaining_size", payload.remaining_size},
        {"seriesSlug", payload.series_slug},
        {"side", payload.side},
        {"trade_id", payload.trade_id},
        {"transaction_hash", payload.transaction_hash},
        {"type", payload.order_type}
    };
}

struct NotificationResponse {
    uint32_t type;
    std::string owner;  // ApiKey (UUID)
    NotificationPayload payload;
};

inline void from_json(const json& j, NotificationResponse& resp) {
    j.at("type").get_to(resp.type);
    j.at("owner").get_to(resp.owner);
    j.at("payload").get_to(resp.payload);
}

struct BalanceAllowanceResponse {
    Decimal balance;
    std::unordered_map<std::string, std::string> allowances;
};

inline void from_json(const json& j, BalanceAllowanceResponse& resp) {
    j.at("balance").get_to(resp.balance);
    
    if (j.contains("allowances") && !j["allowances"].is_null()) {
        resp.allowances = j["allowances"].get<std::unordered_map<std::string, std::string>>();
    }
}

struct OrderScoringResponse {
    bool scoring;
};

inline void from_json(const json& j, OrderScoringResponse& resp) {
    j.at("scoring").get_to(resp.scoring);
}

using OrdersScoringResponse = std::unordered_map<std::string, bool>;

struct BuilderApiKeyResponse {
    std::string key;  // ApiKey (UUID)
    std::optional<Timestamp> created_at;
    std::optional<Timestamp> revoked_at;
};

inline void from_json(const json& j, BuilderApiKeyResponse& resp) {
    j.at("key").get_to(resp.key);
    
    if (j.contains("created_at") && !j["created_at"].is_null()) {
        resp.created_at = j["created_at"].get<Timestamp>();
    }
    
    if (j.contains("revoked_at") && !j["revoked_at"].is_null()) {
        resp.revoked_at = j["revoked_at"].get<Timestamp>();
    }
}

struct BuilderTradeResponse {
    std::string id;
    std::string trade_type;
    std::string taker_order_hash;
    std::string builder;
    std::string market;
    std::string asset_id;
    Side side;
    Decimal size;
    Decimal size_usdc;
    Decimal price;
    OrderStatusType status;
    std::string outcome;
    uint32_t outcome_index;
    std::string owner;  // ApiKey (UUID)
    std::string maker;
    std::string transaction_hash;
    Timestamp match_time;
    uint32_t bucket_index;
    Decimal fee;
    Decimal fee_usdc;
    std::optional<std::string> err_msg;
    std::optional<Timestamp> created_at;
    std::optional<Timestamp> updated_at;
};

inline void from_json(const json& j, BuilderTradeResponse& resp) {
    j.at("id").get_to(resp.id);
    j.at("trade_type").get_to(resp.trade_type);
    j.at("taker_order_hash").get_to(resp.taker_order_hash);
    j.at("builder").get_to(resp.builder);
    j.at("market").get_to(resp.market);
    j.at("asset_id").get_to(resp.asset_id);
    j.at("side").get_to(resp.side);
    j.at("size").get_to(resp.size);
    j.at("size_usdc").get_to(resp.size_usdc);
    j.at("price").get_to(resp.price);
    j.at("status").get_to(resp.status);
    j.at("outcome").get_to(resp.outcome);
    j.at("outcome_index").get_to(resp.outcome_index);
    j.at("owner").get_to(resp.owner);
    j.at("maker").get_to(resp.maker);
    j.at("transaction_hash").get_to(resp.transaction_hash);
    j.at("match_time").get_to(resp.match_time);
    j.at("bucket_index").get_to(resp.bucket_index);
    j.at("fee").get_to(resp.fee);
    j.at("fee_usdc").get_to(resp.fee_usdc);
    
    if (j.contains("err_msg") && !j["err_msg"].is_null()) {
        resp.err_msg = j["err_msg"].get<std::string>();
    } else if (j.contains("error_msg") && !j["error_msg"].is_null()) {
        resp.err_msg = j["error_msg"].get<std::string>();
    }
    
    if (j.contains("created_at") && !j["created_at"].is_null()) {
        resp.created_at = j["created_at"].get<Timestamp>();
    }
    
    if (j.contains("updated_at") && !j["updated_at"].is_null()) {
        resp.updated_at = j["updated_at"].get<Timestamp>();
    }
}

// ==================== Generic Paginated Response ====================

template<typename T>
struct Page {
    std::vector<T> data;
    std::string next_cursor;
    uint64_t limit;
    uint64_t count;
};

template<typename T>
inline void from_json(const json& j, Page<T>& page) {
    j.at("data").get_to(page.data);
    j.at("next_cursor").get_to(page.next_cursor);
    j.at("limit").get_to(page.limit);
    j.at("count").get_to(page.count);
}

// ==================== Legacy Types (for backward compatibility) ====================

struct OrderArgs {
    std::string token_id;
    double price;
    double size;
    Side side;
    std::optional<uint64_t> fee_rate_bps = std::nullopt;
    uint64_t nonce = 0;
    uint64_t expiration = 0;
    std::string taker = "0x0000000000000000000000000000000000000000";
};

struct MarketOrderArgs {
    std::string token_id;
    double amount;
    Side side;
    OrderType order_type = OrderType::FOK;
    std::optional<double> price = std::nullopt;
    std::optional<uint64_t> fee_rate_bps = std::nullopt;
    uint64_t nonce = 0;
    std::string taker = "0x0000000000000000000000000000000000000000";
};

struct CreateOrderOptions {
    std::string tick_size;
    bool neg_risk;
};

struct RoundConfig {
    int price;
    int size;
    int amount;
};

// ==================== Rewards/Earnings API Types ====================

struct Earning {
    std::string market;
    std::string asset_id;
    std::string amount;
};

inline void from_json(const json& j, Earning& e) {
    j.at("market").get_to(e.market);
    j.at("assetId").get_to(e.asset_id);
    j.at("amount").get_to(e.amount);
}

inline void to_json(json& j, const Earning& e) {
    j = json{
        {"market", e.market},
        {"assetId", e.asset_id},
        {"amount", e.amount}
    };
}

struct RewardsMakerOrder {
    std::string order_id;
    std::string market;
    std::string asset_id;
    std::string original_size;
    std::string price;
    Side side;
    Timestamp timestamp;
    std::string matched_size;
    std::string outcome;
};

inline void from_json(const json& j, RewardsMakerOrder& m) {
    j.at("orderId").get_to(m.order_id);
    j.at("market").get_to(m.market);
    j.at("assetId").get_to(m.asset_id);
    j.at("originalSize").get_to(m.original_size);
    j.at("price").get_to(m.price);
    j.at("side").get_to(m.side);
    j.at("timestamp").get_to(m.timestamp);
    j.at("matchedSize").get_to(m.matched_size);
    j.at("outcome").get_to(m.outcome);
}

inline void to_json(json& j, const RewardsMakerOrder& m) {
    j = json{
        {"orderId", m.order_id},
        {"market", m.market},
        {"assetId", m.asset_id},
        {"originalSize", m.original_size},
        {"price", m.price},
        {"side", m.side},
        {"timestamp", m.timestamp},
        {"matchedSize", m.matched_size},
        {"outcome", m.outcome}
    };
}

struct RewardsUserInfo {
    std::string user;
    std::vector<RewardsMakerOrder> maker_orders;
};

inline void from_json(const json& j, RewardsUserInfo& u) {
    j.at("user").get_to(u.user);
    j.at("makerOrders").get_to(u.maker_orders);
}

inline void to_json(json& j, const RewardsUserInfo& u) {
    j = json{
        {"user", u.user},
        {"makerOrders", u.maker_orders}
    };
}

struct RewardsMarketInfo {
    std::string market;
    std::string asset_id;
    std::string rewards_daily_rate;
    std::vector<RewardsUserInfo> user_info;
};

inline void from_json(const json& j, RewardsMarketInfo& m) {
    j.at("market").get_to(m.market);
    j.at("assetId").get_to(m.asset_id);
    j.at("rewardsDailyRate").get_to(m.rewards_daily_rate);
    j.at("userInfo").get_to(m.user_info);
}

inline void to_json(json& j, const RewardsMarketInfo& m) {
    j = json{
        {"market", m.market},
        {"assetId", m.asset_id},
        {"rewardsDailyRate", m.rewards_daily_rate},
        {"userInfo", m.user_info}
    };
}

struct UserEarningResponse {
    std::string user;
    std::string market;
    std::string asset_id;
    std::string date;
    std::string amount;
};

inline void from_json(const json& j, UserEarningResponse& r) {
    j.at("user").get_to(r.user);
    j.at("market").get_to(r.market);
    j.at("assetId").get_to(r.asset_id);
    j.at("date").get_to(r.date);
    j.at("amount").get_to(r.amount);
}

inline void to_json(json& j, const UserEarningResponse& r) {
    j = json{
        {"user", r.user},
        {"market", r.market},
        {"assetId", r.asset_id},
        {"date", r.date},
        {"amount", r.amount}
    };
}

struct TotalUserEarningResponse {
    std::string user;
    std::string date;
    std::string total_earnings;
    std::vector<Earning> earnings;
};

inline void from_json(const json& j, TotalUserEarningResponse& r) {
    j.at("user").get_to(r.user);
    j.at("date").get_to(r.date);
    j.at("totalEarnings").get_to(r.total_earnings);
    j.at("earnings").get_to(r.earnings);
}

inline void to_json(json& j, const TotalUserEarningResponse& r) {
    j = json{
        {"user", r.user},
        {"date", r.date},
        {"totalEarnings", r.total_earnings},
        {"earnings", r.earnings}
    };
}

struct RewardsConfig {
    std::string rewards_daily_rate;
    std::string start_date;
    std::string end_date;
};

inline void from_json(const json& j, RewardsConfig& r) {
    j.at("rewardsDailyRate").get_to(r.rewards_daily_rate);
    j.at("startDate").get_to(r.start_date);
    j.at("endDate").get_to(r.end_date);
}

inline void to_json(json& j, const RewardsConfig& r) {
    j = json{
        {"rewardsDailyRate", r.rewards_daily_rate},
        {"startDate", r.start_date},
        {"endDate", r.end_date}
    };
}

struct MarketRewardsConfig {
    std::string market;
    std::string asset_id;
    std::vector<RewardsConfig> rewards_config;
};

inline void from_json(const json& j, MarketRewardsConfig& m) {
    j.at("market").get_to(m.market);
    j.at("assetId").get_to(m.asset_id);
    j.at("rewardsConfig").get_to(m.rewards_config);
}

inline void to_json(json& j, const MarketRewardsConfig& m) {
    j = json{
        {"market", m.market},
        {"assetId", m.asset_id},
        {"rewardsConfig", m.rewards_config}
    };
}

struct UserRewardsEarningRequest {
    std::string start_date;
    std::string end_date;
};

inline void to_json(json& j, const UserRewardsEarningRequest& r) {
    j = json{
        {"startDate", r.start_date},
        {"endDate", r.end_date}
    };
}

struct UserRewardsEarningResponse {
    std::string user;
    std::string start_date;
    std::string end_date;
    std::string total_earnings;
    std::vector<Earning> earnings;
    std::vector<MarketRewardsConfig> markets_config;
};

inline void from_json(const json& j, UserRewardsEarningResponse& r) {
    j.at("user").get_to(r.user);
    j.at("startDate").get_to(r.start_date);
    j.at("endDate").get_to(r.end_date);
    j.at("totalEarnings").get_to(r.total_earnings);
    j.at("earnings").get_to(r.earnings);
    j.at("marketsConfig").get_to(r.markets_config);
}

inline void to_json(json& j, const UserRewardsEarningResponse& r) {
    j = json{
        {"user", r.user},
        {"startDate", r.start_date},
        {"endDate", r.end_date},
        {"totalEarnings", r.total_earnings},
        {"earnings", r.earnings},
        {"marketsConfig", r.markets_config}
    };
}

struct RewardsPercentagesResponse {
    std::string date;
    std::unordered_map<std::string, std::string> percentages;
};

inline void from_json(const json& j, RewardsPercentagesResponse& r) {
    j.at("date").get_to(r.date);
    j.at("percentages").get_to(r.percentages);
}

inline void to_json(json& j, const RewardsPercentagesResponse& r) {
    j = json{
        {"date", r.date},
        {"percentages", r.percentages}
    };
}

struct CurrentRewardResponse {
    std::string market;
    std::string asset_id;
    std::string rewards_daily_rate;
    std::string rewards_min_size;
    std::string rewards_max_spread;
};

inline void from_json(const json& j, CurrentRewardResponse& r) {
    j.at("market").get_to(r.market);
    j.at("assetId").get_to(r.asset_id);
    j.at("rewardsDailyRate").get_to(r.rewards_daily_rate);
    j.at("rewardsMinSize").get_to(r.rewards_min_size);
    j.at("rewardsMaxSpread").get_to(r.rewards_max_spread);
}

inline void to_json(json& j, const CurrentRewardResponse& r) {
    j = json{
        {"market", r.market},
        {"assetId", r.asset_id},
        {"rewardsDailyRate", r.rewards_daily_rate},
        {"rewardsMinSize", r.rewards_min_size},
        {"rewardsMaxSpread", r.rewards_max_spread}
    };
}

struct MarketRewardResponse {
    std::string market;
    std::string asset_id;
    std::string date;
    std::vector<RewardsMarketInfo> market_info;
};

inline void from_json(const json& j, MarketRewardResponse& r) {
    j.at("market").get_to(r.market);
    j.at("assetId").get_to(r.asset_id);
    j.at("date").get_to(r.date);
    j.at("marketInfo").get_to(r.market_info);
}

inline void to_json(json& j, const MarketRewardResponse& r) {
    j = json{
        {"market", r.market},
        {"assetId", r.asset_id},
        {"date", r.date},
        {"marketInfo", r.market_info}
    };
}

} // namespace clob
