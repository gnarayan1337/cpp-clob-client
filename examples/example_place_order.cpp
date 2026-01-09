#include <clob/client.hpp>
#include <clob/signer.hpp>
#include <iostream>
#include <cstdlib>

int main() {
    try {
        // Get private key from environment
        const char* pk_env = std::getenv("PK");
        if (!pk_env) {
            std::cerr << "Error: PK environment variable not set" << std::endl;
            return 1;
        }
        
        std::string private_key = pk_env;
        
        // Create signer
        auto signer = std::make_shared<clob::Signer>(private_key, clob::POLYGON);
        
        // Get API creds from environment (or create them first)
        const char* api_key_env = std::getenv("API_KEY");
        const char* api_secret_env = std::getenv("API_SECRET");
        const char* api_passphrase_env = std::getenv("API_PASSPHRASE");
        
        clob::ApiCreds creds;
        if (api_key_env && api_secret_env && api_passphrase_env) {
            creds.api_key = api_key_env;
            creds.api_secret = api_secret_env;
            creds.api_passphrase = api_passphrase_env;
        } else {
            // Create client to get creds
            clob::ClobClient temp_client("https://clob.polymarket.com", signer);
            creds = temp_client.create_or_derive_api_creds();
            std::cout << "Created API credentials" << std::endl;
        }
        
        // Create L2 authenticated client
        clob::ClobClient client("https://clob.polymarket.com", signer, creds);
        
        // Example token ID (make sure to use a valid one)
        std::string token_id = "21742633143463906290569050155826241533067272736897614950488156847949938836455";
        
        // Get market info
        auto tick_size = client.get_tick_size(token_id);
        auto neg_risk = client.get_neg_risk(token_id);
        
        std::cout << "Token ID: " << token_id << std::endl;
        std::cout << "Tick size: 0.01" << std::endl;
        std::cout << "Neg risk: " << (neg_risk.neg_risk ? "true" : "false") << std::endl;
        
        // Create a limit order
        clob::OrderArgs order_args;
        order_args.token_id = token_id;
        order_args.price = 0.50;  // 50 cents
        order_args.size = 10.0;   // 10 shares
        order_args.side = clob::Side::BUY;
        
        clob::CreateOrderOptions options;
        options.tick_size = "0.01";  // Use string directly
        options.neg_risk = neg_risk.neg_risk;
        
        std::cout << "\n=== Creating Order ===" << std::endl;
        auto signed_order = client.create_order(order_args, options);
        
        std::cout << "Order created:" << std::endl;
        std::cout << "  Maker: " << signed_order.order.maker << std::endl;
        std::cout << "  Token ID: " << signed_order.order.token_id << std::endl;
        std::cout << "  Side: " << (signed_order.order.side == 0 ? "BUY" : "SELL") << std::endl;
        std::cout << "  Maker amount: " << signed_order.order.maker_amount << std::endl;
        std::cout << "  Taker amount: " << signed_order.order.taker_amount << std::endl;
        
        // Post the order (uncomment to actually post)
        // std::cout << "\n=== Posting Order ===" << std::endl;
        // auto result = client.post_order(signed_order, clob::OrderType::GTC);
        // std::cout << "Order posted: " << result.dump(2) << std::endl;
        
        std::cout << "\nOrder created successfully (not posted)" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}


