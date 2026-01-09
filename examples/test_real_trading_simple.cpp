// Example: Complete trading flow
//
// This example demonstrates the full trading workflow:
// 1. Create a signer from private key
// 2. Get/derive API credentials
// 3. Find an active market
// 4. Create and sign an order
// 5. Post the order
// 6. Optionally cancel the order
//
// Requirements:
// - Export PK=your_private_key
// - Have USDC on Polygon in your wallet
// - Have run the approvals example first

#include <clob/client.hpp>
#include <clob/signer.hpp>
#include <iostream>
#include <cstdlib>
#include <algorithm>

std::string tick_size_to_string(clob::TickSize ts) {
    switch (ts) {
        case clob::TickSize::TENTH: return "0.1";
        case clob::TickSize::HUNDREDTH: return "0.01";
        case clob::TickSize::THOUSANDTH: return "0.001";
        case clob::TickSize::TEN_THOUSANDTH: return "0.0001";
        default: return "0.01";
    }
}

int main() {
    try {
        // ========== STEP 1: Get Private Key ==========
        const char* pk_env = std::getenv("PK");
        
        if (!pk_env) {
            std::cerr << "Error: PK environment variable not set" << std::endl;
            std::cerr << "\nYou only need your MetaMask private key!" << std::endl;
            std::cerr << "API credentials will be created automatically.\n" << std::endl;
            std::cerr << "Usage:" << std::endl;
            std::cerr << "  export PK=0xYOUR_PRIVATE_KEY_HERE" << std::endl;
            std::cerr << "  ./trading" << std::endl;
            return 1;
        }
        
        std::string private_key = pk_env;
        
        // ========== STEP 2: Create L1 Client (Just Signer) ==========
        std::cout << "=== Creating L1 Authenticated Client ===" << std::endl;
        auto signer = std::make_shared<clob::Signer>(private_key, clob::POLYGON);
        std::cout << "Your address: " << signer->address() << std::endl;
        
        // Create L1 client (can sign messages but can't post orders yet)
        clob::ClobClient l1_client("https://clob.polymarket.com", signer);
        
        // ========== STEP 3: Create/Derive API Credentials ==========
        std::cout << "\n=== Creating API Credentials (L1 Auth) ===" << std::endl;
        std::cout << "This will sign a message with your private key..." << std::endl;
        
        clob::ApiCreds creds;
        try {
            creds = l1_client.create_api_key();
            std::cout << "Created NEW API credentials" << std::endl;
        } catch (const std::exception& e) {
            std::cout << "API key already exists, deriving..." << std::endl;
            creds = l1_client.derive_api_key();
            std::cout << "Derived EXISTING API credentials" << std::endl;
        }
        
        std::cout << "API Key: " << creds.api_key << std::endl;
        std::cout << "Secret: " << creds.api_secret.substr(0, 10) << "..." << std::endl;
        std::cout << "Passphrase: " << creds.api_passphrase.substr(0, 10) << "..." << std::endl;
        
        // ========== STEP 4: Create L2 Client (With API Creds) ==========
        std::cout << "\n=== Upgrading to L2 Client ===" << std::endl;
        clob::ClobClient client("https://clob.polymarket.com", signer, creds);
        std::cout << "L2 client created (can now post orders)" << std::endl;
        std::cout << "Mode: L" << static_cast<int>(client.get_mode()) << std::endl;
        
        // ========== STEP 5: Find Active Market ==========
        std::cout << "\n=== Finding Active Market ===" << std::endl;
        
        auto markets = client.get_sampling_markets();
        std::cout << "Retrieved " << markets.data.size() << " sampling markets" << std::endl;
        
        clob::MarketResponse selected_market;
        std::string token_id;
        bool found = false;
        
        int checked = 0, with_price = 0, with_orderbook = 0;
        
        for (const auto& market : markets.data) {
            if (!market.tokens.empty()) {
                double price = market.tokens[0].price;
                checked++;
                
                if (price > 0.05 && price < 0.95) {
                    with_price++;
                    
                    try {
                        auto test_book = client.get_order_book(market.tokens[0].token_id);
                        if (!test_book.bids.empty() || !test_book.asks.empty()) {
                            with_orderbook++;
                            selected_market = market;
                            token_id = market.tokens[0].token_id;
                            found = true;
                            
                            std::cout << "\nFound tradeable market:" << std::endl;
                            std::cout << "  " << market.question << std::endl;
                            std::cout << "  Outcome: " << market.tokens[0].outcome << std::endl;
                            std::cout << "  Current price: " << price << std::endl;
                            std::cout << "  Bids: " << test_book.bids.size() << std::endl;
                            std::cout << "  Asks: " << test_book.asks.size() << std::endl;
                            break;
                        }
                    } catch (const std::exception& e) {
                        continue;
                    }
                }
            }
        }
        
        if (!found) {
            std::cerr << "No active markets with orderbooks found" << std::endl;
            return 1;
        }
        
        // ========== STEP 6: Get Market Data ==========
        std::cout << "\n=== Getting Market Data ===" << std::endl;
        
        auto orderbook = client.get_order_book(token_id);
        std::cout << "Bids: " << orderbook.bids.size() << std::endl;
        std::cout << "Asks: " << orderbook.asks.size() << std::endl;
        
        if (!orderbook.bids.empty()) {
            std::cout << "Best bid: $" << orderbook.bids[0].price << " x " << orderbook.bids[0].size << std::endl;
        }
        if (!orderbook.asks.empty()) {
            std::cout << "Best ask: $" << orderbook.asks[0].price << " x " << orderbook.asks[0].size << std::endl;
        }
        
        auto tick_size_resp = client.get_tick_size(token_id);
        auto neg_risk_resp = client.get_neg_risk(token_id);
        std::string tick_size_str = tick_size_to_string(tick_size_resp.minimum_tick_size);
        
        std::cout << "Tick size: " << tick_size_str << std::endl;
        std::cout << "Min order size: " << selected_market.minimum_order_size << std::endl;
        
        // ========== STEP 7: Calculate Order ==========
        std::cout << "\n=== Calculating $1 Order ===" << std::endl;
        
        double target_price;
        if (!orderbook.bids.empty()) {
            double best_bid = std::stod(orderbook.bids[0].price);
            double tick = std::stod(tick_size_str);
            target_price = best_bid - (tick * 2);
            target_price = std::max(target_price, tick * 2);
        } else {
            target_price = 0.10;
        }
        
        double target_size = std::ceil((1.0 / target_price) * 100.0) / 100.0;
        target_size = std::max(target_size, selected_market.minimum_order_size);
        
        double total_cost = target_price * target_size;
        
        std::cout << "Order will be:" << std::endl;
        std::cout << "  BUY " << target_size << " shares of '" << selected_market.tokens[0].outcome << "'" << std::endl;
        std::cout << "  at $" << target_price << " each" << std::endl;
        std::cout << "  Total: $" << total_cost << std::endl;
        std::cout << "  (Order unlikely to fill immediately - safe to cancel)" << std::endl;
        
        // ========== STEP 8: Create and Sign Order ==========
        std::cout << "\n=== Creating Order ===" << std::endl;
        
        clob::OrderArgs order_args;
        order_args.token_id = token_id;
        order_args.price = target_price;
        order_args.size = target_size;
        order_args.side = clob::Side::BUY;
        
        clob::CreateOrderOptions options;
        options.tick_size = tick_size_str;
        options.neg_risk = neg_risk_resp.neg_risk;
        
        auto signed_order = client.create_order(order_args, options);
        std::cout << "Order created and signed" << std::endl;
        
        // ========== STEP 9: POST ORDER ==========
        std::cout << "\n=== READY TO POST ORDER ===" << std::endl;
        std::cout << "This will place a REAL order on Polygon mainnet" << std::endl;
        std::cout << "Market: " << selected_market.question << std::endl;
        std::cout << "Cost: ~$" << total_cost << std::endl;
        std::cout << "\nPress Enter to POST order (Ctrl+C to cancel)..." << std::endl;
        std::cin.get();
        
        std::cout << "Posting order..." << std::endl;
        auto resp = client.post_order(signed_order, clob::OrderType::GTC);
        
        std::cout << "\nORDER POSTED!" << std::endl;
        std::cout << "Order ID: " << resp.order_id << std::endl;
        std::cout << "Success: " << (resp.success ? "YES" : "NO") << std::endl;
            std::cout << "Status: ";
            switch (resp.status) {
            case clob::OrderStatusType::LIVE: std::cout << "LIVE"; break;
            case clob::OrderStatusType::MATCHED: std::cout << "MATCHED"; break;
            case clob::OrderStatusType::DELAYED: std::cout << "DELAYED"; break;
                default: std::cout << "OTHER"; break;
            }
            std::cout << std::endl;
            
            if (resp.error_msg.has_value()) {
                std::cout << "Error: " << *resp.error_msg << std::endl;
            }
            
        // ========== STEP 10: Verify and Cancel ==========
        if (resp.success && !resp.order_id.empty()) {
                std::cout << "\n=== Verifying Order ===" << std::endl;
                auto order_details = client.get_order(resp.order_id);
            std::cout << "Order confirmed in system:" << std::endl;
                std::cout << "  Price: $" << order_details.price << std::endl;
                std::cout << "  Size: " << order_details.original_size << " shares" << std::endl;
                std::cout << "  Matched: " << order_details.size_matched << " shares" << std::endl;
                
                std::cout << "\n=== Cancel Order? ===" << std::endl;
                std::cout << "Cancel order " << resp.order_id << "? (y/N): ";
                std::string cancel_choice;
                std::getline(std::cin, cancel_choice);
                
                if (cancel_choice == "y" || cancel_choice == "Y") {
                    std::cout << "Canceling..." << std::endl;
                    auto cancel_resp = client.cancel(resp.order_id);
                    
                    if (!cancel_resp.canceled.empty()) {
                    std::cout << "Order CANCELED successfully!" << std::endl;
                    } else if (!cancel_resp.not_canceled.empty()) {
                    std::cout << "Could not cancel:" << std::endl;
                        for (const auto& [id, reason] : cancel_resp.not_canceled) {
                            std::cout << "  " << reason << std::endl;
                        }
                    }
                } else {
                    std::cout << "Order left open. Cancel manually at:" << std::endl;
                    std::cout << "  https://polymarket.com/activity" << std::endl;
            }
        }
        
        std::cout << "\n=== TEST COMPLETE ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
