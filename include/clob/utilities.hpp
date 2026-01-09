#pragma once

#include <string>
#include <cmath>
#include <optional>
#include <simdjson.h>
#include "types.hpp"

namespace clob {
namespace utils {

// ========== SIMD JSON Parsing Helpers ==========

// Safe extraction from simdjson::dom::element
std::string get_string(const simdjson::dom::element& elem, const char* key);
std::optional<std::string> get_optional_string(const simdjson::dom::element& elem, const char* key);

int64_t get_int64(const simdjson::dom::element& elem, const char* key);
std::optional<int64_t> get_optional_int64(const simdjson::dom::element& elem, const char* key);

uint64_t get_uint64(const simdjson::dom::element& elem, const char* key);
std::optional<uint64_t> get_optional_uint64(const simdjson::dom::element& elem, const char* key);

double get_double(const simdjson::dom::element& elem, const char* key);
std::optional<double> get_optional_double(const simdjson::dom::element& elem, const char* key);

bool get_bool(const simdjson::dom::element& elem, const char* key);
std::optional<bool> get_optional_bool(const simdjson::dom::element& elem, const char* key);

// Array helpers
simdjson::dom::array get_array(const simdjson::dom::element& elem, const char* key);
std::optional<simdjson::dom::array> get_optional_array(const simdjson::dom::element& elem, const char* key);

// Object helpers
simdjson::dom::object get_object(const simdjson::dom::element& elem, const char* key);
std::optional<simdjson::dom::object> get_optional_object(const simdjson::dom::element& elem, const char* key);

// Parse responses from simdjson elements - Core types
MarketResponse parse_market_simd(const simdjson::dom::element& elem);
SimplifiedMarketResponse parse_simplified_market_simd(const simdjson::dom::element& elem);
OrderBookSummaryResponse parse_orderbook_simd(const simdjson::dom::element& elem);
PostOrderResponse parse_post_order_simd(const simdjson::dom::element& elem);
OpenOrderResponse parse_open_order_simd(const simdjson::dom::element& elem);
TradeResponse parse_trade_simd(const simdjson::dom::element& elem);

// Simple response types
TickSizeResponse parse_tick_size_simd(const simdjson::dom::element& elem);
NegRiskResponse parse_neg_risk_simd(const simdjson::dom::element& elem);
FeeRateResponse parse_fee_rate_simd(const simdjson::dom::element& elem);
MidpointResponse parse_midpoint_simd(const simdjson::dom::element& elem);
PriceResponse parse_price_simd(const simdjson::dom::element& elem);
SpreadResponse parse_spread_simd(const simdjson::dom::element& elem);
LastTradePriceResponse parse_last_trade_price_simd(const simdjson::dom::element& elem);
LastTradesPricesResponse parse_last_trades_prices_simd(const simdjson::dom::element& elem);

// Complex response types
ApiKeysResponse parse_api_keys_simd(const simdjson::dom::element& elem);
BalanceAllowanceResponse parse_balance_allowance_simd(const simdjson::dom::element& elem);
NotificationResponse parse_notification_simd(const simdjson::dom::element& elem);
CancelOrdersResponse parse_cancel_orders_simd(const simdjson::dom::element& elem);
BanStatusResponse parse_ban_status_simd(const simdjson::dom::element& elem);
OrderScoringResponse parse_order_scoring_simd(const simdjson::dom::element& elem);

// Reward/earning response types
UserEarningResponse parse_user_earning_simd(const simdjson::dom::element& elem);
TotalUserEarningResponse parse_total_user_earning_simd(const simdjson::dom::element& elem);
RewardsPercentagesResponse parse_rewards_percentages_simd(const simdjson::dom::element& elem);
CurrentRewardResponse parse_current_reward_simd(const simdjson::dom::element& elem);
MarketRewardResponse parse_market_reward_simd(const simdjson::dom::element& elem);

// Parse vector from simdjson array
template<typename T>
std::vector<T> parse_vector_simd(
    const simdjson::dom::array& arr,
    T (*parse_fn)(const simdjson::dom::element&)
);

// Template for parsing Page<T> from simdjson
template<typename T>
Page<T> parse_page_simd(
    const simdjson::dom::element& elem,
    T (*parse_fn)(const simdjson::dom::element&)
);

// ========== Original Utilities ==========

// Rounding utilities
double round_down(double value, int decimals);
double round_up(double value, int decimals);
double round_normal(double value, int decimals);
int decimal_places(double value);

// Convert to token decimals (6 decimals for USDC)
uint64_t to_token_decimals(double value);

// Price validation
bool price_valid(double price, const std::string& tick_size);

// Tick size comparison
bool is_tick_size_smaller(const std::string& tick_size, const std::string& min_tick_size);

// Convert order to JSON for posting
json order_to_json(
    const SignedOrder& order,
    const std::string& owner,
    OrderType order_type
);

// Parse orderbook summary
OrderBookSummaryResponse parse_raw_orderbook_summary(const json& raw);

// Generate orderbook hash
std::string generate_orderbook_summary_hash(const OrderBookSummaryResponse& orderbook);

// Hex utilities
std::string to_checksum_address(const std::string& address);

} // namespace utils
} // namespace clob


