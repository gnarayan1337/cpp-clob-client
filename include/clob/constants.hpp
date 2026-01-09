#pragma once

#include <string>
#include <unordered_map>
#include "types.hpp"

namespace clob {

// Endpoints
namespace endpoints {
    constexpr const char* TIME = "/time";
    constexpr const char* CREATE_API_KEY = "/auth/api-key";
    constexpr const char* DERIVE_API_KEY = "/auth/derive-api-key";
    constexpr const char* GET_API_KEYS = "/auth/api-keys";
    constexpr const char* DELETE_API_KEY = "/auth/api-key";
    constexpr const char* CLOSED_ONLY = "/auth/ban-status/closed-only";
    
    constexpr const char* GET_ORDER_BOOK = "/book";
    constexpr const char* GET_ORDER_BOOKS = "/books";
    constexpr const char* GET_TICK_SIZE = "/tick-size";
    constexpr const char* GET_NEG_RISK = "/neg-risk";
    constexpr const char* GET_FEE_RATE = "/fee-rate";
    constexpr const char* MID_POINT = "/midpoint";
    constexpr const char* MID_POINTS = "/midpoints";
    constexpr const char* PRICE = "/price";
    constexpr const char* GET_PRICES = "/prices";
    constexpr const char* GET_SPREAD = "/spread";
    constexpr const char* GET_SPREADS = "/spreads";
    constexpr const char* GET_LAST_TRADE_PRICE = "/last-trade-price";
    constexpr const char* GET_LAST_TRADES_PRICES = "/last-trades-prices";
    
    constexpr const char* GET_MARKETS = "/markets";
    constexpr const char* GET_MARKET = "/markets/";
    constexpr const char* GET_SIMPLIFIED_MARKETS = "/simplified-markets";
    constexpr const char* GET_SAMPLING_MARKETS = "/sampling-markets";
    constexpr const char* GET_SAMPLING_SIMPLIFIED_MARKETS = "/sampling-simplified-markets";
    
    constexpr const char* ORDERS = "/data/orders";
    constexpr const char* GET_ORDER = "/data/order/";
    constexpr const char* POST_ORDER = "/order";
    constexpr const char* POST_ORDERS = "/orders";
    constexpr const char* CANCEL = "/order";
    constexpr const char* CANCEL_ORDERS = "/orders";
    constexpr const char* CANCEL_ALL = "/cancel-all";
    constexpr const char* CANCEL_MARKET_ORDERS = "/cancel-market-orders";
    
    constexpr const char* TRADES = "/data/trades";
    constexpr const char* GET_NOTIFICATIONS = "/notifications";
    constexpr const char* DROP_NOTIFICATIONS = "/notifications";
    constexpr const char* GET_BALANCE_ALLOWANCE = "/balance-allowance";
    constexpr const char* UPDATE_BALANCE_ALLOWANCE = "/balance-allowance/update";
    constexpr const char* IS_ORDER_SCORING = "/order-scoring";
    constexpr const char* ARE_ORDERS_SCORING = "/orders-scoring";
}

// Default cursor for pagination
constexpr const char* INITIAL_CURSOR = "MA==";
constexpr const char* END_CURSOR = "LTE=";

// Rounding configurations
const std::unordered_map<std::string, RoundConfig> ROUNDING_CONFIG = {
    {"0.1", {1, 2, 3}},
    {"0.01", {2, 2, 4}},
    {"0.001", {3, 2, 5}},
    {"0.0001", {4, 2, 6}}
};

// Contract configurations
ContractConfig get_contract_config(uint64_t chain_id, bool neg_risk = false);

// Order domain name and version for EIP712
constexpr const char* ORDER_DOMAIN_NAME = "Polymarket CTF Exchange";
constexpr const char* ORDER_VERSION = "1";

} // namespace clob



