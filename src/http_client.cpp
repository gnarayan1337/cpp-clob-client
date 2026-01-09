#include "clob/http_client.hpp"
#include <httplib.h>
#include <stdexcept>
#include <sstream>
#include <chrono>

namespace clob {

HttpClient::HttpClient(const std::string& host) 
    : host_(host)
    , heartbeat_running_(false)
    , total_requests_(0)
    , reused_connections_(0)
    , total_latency_ms_(0.0)
    , last_latency_ms_(0.0)
    , connection_warm_(false)
{
    parse_host();
    init_client();
}

HttpClient::~HttpClient() {
    stop_heartbeat();
}

HttpClient::HttpClient(HttpClient&& other) noexcept
    : ssl_client_(std::move(other.ssl_client_))
    , client_(std::move(other.client_))
    , parser_(std::move(other.parser_))
    , parser_buffer_(std::move(other.parser_buffer_))
    , host_(std::move(other.host_))
    , scheme_(std::move(other.scheme_))
    , host_only_(std::move(other.host_only_))
    , port_(other.port_)
    , heartbeat_running_(other.heartbeat_running_.load())
    , total_requests_(other.total_requests_)
    , reused_connections_(other.reused_connections_)
    , total_latency_ms_(other.total_latency_ms_)
    , last_latency_ms_(other.last_latency_ms_)
    , connection_warm_(other.connection_warm_)
{
    other.stop_heartbeat();
}

HttpClient& HttpClient::operator=(HttpClient&& other) noexcept {
    if (this != &other) {
        stop_heartbeat();
        other.stop_heartbeat();
        
        ssl_client_ = std::move(other.ssl_client_);
        client_ = std::move(other.client_);
        parser_ = std::move(other.parser_);
        parser_buffer_ = std::move(other.parser_buffer_);
        host_ = std::move(other.host_);
        scheme_ = std::move(other.scheme_);
        host_only_ = std::move(other.host_only_);
        port_ = other.port_;
        heartbeat_running_ = other.heartbeat_running_.load();
        total_requests_ = other.total_requests_;
        reused_connections_ = other.reused_connections_;
        total_latency_ms_ = other.total_latency_ms_;
        last_latency_ms_ = other.last_latency_ms_;
        connection_warm_ = other.connection_warm_;
    }
    return *this;
}

void HttpClient::parse_host() {
    // Parse scheme and host
    size_t scheme_pos = host_.find("://");
    if (scheme_pos != std::string::npos) {
        scheme_ = host_.substr(0, scheme_pos);
        std::string remainder = host_.substr(scheme_pos + 3);
        
        // Remove path component
        size_t path_pos = remainder.find("/");
        if (path_pos != std::string::npos) {
            remainder = remainder.substr(0, path_pos);
        }
        
        // Parse port
        size_t port_pos = remainder.find(":");
        if (port_pos != std::string::npos) {
            host_only_ = remainder.substr(0, port_pos);
            std::string port_str = remainder.substr(port_pos + 1);
            try {
                port_ = std::stoi(port_str);
            } catch (...) {
                port_ = (scheme_ == "https") ? 443 : 80;
            }
        } else {
            host_only_ = remainder;
            port_ = (scheme_ == "https") ? 443 : 80;
        }
    } else {
        scheme_ = "https";
        host_only_ = host_;
        port_ = 443;
    }
}

void HttpClient::init_client() {
    if (scheme_ == "https") {
        ssl_client_ = std::make_unique<httplib::SSLClient>(host_only_, port_);
        
        // LOW-LATENCY OPTIMIZATIONS
        
        // 1. Enable HTTP keep-alive (reuse connection)
        ssl_client_->set_keep_alive(true);
        
        // 2. Set connection timeout
        ssl_client_->set_connection_timeout(5);  // 5 seconds
        ssl_client_->set_read_timeout(10);       // 10 seconds
        ssl_client_->set_write_timeout(10);      // 10 seconds
        
        // 3. Enable SSL certificate verification for security
        ssl_client_->enable_server_certificate_verification(true);
        
        // 4. Set follow redirects
        ssl_client_->set_follow_location(true);
        
        // 5. Set TCP_NODELAY (disable Nagle's algorithm)
        // Note: cpp-httplib enables this by default
        
        // 6. Set default headers with keep-alive
        ssl_client_->set_default_headers({
            {"Connection", "keep-alive"},
            {"Keep-Alive", "timeout=60, max=1000"},
            {"Accept", "application/json"},
            {"Content-Type", "application/json"},
            {"User-Agent", "cpp-clob-client/1.0"}
        });
        
    } else {
        client_ = std::make_unique<httplib::Client>(host_only_, port_);
        
        // Same optimizations for HTTP
        client_->set_keep_alive(true);
        client_->set_connection_timeout(5);
        client_->set_read_timeout(10);
        client_->set_write_timeout(10);
        client_->set_follow_location(true);
        
        client_->set_default_headers({
            {"Connection", "keep-alive"},
            {"Keep-Alive", "timeout=60, max=1000"},
            {"Accept", "application/json"},
            {"Content-Type", "application/json"},
            {"User-Agent", "cpp-clob-client/1.0"}
        });
    }
}

std::string HttpClient::build_query_string(const json& params) const {
    if (params.empty()) {
        return "";
    }
    
    std::ostringstream oss;
    bool first = true;
    
    for (auto it = params.begin(); it != params.end(); ++it) {
        if (!first) oss << "&";
        first = false;
        
        oss << it.key() << "=";
        if (it.value().is_string()) {
            oss << it.value().get<std::string>();
        } else if (it.value().is_number()) {
            oss << it.value().dump();
        } else {
            oss << it.value().dump();
        }
    }
    
    return oss.str();
}

void HttpClient::update_stats(double latency_ms) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    total_requests_++;
    total_latency_ms_ += latency_ms;
    last_latency_ms_ = latency_ms;
    
    // Check if connection was reused (latency < 40ms typically means reuse)
    if (total_requests_ > 1 && latency_ms < 40.0) {
        reused_connections_++;
    }
}

std::string HttpClient::execute_get(
    const std::string& path,
    const std::optional<Headers>& headers,
    const std::optional<json>& params
) {
    std::lock_guard<std::mutex> lock(client_mutex_);
    
    // Build full path with query params
    std::string full_path = path;
    if (params.has_value() && !params->empty()) {
        full_path += "?" + build_query_string(*params);
    }
    
    // Prepare headers
    httplib::Headers req_headers;
    if (headers.has_value()) {
        for (const auto& [key, value] : *headers) {
            req_headers.emplace(key, value);
        }
    }
    
    // Execute with timing
    auto start = std::chrono::high_resolution_clock::now();
    
    httplib::Result res;
    if (ssl_client_) {
        res = ssl_client_->Get(full_path, req_headers);
    } else {
        res = client_->Get(full_path, req_headers);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double latency_ms = duration.count() / 1000.0;
    
    update_stats(latency_ms);
    
    if (!res) {
        throw std::runtime_error("HTTP request failed: " + httplib::to_string(res.error()));
    }
    
    if (res->status < 200 || res->status >= 300) {
        throw std::runtime_error("HTTP error " + std::to_string(res->status) + ": " + res->body);
    }
    
    return res->body;
}

std::string HttpClient::execute_post(
    const std::string& path,
    const std::optional<json>& data,
    const std::optional<Headers>& headers
) {
    std::lock_guard<std::mutex> lock(client_mutex_);
    
    // Prepare headers
    httplib::Headers req_headers;
    if (headers.has_value()) {
        for (const auto& [key, value] : *headers) {
            req_headers.emplace(key, value);
        }
    }
    
    // Prepare body
    std::string body = data.has_value() ? data->dump() : "{}";
    
    // Execute with timing
    auto start = std::chrono::high_resolution_clock::now();
    
    httplib::Result res;
    if (ssl_client_) {
        res = ssl_client_->Post(path, req_headers, body, "application/json");
    } else {
        res = client_->Post(path, req_headers, body, "application/json");
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double latency_ms = duration.count() / 1000.0;
    
    update_stats(latency_ms);
    
    if (!res) {
        throw std::runtime_error("HTTP request failed: " + httplib::to_string(res.error()));
    }
    
    if (res->status < 200 || res->status >= 300) {
        throw std::runtime_error("HTTP error " + std::to_string(res->status) + ": " + res->body);
    }
    
    return res->body;
}

std::string HttpClient::execute_del(
    const std::string& path,
    const std::optional<json>& data,
    const std::optional<Headers>& headers
) {
    std::lock_guard<std::mutex> lock(client_mutex_);
    
    // Prepare headers
    httplib::Headers req_headers;
    if (headers.has_value()) {
        for (const auto& [key, value] : *headers) {
            req_headers.emplace(key, value);
        }
    }
    
    // Prepare body
    std::string body = data.has_value() ? data->dump() : "{}";
    
    // Execute with timing
    auto start = std::chrono::high_resolution_clock::now();
    
    httplib::Result res;
    if (ssl_client_) {
        res = ssl_client_->Delete(path, req_headers, body, "application/json");
    } else {
        res = client_->Delete(path, req_headers, body, "application/json");
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    double latency_ms = duration.count() / 1000.0;
    
    update_stats(latency_ms);
    
    if (!res) {
        throw std::runtime_error("HTTP request failed: " + httplib::to_string(res.error()));
    }
    
    if (res->status < 200 || res->status >= 300) {
        throw std::runtime_error("HTTP error " + std::to_string(res->status) + ": " + res->body);
    }
    
    return res->body;
}

// ========== SIMD JSON Methods (High Performance) ==========

simdjson::dom::element HttpClient::get_simd(
    const std::string& path,
    const std::optional<Headers>& headers,
    const std::optional<json>& params
) {
    std::string response_body = execute_get(path, headers, params);
    
    // Parse with SIMD JSON (20x faster than nlohmann)
    parser_buffer_ = response_body;  // Copy to reusable buffer
    auto doc = parser_.parse(parser_buffer_);
    if (doc.error()) {
        throw std::runtime_error("SIMD JSON parse error: " + std::string(simdjson::error_message(doc.error())));
    }
    
    return doc.value();
}

simdjson::dom::element HttpClient::post_simd(
    const std::string& path,
    const std::optional<json>& data,
    const std::optional<Headers>& headers
) {
    std::string response_body = execute_post(path, data, headers);
    
    parser_buffer_ = response_body;
    auto doc = parser_.parse(parser_buffer_);
    if (doc.error()) {
        throw std::runtime_error("SIMD JSON parse error: " + std::string(simdjson::error_message(doc.error())));
    }
    
    return doc.value();
}

simdjson::dom::element HttpClient::del_simd(
    const std::string& path,
    const std::optional<json>& data,
    const std::optional<Headers>& headers
) {
    std::string response_body = execute_del(path, data, headers);
    
    parser_buffer_ = response_body;
    auto doc = parser_.parse(parser_buffer_);
    if (doc.error()) {
        throw std::runtime_error("SIMD JSON parse error: " + std::string(simdjson::error_message(doc.error())));
    }
    
    return doc.value();
}

// ========== Legacy JSON Methods (Backward Compatibility) ==========

json HttpClient::get(
    const std::string& path,
    const std::optional<Headers>& headers,
    const std::optional<json>& params
) {
    std::string response_body = execute_get(path, headers, params);
    return json::parse(response_body);
}

json HttpClient::post(
    const std::string& path,
    const std::optional<json>& data,
    const std::optional<Headers>& headers
) {
    std::string response_body = execute_post(path, data, headers);
    return json::parse(response_body);
}

json HttpClient::del(
    const std::string& path,
    const std::optional<json>& data,
    const std::optional<Headers>& headers
) {
    std::string response_body = execute_del(path, data, headers);
    return json::parse(response_body);
}

// ========== Low-Latency Optimizations ==========

bool HttpClient::warm_connection() {
    try {
        // Hit a cheap endpoint to establish TCP/TLS connection
        execute_get("/ok", std::nullopt, std::nullopt);
        
        std::lock_guard<std::mutex> lock(stats_mutex_);
        connection_warm_ = true;
        return true;
    } catch (...) {
        return false;
    }
}

void HttpClient::start_heartbeat(int interval_seconds) {
    if (heartbeat_running_.load()) {
        return;  // Already running
    }
    
    heartbeat_running_.store(true);
    heartbeat_thread_ = std::thread([this, interval_seconds]() {
        while (heartbeat_running_.load()) {
            // Sleep in small increments for quick shutdown
            for (int i = 0; i < interval_seconds * 10 && heartbeat_running_.load(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            
            if (!heartbeat_running_.load()) {
                break;
            }
            
            // Send lightweight GET to keep connection alive
            try {
                execute_get("/ok", std::nullopt, std::nullopt);
            } catch (...) {
                // Ignore errors in heartbeat
            }
        }
    });
}

void HttpClient::stop_heartbeat() {
    heartbeat_running_.store(false);
    if (heartbeat_thread_.joinable()) {
        heartbeat_thread_.join();
    }
}

bool HttpClient::is_heartbeat_running() const {
    return heartbeat_running_.load();
}

ConnectionStats HttpClient::get_stats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    ConnectionStats stats;
    stats.total_requests = total_requests_;
    stats.reused_connections = reused_connections_;
    stats.avg_latency_ms = total_requests_ > 0 ? total_latency_ms_ / total_requests_ : 0.0;
    stats.last_latency_ms = last_latency_ms_;
    stats.connection_warm = connection_warm_;
    return stats;
}

} // namespace clob
