# Polymarket C++ Client

[![CI](https://github.com/gnarayan1337/cpp-clob-client/actions/workflows/ci.yml/badge.svg)](https://github.com/gnarayan1337/cpp-clob-client/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/gnarayan1337/cpp-clob-client)](https://github.com/gnarayan1337/cpp-clob-client/releases)

A high-performance C++ client for interacting with Polymarket services, primarily the Central Limit Order Book (CLOB).
This library provides strongly typed request builders, authenticated endpoints, EIP-712 signing, and simdjson parsing.

## Table of Contents

- [Overview](#overview)
- [Getting Started](#getting-started)
- [Examples](#examples)
- [Setting Token Allowances](#token-allowances)
- [API Reference](#api-reference)
- [Architecture](#architecture)
- [Contributing](#contributing)
- [About Polymarket](#about-polymarket)

## Overview

- **Typed CLOB requests** (orders, trades, markets, balances, and more)
- **Dual authentication flows**
    - L1: Signer-only authentication (create API keys)
    - L2: Full API key authentication (trading)
- **Native EIP-712 signing** — no external Ethereum libraries required
- **simdjson parsing** — high-performance JSON parsing
- **On-chain approvals** — built-in Ethereum JSON-RPC for USDC/CTF approvals
- **Order builders** for easy construction and signing
- **Strongly typed responses** — compile-time safety

## Getting Started

### Requirements

- C++20 or later
- CMake 3.15+
- OpenSSL
- libsecp256k1
- libcurl

### macOS

```bash
brew install openssl secp256k1 curl cmake
```

### Ubuntu/Debian

```bash
sudo apt-get install libssl-dev libsecp256k1-dev libcurl4-openssl-dev cmake
```

### Building

```bash
git clone https://github.com/polymarket/cpp-clob-client
cd cpp-clob-client
mkdir build && cd build
cmake ..
make -j$(nproc)
```

Then run any of the examples:

```bash
./unauthenticated
```

## Examples

Some hand-picked examples. Please see `examples/` for more.

### Unauthenticated client (read-only)

```cpp
#include <clob/client.hpp>
#include <iostream>

int main() {
clob::ClobClient client("https://clob.polymarket.com");

    std::string ok = client.get_ok();
    std::cout << "Ok: " << ok << std::endl;

auto markets = client.get_markets();
    std::cout << "Markets: " << markets.data.size() << std::endl;

    return 0;
}
```

### Authenticated client

Set `PK` as an environment variable with your private key.

If using MetaMask or hardware wallet, you must first set token allowances. See [Token Allowances](#token-allowances) section below.

```cpp
#include <clob/client.hpp>
#include <clob/signer.hpp>
#include <iostream>
#include <cstdlib>

int main() {
    const char* pk_env = std::getenv("PK");
    if (!pk_env) {
        std::cerr << "PK environment variable not set" << std::endl;
        return 1;
    }
    std::string private_key = pk_env;

auto signer = std::make_shared<clob::Signer>(private_key, clob::POLYGON);

    // Create L1 client and get API credentials
clob::ClobClient l1_client("https://clob.polymarket.com", signer);
    clob::ApiCreds creds = l1_client.create_or_derive_api_creds();

    // Create L2 authenticated client
    clob::ClobClient client("https://clob.polymarket.com", signer, creds);

    std::string ok = client.get_ok();
    std::cout << "Ok: " << ok << std::endl;

    auto api_keys = client.get_api_keys();
    if (api_keys.keys.has_value()) {
        std::cout << "API keys: " << api_keys.keys->size() << std::endl;
    }

    return 0;
}
```

### Place a limit order

```cpp
#include <clob/client.hpp>
#include <clob/signer.hpp>
#include <iostream>
#include <cstdlib>

int main() {
    const char* pk_env = std::getenv("PK");
    if (!pk_env) return 1;
    std::string private_key = pk_env;

    auto signer = std::make_shared<clob::Signer>(private_key, clob::POLYGON);

    clob::ClobClient l1_client("https://clob.polymarket.com", signer);
    clob::ApiCreds creds = l1_client.create_or_derive_api_creds();
clob::ClobClient client("https://clob.polymarket.com", signer, creds);

    // Set up order arguments
    clob::OrderArgs args;
    args.token_id = "your_token_id_here";
    args.price = 0.50;
    args.size = 100.0;
    args.side = clob::Side::BUY;

    // Set up order options
    clob::CreateOrderOptions opts;
    opts.tick_size = "0.01";
    opts.neg_risk = false;

    clob::SignedOrder signed_order = client.create_order(args, opts);
    clob::PostOrderResponse response = client.post_order(signed_order, clob::OrderType::GTC);

    std::cout << "Order ID: " << response.order_id << std::endl;

    return 0;
}
```

## Token Allowances

### Do I need to set allowances?

MetaMask and EOA users must set token allowances.
If you are using a proxy or Safe-type wallet, then you do not.

### What are allowances?

Think of allowances as permissions. Before Polymarket can move your funds to execute trades, you need to give the exchange contracts permission to access your USDC and conditional tokens.

### Quick Setup

You need to approve two types of tokens:
1. **USDC** (for deposits and trading)
2. **Conditional Tokens** (the outcome tokens you trade)

Each needs approval for the exchange contracts to work properly.

### Setting Allowances

Use `examples/example_approvals.cpp` to approve the right contracts:

```bash
export PK=your_private_key
./approvals
```

You only need to set these once per wallet. After that, you can trade freely.

## API Reference

### Public Endpoints

```cpp
std::string get_ok();
Timestamp get_server_time();
Page<MarketResponse> get_markets(const std::string& next_cursor = "");
MarketResponse get_market(const std::string& condition_id);
OrderBookSummaryResponse get_order_book(const std::string& token_id);
MidpointResponse get_midpoint(const std::string& token_id);
PriceResponse get_price(const std::string& token_id, Side side);
SpreadResponse get_spread(const std::string& token_id);
TickSizeResponse get_tick_size(const std::string& token_id);
NegRiskResponse get_neg_risk(const std::string& token_id);
FeeRateResponse get_fee_rate_bps(const std::string& token_id);
```

### Authenticated Endpoints

```cpp
ApiCreds create_api_key();
ApiCreds derive_api_key();
ApiCreds create_or_derive_api_creds();
PostOrderResponse post_order(const SignedOrder& order, OrderType order_type);
OpenOrderResponse get_order(const std::string& order_id);
Page<OpenOrderResponse> get_orders();
CancelOrdersResponse cancel(const std::string& order_id);
CancelOrdersResponse cancel_all();
CancelOrdersResponse cancel_orders(const std::vector<std::string>& order_ids);
Page<TradeResponse> get_trades();
BalanceAllowanceResponse get_balance_allowance();
```

## Architecture

```
include/clob/
├── client.hpp        # Main API client
├── signer.hpp        # EIP-712 signing
├── order_builder.hpp # Order construction
├── types.hpp         # Request/response types
├── eip712.hpp        # EIP-712 typed data
├── eth_rpc.hpp       # Ethereum JSON-RPC
├── http_client.hpp   # HTTP client with simdjson
└── constants.hpp     # Chain/contract constants

src/
├── client.cpp        # API implementation
├── signer.cpp        # secp256k1 signing
├── order_builder.cpp # Order building logic
├── eip712.cpp        # Keccak-256, type hashing
├── eth_rpc.cpp       # RLP encoding, transactions
├── http_client.cpp   # HTTP + simdjson parsing
└── utilities.cpp     # Helper functions

examples/
├── unauthenticated   # Public endpoints
├── authenticated     # API key creation
├── place_order       # Order creation
├── approvals         # On-chain approvals
├── trading           # Full trading flow
├── parallel          # Concurrent requests
└── pagination        # Paginated responses
```

### Implementation Details

Built from scratch without external Ethereum libraries:

- **EIP-712 typed data signing**
- **RLP encoding**
- **ABI encoding**
- **Ethereum JSON-RPC client**
- **On-chain approval helpers**
- **L2 HMAC auth message construction**

## Contributing

We encourage contributions from the community. Check out our [contributing guidelines](CONTRIBUTING.md) for instructions on how to contribute to this SDK.

## License

MIT

## About Polymarket

[Polymarket](https://docs.polymarket.com/polymarket-learn/get-started/what-is-polymarket) is the world's largest prediction market, allowing you to stay informed and profit from your knowledge by betting on future events across various topics.

Studies show prediction markets are often more accurate than pundits because they combine news, polls, and expert opinions into a single value that represents the market's view of an event's odds. Our markets reflect accurate, unbiased, and real-time probabilities for the events that matter most to you. Markets seek truth.
