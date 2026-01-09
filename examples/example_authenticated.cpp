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
            std::cerr << "Usage: export PK=your_private_key_hex" << std::endl;
            return 1;
        }
        
        std::string private_key = pk_env;
        
        // Create signer (Polygon Mainnet)
        auto signer = std::make_shared<clob::Signer>(private_key, clob::POLYGON);
        std::cout << "Address: " << signer->address() << std::endl;
        
        // Create L1 authenticated client
        clob::ClobClient client("https://clob.polymarket.com", signer);
        std::cout << "Client address: " << client.get_address() << std::endl;
        
        // Create or derive API credentials
        std::cout << "\n=== Creating/Deriving API Key ===" << std::endl;
        auto creds = client.create_or_derive_api_creds();
        std::cout << "API Key: " << creds.api_key << std::endl;
        
        // Set credentials to elevate to L2
        client.set_api_creds(creds);
        
        // Now we can use L2 endpoints
        std::cout << "\n=== Getting API Keys ===" << std::endl;
        auto api_keys = client.get_api_keys();
        if (api_keys.keys.has_value()) {
            std::cout << "API Keys: " << api_keys.keys->size() << " keys" << std::endl;
        } else {
            std::cout << "API Keys: None" << std::endl;
        }
        
        // Get open orders
        std::cout << "\n=== Getting Open Orders ===" << std::endl;
        auto orders = client.get_orders();
        std::cout << "Open orders: " << orders.count << " orders" << std::endl;
        
        // Get balance and allowance
        std::cout << "\n=== Getting Balance/Allowance ===" << std::endl;
        auto balance = client.get_balance_allowance();
        std::cout << "Balance: " << balance.balance << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}


