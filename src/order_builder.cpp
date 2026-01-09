#include "clob/order_builder.hpp"
#include "clob/constants.hpp"
#include "clob/utilities.hpp"
#include "clob/eip712.hpp"
#include <stdexcept>
#include <ctime>
#include <random>

namespace clob {

OrderBuilder::OrderBuilder(
    std::shared_ptr<Signer> signer,
    SignatureType sig_type,
    const std::string& funder
) : signer_(signer), sig_type_(sig_type) {
    funder_ = funder.empty() ? signer->address() : funder;
}

OrderBuilder::OrderAmounts OrderBuilder::get_order_amounts(
    Side side,
    double size,
    double price,
    const RoundConfig& config
) {
    double raw_price = utils::round_normal(price, config.price);
    
    if (side == Side::BUY) {
        double raw_taker_amt = utils::round_down(size, config.size);
        double raw_maker_amt = raw_taker_amt * raw_price;
        
        if (utils::decimal_places(raw_maker_amt) > config.amount) {
            raw_maker_amt = utils::round_up(raw_maker_amt, config.amount + 4);
            if (utils::decimal_places(raw_maker_amt) > config.amount) {
                raw_maker_amt = utils::round_down(raw_maker_amt, config.amount);
            }
        }
        
        return {
            0,  // BUY
            std::to_string(utils::to_token_decimals(raw_maker_amt)),
            std::to_string(utils::to_token_decimals(raw_taker_amt))
        };
    } else if (side == Side::SELL) {
        double raw_maker_amt = utils::round_down(size, config.size);
        double raw_taker_amt = raw_maker_amt * raw_price;
        
        if (utils::decimal_places(raw_taker_amt) > config.amount) {
            raw_taker_amt = utils::round_up(raw_taker_amt, config.amount + 4);
            if (utils::decimal_places(raw_taker_amt) > config.amount) {
                raw_taker_amt = utils::round_down(raw_taker_amt, config.amount);
            }
        }
        
        return {
            1,  // SELL
            std::to_string(utils::to_token_decimals(raw_maker_amt)),
            std::to_string(utils::to_token_decimals(raw_taker_amt))
        };
    }
    
    throw std::runtime_error("Invalid side");
}

OrderBuilder::OrderAmounts OrderBuilder::get_market_order_amounts(
    Side side,
    double amount,
    double price,
    const RoundConfig& config
) {
    double raw_price = utils::round_normal(price, config.price);
    
    if (side == Side::BUY) {
        double raw_maker_amt = utils::round_down(amount, config.size);
        double raw_taker_amt = raw_maker_amt / raw_price;
        
        if (utils::decimal_places(raw_taker_amt) > config.amount) {
            raw_taker_amt = utils::round_up(raw_taker_amt, config.amount + 4);
            if (utils::decimal_places(raw_taker_amt) > config.amount) {
                raw_taker_amt = utils::round_down(raw_taker_amt, config.amount);
            }
        }
        
        return {
            0,  // BUY
            std::to_string(utils::to_token_decimals(raw_maker_amt)),
            std::to_string(utils::to_token_decimals(raw_taker_amt))
        };
    } else if (side == Side::SELL) {
        double raw_maker_amt = utils::round_down(amount, config.size);
        double raw_taker_amt = raw_maker_amt * raw_price;
        
        if (utils::decimal_places(raw_taker_amt) > config.amount) {
            raw_taker_amt = utils::round_up(raw_taker_amt, config.amount + 4);
            if (utils::decimal_places(raw_taker_amt) > config.amount) {
                raw_taker_amt = utils::round_down(raw_taker_amt, config.amount);
            }
        }
        
        return {
            1,  // SELL
            std::to_string(utils::to_token_decimals(raw_maker_amt)),
            std::to_string(utils::to_token_decimals(raw_taker_amt))
        };
    }
    
    throw std::runtime_error("Invalid side");
}

SignedOrder OrderBuilder::create_order(
    const OrderArgs& args,
    const CreateOrderOptions& options
) {
    auto it = ROUNDING_CONFIG.find(options.tick_size);
    if (it == ROUNDING_CONFIG.end()) {
        throw std::runtime_error("Invalid tick size");
    }
    
    auto amounts = get_order_amounts(args.side, args.size, args.price, it->second);
    
    // Generate salt (random 64-bit number, masked to 53 bits for IEEE 754)
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    uint64_t salt = dis(gen) & ((1ULL << 53) - 1);  // Mask to 53 bits
    
    // Get contract config
    auto contract_config = get_contract_config(signer_->get_chain_id(), options.neg_risk);
    
    // Build order struct for signing
    json order_data = {
        {"salt", std::to_string(salt)},
        {"maker", funder_},
        {"signer", signer_->address()},
        {"taker", args.taker},
        {"tokenId", args.token_id},
        {"makerAmount", amounts.maker_amount},
        {"takerAmount", amounts.taker_amount},
        {"side", amounts.side},
        {"expiration", std::to_string(args.expiration)},
        {"nonce", std::to_string(args.nonce)},
        {"feeRateBps", std::to_string(args.fee_rate_bps.value_or(0))},
        {"signatureType", static_cast<uint8_t>(sig_type_)}
    };
    
    // EIP712 domain
    json domain = {
        {"name", ORDER_DOMAIN_NAME},
        {"version", ORDER_VERSION},
        {"chainId", signer_->get_chain_id()},
        {"verifyingContract", contract_config.exchange}
    };
    
    // EIP712 types
    json types = {
        {"Order", json::array({
            {{"name", "salt"}, {"type", "uint256"}},
            {{"name", "maker"}, {"type", "address"}},
            {{"name", "signer"}, {"type", "address"}},
            {{"name", "taker"}, {"type", "address"}},
            {{"name", "tokenId"}, {"type", "uint256"}},
            {{"name", "makerAmount"}, {"type", "uint256"}},
            {{"name", "takerAmount"}, {"type", "uint256"}},
            {{"name", "expiration"}, {"type", "uint256"}},
            {{"name", "nonce"}, {"type", "uint256"}},
            {{"name", "feeRateBps"}, {"type", "uint256"}},
            {{"name", "side"}, {"type", "uint8"}},
            {{"name", "signatureType"}, {"type", "uint8"}}
        })}
    };
    
    // Sign
    std::string signature = signer_->sign_typed_data(domain, "Order", order_data, types);
    
    // Build order
    Order order;
    order.salt = std::to_string(salt);
    order.maker = funder_;
    order.signer = signer_->address();
    order.taker = args.taker;
    order.token_id = args.token_id;
    order.maker_amount = amounts.maker_amount;
    order.taker_amount = amounts.taker_amount;
    order.side = amounts.side;
    order.expiration = std::to_string(args.expiration);
    order.nonce = std::to_string(args.nonce);
    order.fee_rate_bps = std::to_string(args.fee_rate_bps.value_or(0));
    order.signature_type = static_cast<uint8_t>(sig_type_);
    
    // Build signed order
    SignedOrder signed_order;
    signed_order.order = order;
    signed_order.signature = signature;
    signed_order.order_type = OrderType::GTC;  // Default for limit orders
    signed_order.owner = "";  // Will be filled by client when posting
    
    return signed_order;
}

SignedOrder OrderBuilder::create_market_order(
    const MarketOrderArgs& args,
    const CreateOrderOptions& options
) {
    auto it = ROUNDING_CONFIG.find(options.tick_size);
    if (it == ROUNDING_CONFIG.end()) {
        throw std::runtime_error("Invalid tick size");
    }
    
    if (!args.price.has_value()) {
        throw std::runtime_error("Market order price must be provided");
    }
    
    auto amounts = get_market_order_amounts(args.side, args.amount, *args.price, it->second);
    
    // Generate salt (masked to 53 bits for IEEE 754)
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dis;
    uint64_t salt = dis(gen) & ((1ULL << 53) - 1);  // Mask to 53 bits
    
    // Get contract config
    auto contract_config = get_contract_config(signer_->get_chain_id(), options.neg_risk);
    
    // Build order struct
    json order_data = {
        {"salt", std::to_string(salt)},
        {"maker", funder_},
        {"signer", signer_->address()},
        {"taker", args.taker},
        {"tokenId", args.token_id},
        {"makerAmount", amounts.maker_amount},
        {"takerAmount", amounts.taker_amount},
        {"side", amounts.side},
        {"expiration", "0"},  // Market orders have 0 expiration
        {"nonce", std::to_string(args.nonce)},
        {"feeRateBps", std::to_string(args.fee_rate_bps.value_or(0))},
        {"signatureType", static_cast<uint8_t>(sig_type_)}
    };
    
    // EIP712 domain
    json domain = {
        {"name", ORDER_DOMAIN_NAME},
        {"version", ORDER_VERSION},
        {"chainId", signer_->get_chain_id()},
        {"verifyingContract", contract_config.exchange}
    };
    
    // EIP712 types
    json types = {
        {"Order", json::array({
            {{"name", "salt"}, {"type", "uint256"}},
            {{"name", "maker"}, {"type", "address"}},
            {{"name", "signer"}, {"type", "address"}},
            {{"name", "taker"}, {"type", "address"}},
            {{"name", "tokenId"}, {"type", "uint256"}},
            {{"name", "makerAmount"}, {"type", "uint256"}},
            {{"name", "takerAmount"}, {"type", "uint256"}},
            {{"name", "expiration"}, {"type", "uint256"}},
            {{"name", "nonce"}, {"type", "uint256"}},
            {{"name", "feeRateBps"}, {"type", "uint256"}},
            {{"name", "side"}, {"type", "uint8"}},
            {{"name", "signatureType"}, {"type", "uint8"}}
        })}
    };
    
    // Sign
    std::string signature = signer_->sign_typed_data(domain, "Order", order_data, types);
    
    // Build order
    Order order;
    order.salt = std::to_string(salt);
    order.maker = funder_;
    order.signer = signer_->address();
    order.taker = args.taker;
    order.token_id = args.token_id;
    order.maker_amount = amounts.maker_amount;
    order.taker_amount = amounts.taker_amount;
    order.side = amounts.side;
    order.expiration = "0";
    order.nonce = std::to_string(args.nonce);
    order.fee_rate_bps = std::to_string(args.fee_rate_bps.value_or(0));
    order.signature_type = static_cast<uint8_t>(sig_type_);
    
    // Build signed order
    SignedOrder signed_order;
    signed_order.order = order;
    signed_order.signature = signature;
    signed_order.order_type = args.order_type;
    signed_order.owner = "";  // Will be filled by client when posting
    
    return signed_order;
}

double OrderBuilder::calculate_buy_market_price(
    const std::vector<OrderSummary>& positions,
    double amount_to_match,
    OrderType order_type
) {
    if (positions.empty()) {
        throw std::runtime_error("No match");
    }
    
    double sum = 0.0;
    // Walk through asks from best (lowest) to worst (highest)
    for (const auto& position : positions) {
        double size = std::stod(position.size);
        double price = std::stod(position.price);
        sum += size * price;  // Accumulate notional value
        
        if (sum >= amount_to_match) {
            return price;  // This is the clearing price
        }
    }
    
    if (order_type == OrderType::FOK) {
        throw std::runtime_error("No match");
    }
    
    // FAK: return worst price we can get
    return std::stod(positions.back().price);
}

double OrderBuilder::calculate_sell_market_price(
    const std::vector<OrderSummary>& positions,
    double amount_to_match,
    OrderType order_type
) {
    if (positions.empty()) {
        throw std::runtime_error("No match");
    }
    
    double sum = 0.0;
    // Walk through bids from best (highest) to worst (lowest)
    for (const auto& position : positions) {
        double size = std::stod(position.size);
        sum += size;  // Accumulate share quantity
        
        if (sum >= amount_to_match) {
            return std::stod(position.price);  // This is the clearing price
        }
    }
    
    if (order_type == OrderType::FOK) {
        throw std::runtime_error("No match");
    }
    
    // FAK: return worst price we can get
    return std::stod(positions.back().price);
}

} // namespace clob
