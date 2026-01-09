#pragma once

#include <string>
#include <optional>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <nlohmann/json.hpp>
#include <simdjson.h>

// Forward declare httplib types to avoid including in header
namespace httplib {
    class SSLClient;
    class Client;
}

namespace clob {

using json = nlohmann::json;
using Headers = std::unordered_map<std::string, std::string>;

// Connection statistics for monitoring performance
struct ConnectionStats {
    uint64_t total_requests = 0;
    uint64_t reused_connections = 0;
    double avg_latency_ms = 0.0;
    double last_latency_ms = 0.0;
    bool connection_warm = false;
};

class HttpClient {
public:
    explicit HttpClient(const std::string& host);
    ~HttpClient();
    
    // Disable copy (unique persistent connection)
    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;
    
    // Enable move
    HttpClient(HttpClient&&) noexcept;
    HttpClient& operator=(HttpClient&&) noexcept;
    
    // ========== HTTP Methods - SIMD JSON Interface ==========
    
    // Returns simdjson::dom::element for fast parsing
    simdjson::dom::element get_simd(
        const std::string& path,
        const std::optional<Headers>& headers = std::nullopt,
        const std::optional<json>& params = std::nullopt
    );
    
    simdjson::dom::element post_simd(
        const std::string& path,
        const std::optional<json>& data = std::nullopt,
        const std::optional<Headers>& headers = std::nullopt
    );
    
    simdjson::dom::element del_simd(
        const std::string& path,
        const std::optional<json>& data = std::nullopt,
        const std::optional<Headers>& headers = std::nullopt
    );
    
    // ========== Legacy JSON Methods (for backward compatibility) ==========
    
    json get(
        const std::string& path,
        const std::optional<Headers>& headers = std::nullopt,
        const std::optional<json>& params = std::nullopt
    );
    
    json post(
        const std::string& path,
        const std::optional<json>& data = std::nullopt,
        const std::optional<Headers>& headers = std::nullopt
    );
    
    json del(
        const std::string& path,
        const std::optional<json>& data = std::nullopt,
        const std::optional<Headers>& headers = std::nullopt
    );
    
    // ========== Templated Typed Methods ==========
    
    template<typename T>
    T get_typed(
        const std::string& path,
        const std::optional<Headers>& headers = std::nullopt,
        const std::optional<json>& params = std::nullopt
    ) {
        json response = get(path, headers, params);
        return response.get<T>();
    }
    
    template<typename T>
    T post_typed(
        const std::string& path,
        const std::optional<json>& data = std::nullopt,
        const std::optional<Headers>& headers = std::nullopt
    ) {
        json response = post(path, data, headers);
        return response.get<T>();
    }
    
    template<typename T>
    T del_typed(
        const std::string& path,
        const std::optional<json>& data = std::nullopt,
        const std::optional<Headers>& headers = std::nullopt
    ) {
        json response = del(path, data, headers);
        return response.get<T>();
    }
    
    // ========== Low-Latency Optimizations ==========
    
    // Pre-warm TCP/TLS connection (call before trading)
    bool warm_connection();
    
    // Start background heartbeat to keep connection alive
    // Default: 25 seconds (servers typically timeout at 30-60s)
    void start_heartbeat(int interval_seconds = 25);
    
    // Stop background heartbeat
    void stop_heartbeat();
    
    // Check if heartbeat is running
    bool is_heartbeat_running() const;
    
    // Get connection statistics
    ConnectionStats get_stats() const;
    
    // ========== Getters ==========
    
    std::string get_host() const { return host_; }

private:
    // Persistent HTTP clients (reused for all requests)
    std::unique_ptr<httplib::SSLClient> ssl_client_;
    std::unique_ptr<httplib::Client> client_;
    
    // SIMD JSON parser (reusable for performance)
    simdjson::dom::parser parser_;
    std::string parser_buffer_;  // Reusable buffer
    
    // Connection info
    std::string host_;
    std::string scheme_;
    std::string host_only_;
    int port_;
    
    // Thread safety
    mutable std::mutex client_mutex_;
    mutable std::mutex stats_mutex_;
    
    // Background heartbeat
    std::atomic<bool> heartbeat_running_;
    std::thread heartbeat_thread_;
    
    // Connection statistics
    uint64_t total_requests_;
    uint64_t reused_connections_;
    double total_latency_ms_;
    double last_latency_ms_;
    bool connection_warm_;
    
    // Helper methods
    void init_client();
    void parse_host();
    std::string build_query_string(const json& params) const;
    
    // Internal HTTP methods with timing
    std::string execute_get(
        const std::string& path,
        const std::optional<Headers>& headers,
        const std::optional<json>& params
    );
    
    std::string execute_post(
        const std::string& path,
        const std::optional<json>& data,
        const std::optional<Headers>& headers
    );
    
    std::string execute_del(
        const std::string& path,
        const std::optional<json>& data,
        const std::optional<Headers>& headers
    );
    
    // Update stats
    void update_stats(double latency_ms);
};

} // namespace clob
