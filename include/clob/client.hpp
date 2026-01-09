#pragma once

#include <memory>
#include <unordered_map>
#include "types.hpp"
#include "constants.hpp"
#include "signer.hpp"
#include "order_builder.hpp"
#include "http_client.hpp"

namespace clob {

// Forward declare ConnectionStats (defined in http_client.hpp)
struct ConnectionStats;

class ClobClient {
public:
    // Constructor for different auth levels
    // L0: host only
    explicit ClobClient(const std::string& host);
    
    // L1: host + signer
    ClobClient(const std::string& host, std::shared_ptr<Signer> signer);
    
    // L2: host + signer + credentials
    ClobClient(
        const std::string& host,
        std::shared_ptr<Signer> signer,
        const ApiCreds& creds
    );
    
    // Get current authentication level
    AuthLevel get_mode() const { return mode_; }
    
    // Get signer address
    std::string get_address() const;
    
    // Get contract addresses
    std::string get_collateral_address() const;
    std::string get_conditional_address() const;
    std::string get_exchange_address(bool neg_risk = false) const;
    
    // ========== State Transitions ==========
    
    // Deauthenticate: Convert authenticated client back to public-only
    // Removes API credentials and signer, returning to L0 state
    void deauthenticate();
    
    // Check if this client is authenticated
    bool is_authenticated() const { return mode_ >= AuthLevel::L1; }
    
    // Check if this client has API credentials
    bool has_api_credentials() const { return mode_ >= AuthLevel::L2; }
    
    // ========== Public Endpoints (L0) ==========
    
    std::string get_ok();
    Timestamp get_server_time();
    
    // Markets
    Page<MarketResponse> get_markets(const std::string& next_cursor = INITIAL_CURSOR);
    MarketResponse get_market(const std::string& condition_id);
    Page<SimplifiedMarketResponse> get_simplified_markets(const std::string& next_cursor = INITIAL_CURSOR);
    Page<MarketResponse> get_sampling_markets(const std::string& next_cursor = INITIAL_CURSOR);
    Page<SimplifiedMarketResponse> get_sampling_simplified_markets(const std::string& next_cursor = INITIAL_CURSOR);
    
    // Orderbook
    OrderBookSummaryResponse get_order_book(const std::string& token_id);
    std::vector<OrderBookSummaryResponse> get_order_books(const std::vector<std::string>& token_ids);
    
    // Market data
    MidpointResponse get_midpoint(const std::string& token_id);
    MidpointsResponse get_midpoints(const std::vector<std::string>& token_ids);
    PriceResponse get_price(const std::string& token_id, Side side);
    PricesResponse get_prices(const std::vector<PriceRequest>& requests);
    SpreadResponse get_spread(const std::string& token_id);
    SpreadsResponse get_spreads(const std::vector<std::string>& token_ids);
    LastTradePriceResponse get_last_trade_price(const std::string& token_id);
    std::vector<LastTradesPricesResponse> get_last_trades_prices(const std::vector<std::string>& token_ids);
    
    // Market info
    TickSizeResponse get_tick_size(const std::string& token_id);
    NegRiskResponse get_neg_risk(const std::string& token_id);
    FeeRateResponse get_fee_rate_bps(const std::string& token_id);
    
    // ========== L1 Authenticated Endpoints ==========
    
    ApiCreds create_api_key(std::optional<uint32_t> nonce = std::nullopt);
    ApiCreds derive_api_key(std::optional<uint32_t> nonce = std::nullopt);
    ApiCreds create_or_derive_api_creds(std::optional<uint32_t> nonce = std::nullopt);
    
    // ========== L2 Authenticated Endpoints ==========
    
    // API key management
    void set_api_creds(const ApiCreds& creds);
    ApiKeysResponse get_api_keys();
    BanStatusResponse get_closed_only_mode();
    json delete_api_key();
    
    // Orders
    SignedOrder create_order(
        const OrderArgs& args,
        const CreateOrderOptions& options
    );
    
    SignedOrder create_market_order(
        const MarketOrderArgs& args,
        const CreateOrderOptions& options
    );
    
    PostOrderResponse post_order(const SignedOrder& order, OrderType order_type = OrderType::GTC);
    std::vector<PostOrderResponse> post_orders(const std::vector<std::pair<SignedOrder, OrderType>>& orders);
    PostOrderResponse create_and_post_order(
        const OrderArgs& args,
        const CreateOrderOptions& options
    );
    
    // Order queries
    OpenOrderResponse get_order(const std::string& order_id);
    Page<OpenOrderResponse> get_orders(
        const std::optional<OpenOrderParams>& params = std::nullopt,
        const std::string& next_cursor = INITIAL_CURSOR
    );
    
    // Order cancellation
    CancelOrdersResponse cancel(const std::string& order_id);
    CancelOrdersResponse cancel_orders(const std::vector<std::string>& order_ids);
    CancelOrdersResponse cancel_all();
    CancelOrdersResponse cancel_market_orders(const std::string& market = "", const std::string& asset_id = "");
    
    // Trades
    Page<TradeResponse> get_trades(
        const std::optional<TradeParams>& params = std::nullopt,
        const std::string& next_cursor = INITIAL_CURSOR
    );
    
    // Balance and allowance
    BalanceAllowanceResponse get_balance_allowance(const std::optional<BalanceAllowanceParams>& params = std::nullopt);
    void update_balance_allowance(const std::optional<BalanceAllowanceParams>& params = std::nullopt);
    
    // Notifications
    std::vector<NotificationResponse> get_notifications();
    void drop_notifications();
    
    // Order scoring
    OrderScoringResponse is_order_scoring(const std::string& order_id);
    OrdersScoringResponse are_orders_scoring(const std::vector<std::string>& order_ids);
    
    // Market price calculation
    double calculate_market_price(
        const std::string& token_id,
        Side side,
        double amount,
        OrderType order_type
    );
    
    // ========== Rewards/Earnings API ==========
    
    Page<UserEarningResponse> get_earnings_for_user_for_day(
        const std::string& date,
        const std::string& next_cursor = INITIAL_CURSOR
    );
    
    std::vector<TotalUserEarningResponse> get_total_earnings_for_user_for_day(
        const std::string& date
    );
    
    std::vector<UserRewardsEarningResponse> get_user_earnings_and_markets_config(
        const UserRewardsEarningRequest& request
    );
    
    RewardsPercentagesResponse get_reward_percentages();
    
    Page<CurrentRewardResponse> get_current_rewards(
        const std::string& next_cursor = INITIAL_CURSOR
    );
    
    Page<MarketRewardResponse> get_raw_rewards_for_market(
        const std::string& condition_id,
        const std::string& next_cursor = INITIAL_CURSOR
    );

private:
    std::string host_;
    std::unique_ptr<HttpClient> http_;
    std::shared_ptr<Signer> signer_;
    std::optional<ApiCreds> creds_;
    std::unique_ptr<OrderBuilder> builder_;
    AuthLevel mode_;
    
    // Local caches
    std::unordered_map<std::string, TickSizeResponse> tick_sizes_;
    std::unordered_map<std::string, NegRiskResponse> neg_risk_;
    std::unordered_map<std::string, FeeRateResponse> fee_rates_;
    
    // Helper methods
    void assert_level_1_auth() const;
    void assert_level_2_auth() const;
    AuthLevel get_client_mode() const;
    
    // Header creation
    Headers create_l1_headers(std::optional<uint32_t> nonce = std::nullopt);
    Headers create_l2_headers(
        const std::string& method,
        const std::string& request_path,
        const std::string& body = ""
    );
    
    // Helper to resolve tick size
    TickSizeResponse resolve_tick_size(
        const std::string& token_id,
        const std::optional<std::string>& tick_size = std::nullopt
    );
    
    // Helper to resolve fee rate
    FeeRateResponse resolve_fee_rate(
        const std::string& token_id,
        std::optional<uint32_t> user_fee_rate = std::nullopt
    );
    
public:
    // ========== Low-Latency Optimization Methods ==========
    
    // Pre-warm TCP/TLS connection before trading
    bool warm_connection();
    
    // Start background heartbeat to keep connection alive (default: 25s)
    void start_heartbeat(int interval_seconds = 25);
    
    // Stop background heartbeat
    void stop_heartbeat();
    
    // Check if heartbeat is running
    bool is_heartbeat_running() const;
    
    // Get connection statistics
    ConnectionStats get_connection_stats() const;
};

} // namespace clob

