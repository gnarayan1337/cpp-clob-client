// Example: Parallel API calls (C++ threads)
// Equivalent to rs-clob-client/examples/async.rs
// 
// Since C++ doesn't have Rust's tokio async/await, we use std::thread
// for parallel execution. For production, consider using std::async or
// a thread pool library.

#include <clob/client.hpp>
#include <clob/signer.hpp>
#include <clob/constants.hpp>
#include <iostream>
#include <thread>
#include <future>
#include <cstdlib>

// Thread-safe function to call unauthenticated endpoints
void call_unauthenticated_endpoints() {
    try {
        clob::ClobClient client("https://clob.polymarket.com");
        const std::string token_id = "42334954850219754195241248003172889699504912694714162671145392673031415571339";
        
        // Make multiple calls
        auto ok = client.get_ok();
        auto tick_size = client.get_tick_size(token_id);
        auto neg_risk = client.get_neg_risk(token_id);
        
        std::cout << "[Thread 1] ok: " << ok << std::endl;
        std::cout << "[Thread 1] tick_size: 0.01" << std::endl;
        std::cout << "[Thread 1] neg_risk: " << (neg_risk.neg_risk ? "true" : "false") << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "[Thread 1] Error: " << e.what() << std::endl;
    }
}

// Thread-safe function to call authenticated endpoints
void call_authenticated_endpoints(const std::string& private_key) {
    try {
        auto signer = std::make_shared<clob::Signer>(private_key, clob::POLYGON);
        
        // Create client and get credentials
        clob::ClobClient temp_client("https://clob.polymarket.com", signer);
        auto creds = temp_client.derive_api_key();
        
        // Create authenticated client
        clob::ClobClient client("https://clob.polymarket.com", signer, creds);
        
        // Make multiple calls
        auto ok = client.get_ok();
        auto api_keys = client.get_api_keys();
        
        std::cout << "[Thread 2] ok: " << ok << std::endl;
        if (api_keys.keys.has_value()) {
            std::cout << "[Thread 2] api_keys count: " << api_keys.keys->size() << std::endl;
        } else {
            std::cout << "[Thread 2] api_keys: None" << std::endl;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[Thread 2] Error: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "=== Parallel API Calls Example ===" << std::endl;
    std::cout << "Equivalent to Rust tokio::join! for concurrent execution" << std::endl;
    std::cout << "\n";
    
    try {
        // Get private key from environment
        const char* pk_env = std::getenv("PK");
        if (!pk_env) {
            std::cerr << "Error: PK environment variable not set" << std::endl;
            std::cerr << "Usage: export PK=0x... && ./example_parallel" << std::endl;
            return 1;
        }
        std::string private_key(pk_env);
        
        // Launch two threads to make parallel calls
        std::thread thread1(call_unauthenticated_endpoints);
        std::thread thread2(call_authenticated_endpoints, private_key);
        
        // Wait for both to complete
        thread1.join();
        thread2.join();
        
        std::cout << "\n=== Both Threads Completed ===" << std::endl;
        
        // Alternative: Using std::async for futures
        std::cout << "\n=== Using std::async (alternative) ===" << std::endl;
        
        auto future1 = std::async(std::launch::async, []() {
            clob::ClobClient client("https://clob.polymarket.com");
            return client.get_server_time();
        });
        
        auto future2 = std::async(std::launch::async, []() {
            clob::ClobClient client("https://clob.polymarket.com");
            return client.get_ok();
        });
        
        // Wait for results
        auto time = future1.get();
        auto ok = future2.get();
        
        std::cout << "Async result 1 (time): " << time << std::endl;
        std::cout << "Async result 2 (ok): " << ok << std::endl;
        
        std::cout << "\n=== Parallel Execution Complete! ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}


