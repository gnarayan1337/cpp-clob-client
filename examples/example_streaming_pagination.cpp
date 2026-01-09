// Example: Manual pagination (equivalent to streaming)
// Equivalent to rs-clob-client/examples/streaming.rs
//
// Since C++ doesn't have async streams like Rust's Stream trait,
// we implement manual pagination using next_cursor from Page<T> responses.
// This achieves the same result - iterating through all pages of data.

#include <clob/client.hpp>
#include <clob/signer.hpp>
#include <clob/constants.hpp>
#include <iostream>
#include <cstdlib>

// Helper function to paginate through all markets
void stream_sampling_markets(clob::ClobClient& client) {
    std::cout << "=== Streaming Sampling Markets (Manual Pagination) ===" << std::endl;
    
    std::string cursor = clob::INITIAL_CURSOR;
    int page_num = 0;
    int total_markets = 0;
    
    while (true) {
        auto page = client.get_sampling_markets(cursor);
        
        page_num++;
        total_markets += page.data.size();
        
        std::cout << "Page " << page_num << ":" << std::endl;
        std::cout << "  Markets in page: " << page.data.size() << std::endl;
        std::cout << "  Total so far: " << total_markets << std::endl;
        
        // Show first market from this page
        if (!page.data.empty()) {
            const auto& market = page.data[0];
            std::cout << "  First market: " << market.question << std::endl;
        }
        
        // Check if there are more pages
        if (page.next_cursor == "LTE=" || page.next_cursor == clob::INITIAL_CURSOR) {
            std::cout << "  No more pages" << std::endl;
            break;
        }
        
        cursor = page.next_cursor;
        
        // Safety limit to avoid infinite loop
        if (page_num >= 10) {
            std::cout << "  Stopping after 10 pages (safety limit)" << std::endl;
            break;
        }
    }
    
    std::cout << "Total markets retrieved: " << total_markets << std::endl;
}

// Helper function to paginate through authenticated data (trades)
void stream_trades(clob::ClobClient& client) {
    std::cout << "\n=== Streaming Trades (Manual Pagination) ===" << std::endl;
    
    std::string cursor = clob::INITIAL_CURSOR;
    int page_num = 0;
    int total_trades = 0;
    
    while (true) {
        // Get trades with optional filtering
        clob::TradeParams params;
        // params.market = "market-id"; // Optional filter
        
        auto page = client.get_trades(params, cursor);
        
        page_num++;
        total_trades += page.data.size();
        
        std::cout << "Page " << page_num << ":" << std::endl;
        std::cout << "  Trades in page: " << page.data.size() << std::endl;
        std::cout << "  Total so far: " << total_trades << std::endl;
        
        // Show first trade from this page
        if (!page.data.empty()) {
            const auto& trade = page.data[0];
            std::cout << "  First trade ID: " << trade.id << std::endl;
            std::cout << "  Market: " << trade.market << std::endl;
        }
        
        // Check if there are more pages
        if (page.next_cursor == "LTE=" || page.next_cursor == clob::INITIAL_CURSOR) {
            std::cout << "  No more pages" << std::endl;
            break;
        }
        
        cursor = page.next_cursor;
        
        // Safety limit
        if (page_num >= 5) {
            std::cout << "  Stopping after 5 pages (safety limit)" << std::endl;
            break;
        }
    }
    
    std::cout << "Total trades retrieved: " << total_trades << std::endl;
}

// Helper function to paginate through rewards
void stream_current_rewards(clob::ClobClient& client) {
    std::cout << "\n=== Streaming Current Rewards (Manual Pagination) ===" << std::endl;
    
    std::string cursor = clob::INITIAL_CURSOR;
    int page_num = 0;
    int total_rewards = 0;
    
    while (true) {
        auto page = client.get_current_rewards(cursor);
        
        page_num++;
        total_rewards += page.data.size();
        
        std::cout << "Page " << page_num << ":" << std::endl;
        std::cout << "  Rewards in page: " << page.data.size() << std::endl;
        std::cout << "  Total so far: " << total_rewards << std::endl;
        
        // Show first reward from this page
        if (!page.data.empty()) {
            const auto& reward = page.data[0];
            std::cout << "  Market: " << reward.market << std::endl;
            std::cout << "  Daily rate: " << reward.rewards_daily_rate << std::endl;
        }
        
        // Check if there are more pages
        if (page.next_cursor == "LTE=" || page.next_cursor == clob::INITIAL_CURSOR) {
            std::cout << "  No more pages" << std::endl;
            break;
        }
        
        cursor = page.next_cursor;
        
        // Safety limit
        if (page_num >= 5) {
            std::cout << "  Stopping after 5 pages (safety limit)" << std::endl;
            break;
        }
    }
    
    std::cout << "Total rewards retrieved: " << total_rewards << std::endl;
}

int main() {
    std::cout << "=== Manual Pagination Example ===" << std::endl;
    std::cout << "Equivalent to Rust streaming (without async streams)" << std::endl;
    std::cout << "\n";
    
    try {
        // Part 1: Unauthenticated streaming (public data)
        clob::ClobClient public_client("https://clob.polymarket.com");
        stream_sampling_markets(public_client);
        
        // Part 2: Authenticated streaming (requires credentials)
        const char* pk_env = std::getenv("PK");
        if (pk_env) {
            std::string private_key(pk_env);
            auto signer = std::make_shared<clob::Signer>(private_key, clob::POLYGON);
            
            clob::ClobClient temp_client("https://clob.polymarket.com", signer);
            auto creds = temp_client.derive_api_key();
            
            clob::ClobClient auth_client("https://clob.polymarket.com", signer, creds);
            
            stream_trades(auth_client);
            stream_current_rewards(auth_client);
        } else {
            std::cout << "\n=== Skipping Authenticated Streaming ===" << std::endl;
            std::cout << "Set PK environment variable to test authenticated streaming" << std::endl;
        }
        
        std::cout << "\n=== All Pagination Examples Complete! ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}


