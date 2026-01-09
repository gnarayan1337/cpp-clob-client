// Example: Public (unauthenticated) endpoints
// No API key required - anyone can query market data

#include <clob/client.hpp>
#include <iostream>

std::string tick_size_to_string(clob::TickSize ts) {
    switch (ts) {
        case clob::TickSize::TENTH: return "0.1";
        case clob::TickSize::HUNDREDTH: return "0.01";
        case clob::TickSize::THOUSANDTH: return "0.001";
        case clob::TickSize::TEN_THOUSANDTH: return "0.0001";
        default: return "unknown";
    }
}

int main() {
    try {
        // Create an unauthenticated client (L0)
        clob::ClobClient client("https://clob.polymarket.com");
        
        // Get server status
        auto ok = client.get_ok();
        std::cout << "Server OK: " << ok << std::endl;
        
        // Get server time
        auto time = client.get_server_time();
        std::cout << "Server time: " << time << std::endl;
        
        // Get markets (paginated response)
        std::cout << "\n=== Getting Markets ===" << std::endl;
        auto markets = client.get_markets();
        std::cout << "Number of markets: " << markets.data.size() << std::endl;
        std::cout << "Next cursor: " << markets.next_cursor << std::endl;
        
        // Find a live market with tokens
        std::string token_id;
        if (!markets.data.empty()) {
            std::cout << "First market condition_id: " << markets.data[0].condition_id << std::endl;
            std::cout << "First market question: " << markets.data[0].question << std::endl;
            std::cout << "First market active: " << (markets.data[0].active ? "Yes" : "No") << std::endl;
            std::cout << "First market closed: " << (markets.data[0].closed ? "Yes" : "No") << std::endl;
        
            // Find first active market with tokens
            for (const auto& market : markets.data) {
                if (market.active && !market.closed && !market.tokens.empty() && market.enable_order_book) {
                    token_id = market.tokens[0].token_id;
                    std::cout << "\nFound active market: " << market.question << std::endl;
                    std::cout << "Token ID: " << token_id << std::endl;
                    break;
                }
            }
        }
        
        if (token_id.empty()) {
            std::cout << "\nNo active markets with tokens found, using first token from any market" << std::endl;
            for (const auto& market : markets.data) {
                if (!market.tokens.empty()) {
                    token_id = market.tokens[0].token_id;
                    break;
                }
            }
        }
        
        // Get simplified markets
        std::cout << "\n=== Getting Simplified Markets ===" << std::endl;
        auto simplified = client.get_simplified_markets();
        std::cout << "Number of simplified markets: " << simplified.data.size() << std::endl;
        if (!simplified.data.empty()) {
            std::cout << "First simplified market:" << std::endl;
            std::cout << "  Condition ID: " << simplified.data[0].condition_id << std::endl;
            std::cout << "  Active: " << (simplified.data[0].active ? "Yes" : "No") << std::endl;
            std::cout << "  Tokens: " << simplified.data[0].tokens.size() << std::endl;
        }
        
        // If we have a token, try to get its data
        if (!token_id.empty()) {
            std::cout << "\n=== Testing Token Data ===" << std::endl;
            std::cout << "Token ID: " << token_id << std::endl;
            
            try {
        auto orderbook = client.get_order_book(token_id);
                std::cout << "Orderbook:" << std::endl;
                std::cout << "   Asset ID: " << orderbook.asset_id << std::endl;
                std::cout << "   Market: " << orderbook.market << std::endl;
                std::cout << "   Bids: " << orderbook.bids.size() << std::endl;
                std::cout << "   Asks: " << orderbook.asks.size() << std::endl;
            } catch (const std::exception& e) {
                std::cout << "Orderbook unavailable (market may be closed): " << e.what() << std::endl;
            }
            
            try {
                auto tick_size_resp = client.get_tick_size(token_id);
                std::cout << "Tick size: " << tick_size_to_string(tick_size_resp.minimum_tick_size) << std::endl;
            } catch (const std::exception& e) {
                std::cout << "Tick size unavailable: " << e.what() << std::endl;
            }
            
            try {
                auto neg_risk = client.get_neg_risk(token_id);
                std::cout << "Neg risk: " << (neg_risk.neg_risk ? "true" : "false") << std::endl;
            } catch (const std::exception& e) {
                std::cout << "Neg risk unavailable: " << e.what() << std::endl;
            }
            
            try {
                auto fee_rate = client.get_fee_rate_bps(token_id);
                std::cout << "Fee rate (bps): " << fee_rate.base_fee << std::endl;
            } catch (const std::exception& e) {
                std::cout << "Fee rate unavailable: " << e.what() << std::endl;
            }
        }
        
        std::cout << "\n=== Success ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
