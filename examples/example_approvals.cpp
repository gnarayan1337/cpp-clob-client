// Example: ERC20 and ERC1155 approvals for trading on Polymarket
//
// Before trading, you must approve the exchange contracts to transfer your tokens.
// This example sets up all required on-chain approvals for Polymarket trading.
// 
// WARNING: This will send transactions to Polygon mainnet - make sure you have MATIC for gas!

#include <clob/eth_rpc.hpp>
#include <clob/signer.hpp>
#include <iostream>
#include <cstdlib>

int main() {
    try {
        // Get private key from environment
        const char* pk_env = std::getenv("PK");
        if (!pk_env) {
            std::cerr << "Error: PK environment variable not set" << std::endl;
            std::cerr << "Usage: export PK=your_private_key && ./approvals" << std::endl;
            return 1;
        }
        
        std::string private_key = pk_env;
        
        std::cout << "=== Polymarket Approval Setup ===" << std::endl;
        std::cout << "This will set up the required on-chain approvals for trading." << std::endl;
        std::cout << "Network: Polygon Mainnet" << std::endl;
        std::cout << std::endl;
        
        // Create signer to show address
        clob::Signer signer(private_key, 137);
        std::cout << "Your address: " << signer.address() << std::endl;
        std::cout << std::endl;
        
        // Show contract addresses
        std::cout << "=== Contract Addresses ===" << std::endl;
        std::cout << "USDC:              " << clob::polygon_contracts::USDC << std::endl;
        std::cout << "Exchange:          " << clob::polygon_contracts::EXCHANGE << std::endl;
        std::cout << "Neg-Risk Exchange: " << clob::polygon_contracts::NEG_RISK_EXCHANGE << std::endl;
        std::cout << "CTF:               " << clob::polygon_contracts::CTF << std::endl;
        std::cout << std::endl;
        
        // Create approval helper
        clob::ApprovalHelper helper(private_key);
        
        // Check current status
        std::cout << "=== Checking Current Approvals ===" << std::endl;
        
        bool usdc_exchange = helper.has_usdc_exchange_approval();
        bool usdc_neg_risk = helper.has_usdc_neg_risk_approval();
        bool ctf_exchange = helper.has_ctf_exchange_approval();
        bool ctf_neg_risk = helper.has_ctf_neg_risk_approval();
        
        std::cout << "USDC -> Exchange:          " << (usdc_exchange ? "[OK] Approved" : "[  ] Not approved") << std::endl;
        std::cout << "USDC -> Neg-Risk Exchange: " << (usdc_neg_risk ? "[OK] Approved" : "[  ] Not approved") << std::endl;
        std::cout << "CTF  -> Exchange:          " << (ctf_exchange ? "[OK] Approved" : "[  ] Not approved") << std::endl;
        std::cout << "CTF  -> Neg-Risk Exchange: " << (ctf_neg_risk ? "[OK] Approved" : "[  ] Not approved") << std::endl;
        std::cout << std::endl;
        
        bool all_approved = usdc_exchange && usdc_neg_risk && ctf_exchange && ctf_neg_risk;
        
        if (all_approved) {
            std::cout << "All approvals already set! You're ready to trade." << std::endl;
            return 0;
        }
        
        // Set up missing approvals
        std::cout << "=== Setting Up Missing Approvals ===" << std::endl;
        std::cout << "This will send transactions to Polygon. Make sure you have MATIC for gas." << std::endl;
        std::cout << "Press Enter to continue (Ctrl+C to cancel)..." << std::endl;
        std::cin.get();
        
        if (!usdc_exchange) {
            std::cout << "Approving USDC for Exchange..." << std::endl;
            std::string tx = helper.approve_usdc_for_exchange();
            std::cout << "  TX: " << tx << std::endl;
        }
        
        if (!usdc_neg_risk) {
            std::cout << "Approving USDC for Neg-Risk Exchange..." << std::endl;
            std::string tx = helper.approve_usdc_for_neg_risk_exchange();
            std::cout << "  TX: " << tx << std::endl;
        }
        
        if (!ctf_exchange) {
            std::cout << "Approving CTF for Exchange..." << std::endl;
            std::string tx = helper.approve_ctf_for_exchange();
            std::cout << "  TX: " << tx << std::endl;
        }
        
        if (!ctf_neg_risk) {
            std::cout << "Approving CTF for Neg-Risk Exchange..." << std::endl;
            std::string tx = helper.approve_ctf_for_neg_risk_exchange();
            std::cout << "  TX: " << tx << std::endl;
        }
        
        std::cout << std::endl;
        std::cout << "All approvals set! You're ready to trade on Polymarket." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
