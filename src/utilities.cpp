#include "clob/utilities.hpp"
#include "clob/eip712.hpp"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace clob {
namespace utils {

// ========== SIMD JSON Parsing Helpers ==========

std::string get_string(const simdjson::dom::element& elem, const char* key) {
    auto result = elem[key].get_string();
    if (result.error()) {
        throw std::runtime_error(std::string("Failed to get string field: ") + key);
    }
    return std::string(result.value());
}

std::optional<std::string> get_optional_string(const simdjson::dom::element& elem, const char* key) {
    auto result = elem[key].get_string();
    if (result.error()) {
        return std::nullopt;
    }
    return std::string(result.value());
}

int64_t get_int64(const simdjson::dom::element& elem, const char* key) {
    auto result = elem[key].get_int64();
    if (result.error()) {
        throw std::runtime_error(std::string("Failed to get int64 field: ") + key);
    }
    return result.value();
}

std::optional<int64_t> get_optional_int64(const simdjson::dom::element& elem, const char* key) {
    auto result = elem[key].get_int64();
    if (result.error()) {
        return std::nullopt;
    }
    return result.value();
}

uint64_t get_uint64(const simdjson::dom::element& elem, const char* key) {
    auto result = elem[key].get_uint64();
    if (result.error()) {
        throw std::runtime_error(std::string("Failed to get uint64 field: ") + key);
    }
    return result.value();
}

std::optional<uint64_t> get_optional_uint64(const simdjson::dom::element& elem, const char* key) {
    auto result = elem[key].get_uint64();
    if (result.error()) {
        return std::nullopt;
    }
    return result.value();
}

double get_double(const simdjson::dom::element& elem, const char* key) {
    auto result = elem[key].get_double();
    if (result.error()) {
        throw std::runtime_error(std::string("Failed to get double field: ") + key);
    }
    return result.value();
}

std::optional<double> get_optional_double(const simdjson::dom::element& elem, const char* key) {
    auto result = elem[key].get_double();
    if (result.error()) {
        return std::nullopt;
    }
    return result.value();
}

bool get_bool(const simdjson::dom::element& elem, const char* key) {
    auto result = elem[key].get_bool();
    if (result.error()) {
        throw std::runtime_error(std::string("Failed to get bool field: ") + key);
    }
    return result.value();
}

std::optional<bool> get_optional_bool(const simdjson::dom::element& elem, const char* key) {
    auto result = elem[key].get_bool();
    if (result.error()) {
        return std::nullopt;
    }
    return result.value();
}

simdjson::dom::array get_array(const simdjson::dom::element& elem, const char* key) {
    auto result = elem[key].get_array();
    if (result.error()) {
        throw std::runtime_error(std::string("Failed to get array field: ") + key);
    }
    return result.value();
}

std::optional<simdjson::dom::array> get_optional_array(const simdjson::dom::element& elem, const char* key) {
    auto result = elem[key].get_array();
    if (result.error()) {
        return std::nullopt;
    }
    return result.value();
}

simdjson::dom::object get_object(const simdjson::dom::element& elem, const char* key) {
    auto result = elem[key].get_object();
    if (result.error()) {
        throw std::runtime_error(std::string("Failed to get object field: ") + key);
    }
    return result.value();
}

std::optional<simdjson::dom::object> get_optional_object(const simdjson::dom::element& elem, const char* key) {
    auto result = elem[key].get_object();
    if (result.error()) {
        return std::nullopt;
    }
    return result.value();
}

// Parse MarketResponse from simdjson
MarketResponse parse_market_simd(const simdjson::dom::element& elem) {
    MarketResponse market;
    
    market.enable_order_book = get_bool(elem, "enable_order_book");
    market.active = get_bool(elem, "active");
    market.closed = get_bool(elem, "closed");
    market.archived = get_bool(elem, "archived");
    market.accepting_orders = get_bool(elem, "accepting_orders");
    market.accepting_order_timestamp = get_optional_string(elem, "accepting_order_timestamp");
    
    market.minimum_order_size = get_double(elem, "minimum_order_size");
    market.minimum_tick_size = get_double(elem, "minimum_tick_size");
    
    market.condition_id = get_string(elem, "condition_id");
    market.question_id = get_string(elem, "question_id");
    market.question = get_string(elem, "question");
    market.description = get_string(elem, "description");
    market.market_slug = get_string(elem, "market_slug");
    
    market.end_date_iso = get_optional_string(elem, "end_date_iso");
    market.game_start_time = get_optional_string(elem, "game_start_time");
    
    market.seconds_delay = get_uint64(elem, "seconds_delay");
    market.fpmm = get_string(elem, "fpmm");
    market.maker_base_fee = get_double(elem, "maker_base_fee");
    market.taker_base_fee = get_double(elem, "taker_base_fee");
    market.notifications_enabled = get_bool(elem, "notifications_enabled");
    market.neg_risk = get_bool(elem, "neg_risk");
    market.neg_risk_market_id = get_string(elem, "neg_risk_market_id");
    market.neg_risk_request_id = get_string(elem, "neg_risk_request_id");
    market.icon = get_string(elem, "icon");
    market.image = get_string(elem, "image");
    
    // Rewards - for now set defaults
    market.rewards.min_size = 0.0;
    market.rewards.max_spread = 0.0;
    
    market.is_50_50_outcome = get_bool(elem, "is_50_50_outcome");
    
    // Tokens array
    auto tokens_arr = get_array(elem, "tokens");
    for (auto token_elem : tokens_arr) {
        Token token;
        token.token_id = get_string(token_elem, "token_id");
        token.outcome = get_string(token_elem, "outcome");
        token.price = get_double(token_elem, "price");
        token.winner = get_optional_bool(token_elem, "winner").value_or(false);
        market.tokens.push_back(token);
    }
    
    // Tags array - optional
    auto tags_result = elem["tags"].get_array();
    if (!tags_result.error()) {
        for (auto tag : tags_result.value()) {
            market.tags.push_back(std::string(tag.get_string().value()));
        }
    }
    
    return market;
}

// Parse OrderBookSummaryResponse from simdjson
OrderBookSummaryResponse parse_orderbook_simd(const simdjson::dom::element& elem) {
    OrderBookSummaryResponse book;
    
    book.market = get_string(elem, "market");
    book.asset_id = get_string(elem, "asset_id");
    book.timestamp = get_string(elem, "timestamp");  // String, not int64!
    book.hash = get_optional_string(elem, "hash");
    
    // Bids array
    auto bids_arr = get_array(elem, "bids");
    for (auto bid_elem : bids_arr) {
        OrderSummary bid;
        bid.price = get_string(bid_elem, "price");
        bid.size = get_string(bid_elem, "size");
        book.bids.push_back(bid);
    }
    
    // Asks array
    auto asks_arr = get_array(elem, "asks");
    for (auto ask_elem : asks_arr) {
        OrderSummary ask;
        ask.price = get_string(ask_elem, "price");
        ask.size = get_string(ask_elem, "size");
        book.asks.push_back(ask);
    }
    
    // Optional fields with defaults
    book.min_order_size = get_optional_string(elem, "min_order_size").value_or("0");
    book.neg_risk = get_optional_bool(elem, "neg_risk").value_or(false);
    
    // Tick size (default to hundredth)
    book.tick_size = TickSize::HUNDREDTH;
    
    return book;
}

// Parse PostOrderResponse from simdjson
PostOrderResponse parse_post_order_simd(const simdjson::dom::element& elem) {
    PostOrderResponse response;
    
    response.order_id = get_string(elem, "orderID");
    response.success = get_bool(elem, "success");
    
    // Parse status string and convert to enum
    std::string status_str = get_string(elem, "status");
    if (status_str == "LIVE") response.status = OrderStatusType::LIVE;
    else if (status_str == "MATCHED") response.status = OrderStatusType::MATCHED;
    else if (status_str == "CANCELED") response.status = OrderStatusType::CANCELED;
    else if (status_str == "DELAYED") response.status = OrderStatusType::DELAYED;
    else if (status_str == "UNMATCHED") response.status = OrderStatusType::UNMATCHED;
    else response.status = OrderStatusType::UNKNOWN;
    
    response.error_msg = get_optional_string(elem, "errorMsg");
    response.making_amount = get_optional_string(elem, "making_amount").value_or("0");
    response.taking_amount = get_optional_string(elem, "taking_amount").value_or("0");
    
    // Optional arrays
    auto tx_result = elem["transaction_hashes"].get_array();
    if (!tx_result.error()) {
        for (auto tx : tx_result.value()) {
            response.transaction_hashes.push_back(std::string(tx.get_string().value()));
        }
    }
    
    auto trade_result = elem["trade_ids"].get_array();
    if (!trade_result.error()) {
        for (auto trade : trade_result.value()) {
            response.trade_ids.push_back(std::string(trade.get_string().value()));
        }
    }
    
    return response;
}

// Parse OpenOrderResponse from simdjson
OpenOrderResponse parse_open_order_simd(const simdjson::dom::element& elem) {
    OpenOrderResponse order;
    
    order.id = get_string(elem, "id");
    
    // Parse status
    std::string status_str = get_string(elem, "status");
    if (status_str == "LIVE") order.status = OrderStatusType::LIVE;
    else if (status_str == "MATCHED") order.status = OrderStatusType::MATCHED;
    else if (status_str == "CANCELED") order.status = OrderStatusType::CANCELED;
    else if (status_str == "DELAYED") order.status = OrderStatusType::DELAYED;
    else if (status_str == "UNMATCHED") order.status = OrderStatusType::UNMATCHED;
    else order.status = OrderStatusType::UNKNOWN;
    
    order.owner = get_string(elem, "owner");
    order.maker_address = get_string(elem, "maker_address");
    order.market = get_string(elem, "market");
    order.asset_id = get_string(elem, "asset_id");
    
    // Parse side
    std::string side_str = get_string(elem, "side");
    order.side = (side_str == "BUY") ? Side::BUY : Side::SELL;
    
    order.original_size = get_string(elem, "original_size");
    order.size_matched = get_string(elem, "size_matched");
    order.price = get_string(elem, "price");
    
    // Optional array
    auto trades_result = elem["associate_trades"].get_array();
    if (!trades_result.error()) {
        for (auto trade : trades_result.value()) {
            order.associate_trades.push_back(std::string(trade.get_string().value()));
        }
    }
    
    order.outcome = get_string(elem, "outcome");
    order.created_at = get_int64(elem, "created_at");
    order.expiration = get_int64(elem, "expiration");
    
    // Parse order type
    std::string type_str = get_string(elem, "order_type");
    if (type_str == "GTC" || type_str == "gtc") order.order_type = OrderType::GTC;
    else if (type_str == "FOK" || type_str == "fok") order.order_type = OrderType::FOK;
    else if (type_str == "GTD" || type_str == "gtd") order.order_type = OrderType::GTD;
    else if (type_str == "FAK" || type_str == "fak") order.order_type = OrderType::FAK;
    else order.order_type = OrderType::UNKNOWN;
    
    return order;
}

// Parse TradeResponse from simdjson
TradeResponse parse_trade_simd(const simdjson::dom::element& elem) {
    TradeResponse trade;
    
    trade.id = get_string(elem, "id");
    trade.taker_order_id = get_string(elem, "taker_order_id");
    trade.market = get_string(elem, "market");
    trade.asset_id = get_string(elem, "asset_id");
    
    // Parse side
    std::string side_str = get_string(elem, "side");
    trade.side = (side_str == "BUY") ? Side::BUY : Side::SELL;
    
    trade.size = get_string(elem, "size");
    trade.fee_rate_bps = get_string(elem, "fee_rate_bps");
    trade.price = get_string(elem, "price");
    
    // Parse status
    std::string status_str = get_string(elem, "status");
    if (status_str == "LIVE") trade.status = OrderStatusType::LIVE;
    else if (status_str == "MATCHED") trade.status = OrderStatusType::MATCHED;
    else if (status_str == "CANCELED") trade.status = OrderStatusType::CANCELED;
    else if (status_str == "DELAYED") trade.status = OrderStatusType::DELAYED;
    else if (status_str == "UNMATCHED") trade.status = OrderStatusType::UNMATCHED;
    else trade.status = OrderStatusType::MATCHED;  // Default for trades
    
    trade.match_time = get_int64(elem, "match_time");
    trade.last_update = get_int64(elem, "last_update");
    trade.outcome = get_string(elem, "outcome");
    trade.bucket_index = static_cast<uint32_t>(get_uint64(elem, "bucket_index"));
    trade.owner = get_string(elem, "owner");
    trade.maker_address = get_string(elem, "maker_address");
    
    // Optional maker_orders array - skip for now
    
    trade.transaction_hash = get_string(elem, "transaction_hash");
    
    // Parse trader_side
    std::string trader_side_str = get_string(elem, "trader_side");
    if (trader_side_str == "TAKER") trade.trader_side = TraderSide::TAKER;
    else if (trader_side_str == "MAKER") trade.trader_side = TraderSide::MAKER;
    else trade.trader_side = TraderSide::UNKNOWN;
    
    trade.error_msg = get_optional_string(elem, "error_msg");
    
    return trade;
}

// Template implementation for Page<T>
template<typename T>
Page<T> parse_page_simd(
    const simdjson::dom::element& elem,
    T (*parse_fn)(const simdjson::dom::element&)
) {
    Page<T> page;
    
    // Parse data array
    auto data_arr = get_array(elem, "data");
    for (auto item_elem : data_arr) {
        page.data.push_back(parse_fn(item_elem));
    }
    
    // Parse pagination fields
    page.next_cursor = get_optional_string(elem, "next_cursor").value_or("");
    page.limit = static_cast<int>(get_optional_int64(elem, "limit").value_or(0));
    page.count = static_cast<int>(get_optional_int64(elem, "count").value_or(0));
    
    return page;
}

// Parse SimplifiedMarketResponse from simdjson
SimplifiedMarketResponse parse_simplified_market_simd(const simdjson::dom::element& elem) {
    SimplifiedMarketResponse market;
    
    market.condition_id = get_string(elem, "condition_id");
    market.active = get_bool(elem, "active");
    market.closed = get_bool(elem, "closed");
    
    // Tokens array
    auto tokens_arr = get_array(elem, "tokens");
    for (auto token_elem : tokens_arr) {
        Token token;
        token.token_id = get_string(token_elem, "token_id");
        token.outcome = get_string(token_elem, "outcome");
        token.price = get_double(token_elem, "price");
        token.winner = get_optional_bool(token_elem, "winner").value_or(false);
        market.tokens.push_back(token);
    }
    
    return market;
}

// ========== Simple Response Type Parsers ==========

TickSizeResponse parse_tick_size_simd(const simdjson::dom::element& elem) {
    TickSizeResponse resp;
    
    // Try to get as double, fallback to string
    double tick_size_val = 0.01;  // Default
    auto result = elem["minimum_tick_size"].get_double();
    if (!result.error()) {
        tick_size_val = result.value();
    } else {
        auto str_result = elem["minimum_tick_size"].get_string();
        if (!str_result.error()) {
            tick_size_val = std::stod(std::string(str_result.value()));
        }
    }
    
    if (tick_size_val == 0.1) resp.minimum_tick_size = TickSize::TENTH;
    else if (tick_size_val == 0.01) resp.minimum_tick_size = TickSize::HUNDREDTH;
    else if (tick_size_val == 0.001) resp.minimum_tick_size = TickSize::THOUSANDTH;
    else if (tick_size_val == 0.0001) resp.minimum_tick_size = TickSize::TEN_THOUSANDTH;
    else resp.minimum_tick_size = TickSize::HUNDREDTH;
    
    return resp;
}

NegRiskResponse parse_neg_risk_simd(const simdjson::dom::element& elem) {
    NegRiskResponse resp;
    resp.neg_risk = get_bool(elem, "neg_risk");
    return resp;
}

FeeRateResponse parse_fee_rate_simd(const simdjson::dom::element& elem) {
    FeeRateResponse resp;
    resp.base_fee = static_cast<uint32_t>(get_uint64(elem, "base_fee"));
    return resp;
}

MidpointResponse parse_midpoint_simd(const simdjson::dom::element& elem) {
    MidpointResponse resp;
    resp.mid = get_string(elem, "mid");
    return resp;
}

PriceResponse parse_price_simd(const simdjson::dom::element& elem) {
    PriceResponse resp;
    resp.price = get_string(elem, "price");
    return resp;
}

SpreadResponse parse_spread_simd(const simdjson::dom::element& elem) {
    SpreadResponse resp;
    resp.spread = get_string(elem, "spread");
    return resp;
}

LastTradePriceResponse parse_last_trade_price_simd(const simdjson::dom::element& elem) {
    LastTradePriceResponse resp;
    resp.price = get_string(elem, "price");
    
    std::string side_str = get_string(elem, "side");
    resp.side = (side_str == "BUY") ? Side::BUY : Side::SELL;
    
    return resp;
}

// ========== Complex Response Type Parsers ==========

ApiKeysResponse parse_api_keys_simd(const simdjson::dom::element& elem) {
    ApiKeysResponse resp;
    
    auto keys_result = elem["apiKeys"].get_array();
    if (!keys_result.error()) {
        std::vector<std::string> keys;
        for (auto key_elem : keys_result.value()) {
            keys.push_back(std::string(key_elem.get_string().value()));
        }
        resp.keys = keys;
    }
    
    return resp;
}

BalanceAllowanceResponse parse_balance_allowance_simd(const simdjson::dom::element& elem) {
    BalanceAllowanceResponse resp;
    
    resp.balance = get_string(elem, "balance");
    
    // Allowances is a map
    auto allowances_result = elem["allowances"].get_object();
    if (!allowances_result.error()) {
        for (auto field : allowances_result.value()) {
            std::string key(field.key);
            std::string value(field.value.get_string().value());
            resp.allowances[key] = value;
        }
    }
    
    return resp;
}

NotificationResponse parse_notification_simd(const simdjson::dom::element& elem) {
    NotificationResponse resp;
    
    resp.type = static_cast<uint32_t>(get_uint64(elem, "type"));
    resp.owner = get_string(elem, "owner");
    
    // Payload parsing - skip for now (complex nested structure)
    
    return resp;
}

CancelOrdersResponse parse_cancel_orders_simd(const simdjson::dom::element& elem) {
    CancelOrdersResponse resp;
    
    // Canceled array
    auto canceled_result = elem["canceled"].get_array();
    if (!canceled_result.error()) {
        for (auto id : canceled_result.value()) {
            resp.canceled.push_back(std::string(id.get_string().value()));
        }
    }
    
    // Not_canceled is a map of order_id -> error_message
    auto not_canceled_result = elem["not_canceled"].get_object();
    if (!not_canceled_result.error()) {
        for (auto field : not_canceled_result.value()) {
            std::string key(field.key);
            std::string value(field.value.get_string().value());
            resp.not_canceled[key] = value;
        }
    }
    
    return resp;
}

BanStatusResponse parse_ban_status_simd(const simdjson::dom::element& elem) {
    BanStatusResponse resp;
    resp.closed_only = get_bool(elem, "closed_only");
    return resp;
}

OrderScoringResponse parse_order_scoring_simd(const simdjson::dom::element& elem) {
    OrderScoringResponse resp;
    resp.scoring = get_bool(elem, "scoring");
    return resp;
}

// ========== Reward/Earning Response Parsers ==========

UserEarningResponse parse_user_earning_simd(const simdjson::dom::element& elem) {
    UserEarningResponse resp;
    
    resp.user = get_string(elem, "user");
    resp.market = get_string(elem, "market");
    resp.asset_id = get_string(elem, "asset_id");
    resp.date = get_string(elem, "date");
    resp.amount = get_string(elem, "amount");
    
    return resp;
}

TotalUserEarningResponse parse_total_user_earning_simd(const simdjson::dom::element& elem) {
    TotalUserEarningResponse resp;
    
    resp.user = get_string(elem, "user");
    resp.date = get_string(elem, "date");
    resp.total_earnings = get_string(elem, "total_earnings");
    
    // Earnings array - skip for now (complex nested structure)
    
    return resp;
}

RewardsPercentagesResponse parse_rewards_percentages_simd(const simdjson::dom::element& elem) {
    RewardsPercentagesResponse resp;
    
    resp.date = get_string(elem, "date");
    
    // Percentages is a map
    auto percentages_result = elem["percentages"].get_object();
    if (!percentages_result.error()) {
        for (auto field : percentages_result.value()) {
            std::string key(field.key);
            std::string value(field.value.get_string().value());
            resp.percentages[key] = value;
        }
    }
    
    return resp;
}

CurrentRewardResponse parse_current_reward_simd(const simdjson::dom::element& elem) {
    CurrentRewardResponse resp;
    
    resp.market = get_string(elem, "market");
    resp.asset_id = get_string(elem, "asset_id");
    resp.rewards_daily_rate = get_string(elem, "rewards_daily_rate");
    resp.rewards_min_size = get_string(elem, "rewards_min_size");
    resp.rewards_max_spread = get_string(elem, "rewards_max_spread");
    
    return resp;
}

MarketRewardResponse parse_market_reward_simd(const simdjson::dom::element& elem) {
    MarketRewardResponse resp;
    
    resp.market = get_string(elem, "market");
    resp.asset_id = get_string(elem, "asset_id");
    resp.date = get_string(elem, "date");
    
    // Market_info array - skip for now (complex nested structure)
    
    return resp;
}

// Add LastTradesPricesResponse parser
LastTradesPricesResponse parse_last_trades_prices_simd(const simdjson::dom::element& elem) {
    LastTradesPricesResponse resp;
    
    resp.token_id = get_string(elem, "token_id");
    resp.price = get_string(elem, "price");
    
    std::string side_str = get_string(elem, "side");
    resp.side = (side_str == "BUY") ? Side::BUY : Side::SELL;
    
    return resp;
}

// Parse vector from simdjson array
template<typename T>
std::vector<T> parse_vector_simd(
    const simdjson::dom::array& arr,
    T (*parse_fn)(const simdjson::dom::element&)
) {
    std::vector<T> result;
    for (auto elem : arr) {
        result.push_back(parse_fn(elem));
    }
    return result;
}

// Explicit template instantiations
template std::vector<PostOrderResponse> parse_vector_simd(
    const simdjson::dom::array&,
    PostOrderResponse (*)(const simdjson::dom::element&)
);

template std::vector<NotificationResponse> parse_vector_simd(
    const simdjson::dom::array&,
    NotificationResponse (*)(const simdjson::dom::element&)
);

template std::vector<TotalUserEarningResponse> parse_vector_simd(
    const simdjson::dom::array&,
    TotalUserEarningResponse (*)(const simdjson::dom::element&)
);

template std::vector<OrderBookSummaryResponse> parse_vector_simd(
    const simdjson::dom::array&,
    OrderBookSummaryResponse (*)(const simdjson::dom::element&)
);

template std::vector<LastTradesPricesResponse> parse_vector_simd(
    const simdjson::dom::array&,
    LastTradesPricesResponse (*)(const simdjson::dom::element&)
);

// Template for parsing Page<T> from simdjson
template Page<MarketResponse> parse_page_simd(
    const simdjson::dom::element&,
    MarketResponse (*)(const simdjson::dom::element&)
);

template Page<SimplifiedMarketResponse> parse_page_simd(
    const simdjson::dom::element&,
    SimplifiedMarketResponse (*)(const simdjson::dom::element&)
);

template Page<OpenOrderResponse> parse_page_simd(
    const simdjson::dom::element&,
    OpenOrderResponse (*)(const simdjson::dom::element&)
);

template Page<TradeResponse> parse_page_simd(
    const simdjson::dom::element&,
    TradeResponse (*)(const simdjson::dom::element&)
);

template Page<UserEarningResponse> parse_page_simd(
    const simdjson::dom::element&,
    UserEarningResponse (*)(const simdjson::dom::element&)
);

template Page<CurrentRewardResponse> parse_page_simd(
    const simdjson::dom::element&,
    CurrentRewardResponse (*)(const simdjson::dom::element&)
);

template Page<MarketRewardResponse> parse_page_simd(
    const simdjson::dom::element&,
    MarketRewardResponse (*)(const simdjson::dom::element&)
);

// ========== Original Utilities ==========

double round_down(double value, int decimals) {
    double multiplier = std::pow(10.0, decimals);
    return std::floor(value * multiplier) / multiplier;
}

double round_up(double value, int decimals) {
    double multiplier = std::pow(10.0, decimals);
    return std::ceil(value * multiplier) / multiplier;
}

double round_normal(double value, int decimals) {
    double multiplier = std::pow(10.0, decimals);
    return std::round(value * multiplier) / multiplier;
}

int decimal_places(double value) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(10) << value;
    std::string str = oss.str();
    
    // Remove trailing zeros
    str.erase(str.find_last_not_of('0') + 1, std::string::npos);
    
    // Find decimal point
    size_t pos = str.find('.');
    if (pos == std::string::npos) {
        return 0;
    }
    
    return str.length() - pos - 1;
}

uint64_t to_token_decimals(double value) {
    // USDC has 6 decimals
    return static_cast<uint64_t>(round_normal(value * 1000000.0, 0));
}

bool price_valid(double price, const std::string& tick_size) {
    double tick = std::stod(tick_size);
    return price >= tick && price <= (1.0 - tick);
}

bool is_tick_size_smaller(const std::string& tick_size, const std::string& min_tick_size) {
    double ts = std::stod(tick_size);
    double mts = std::stod(min_tick_size);
    return ts < mts;
}

json order_to_json(const SignedOrder& order, const std::string& owner, OrderType order_type) {
    // Create a copy with the correct owner
    SignedOrder order_copy = order;
    order_copy.owner = owner;
    order_copy.order_type = order_type;
    
    // Use to_json
    json result;
    to_json(result, order_copy);
    return result;
}

OrderBookSummaryResponse parse_raw_orderbook_summary(const json& raw) {
    OrderBookSummaryResponse obs;
    raw.get_to(obs);
    return obs;
}

std::string generate_orderbook_summary_hash(const OrderBookSummaryResponse& orderbook) {
    // Create a JSON representation and hash it
    json j = {
        {"market", orderbook.market},
        {"asset_id", orderbook.asset_id},
        {"timestamp", orderbook.timestamp}
    };
    
    // Add bids
    json bids_array = json::array();
    for (const auto& bid : orderbook.bids) {
        bids_array.push_back({{"price", bid.price}, {"size", bid.size}});
    }
    j["bids"] = bids_array;
    
    // Add asks
    json asks_array = json::array();
    for (const auto& ask : orderbook.asks) {
        asks_array.push_back({{"price", ask.price}, {"size", ask.size}});
    }
    j["asks"] = asks_array;
    
    // Hash the JSON string
    std::string json_str = j.dump();
    std::vector<uint8_t> data(json_str.begin(), json_str.end());
    auto hash = eip712::keccak256(data);
    
    return eip712::bytes_to_hex(std::vector<uint8_t>(hash.begin(), hash.end()));
}

std::string to_checksum_address(const std::string& address) {
    // Remove 0x prefix if present
    std::string addr = address;
    if (addr.substr(0, 2) == "0x" || addr.substr(0, 2) == "0X") {
        addr = addr.substr(2);
    }
    
    // Convert to lowercase
    std::transform(addr.begin(), addr.end(), addr.begin(), ::tolower);
    
    // Hash the lowercase address
    std::vector<uint8_t> data(addr.begin(), addr.end());
    auto hash = eip712::keccak256(data);
    
    // Apply checksum
    std::string result = "0x";
    for (size_t i = 0; i < addr.length(); i++) {
        if (addr[i] >= 'a' && addr[i] <= 'f') {
            // Check if corresponding hash nibble is >= 8
            int nibble = (hash[i / 2] >> (i % 2 == 0 ? 4 : 0)) & 0xf;
            result += (nibble >= 8) ? std::toupper(addr[i]) : addr[i];
        } else {
            result += addr[i];
        }
    }
    
    return result;
}

} // namespace utils
} // namespace clob


