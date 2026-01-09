#pragma once

#include <memory>
#include "types.hpp"
#include "signer.hpp"

namespace clob {

class OrderBuilder {
public:
    OrderBuilder(
        std::shared_ptr<Signer> signer,
        SignatureType sig_type = SignatureType::EOA,
        const std::string& funder = ""
    );
    
    // Create and sign a limit order
    SignedOrder create_order(
        const OrderArgs& args,
        const CreateOrderOptions& options
    );
    
    // Create and sign a market order
    SignedOrder create_market_order(
        const MarketOrderArgs& args,
        const CreateOrderOptions& options
    );
    
    // Calculate market buy price from orderbook
    double calculate_buy_market_price(
        const std::vector<OrderSummary>& positions,
        double amount_to_match,
        OrderType order_type
    );
    
    // Calculate market sell price from orderbook
    double calculate_sell_market_price(
        const std::vector<OrderSummary>& positions,
        double amount_to_match,
        OrderType order_type
    );
    
    SignatureType get_signature_type() const { return sig_type_; }

private:
    std::shared_ptr<Signer> signer_;
    SignatureType sig_type_;
    std::string funder_;
    
    struct OrderAmounts {
        uint8_t side;
        std::string maker_amount;
        std::string taker_amount;
    };
    
    // Get order amounts based on side and rounding config
    OrderAmounts get_order_amounts(
        Side side,
        double size,
        double price,
        const RoundConfig& config
    );
    
    // Get market order amounts
    OrderAmounts get_market_order_amounts(
        Side side,
        double amount,
        double price,
        const RoundConfig& config
    );
};

} // namespace clob



