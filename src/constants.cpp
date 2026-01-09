#include "clob/constants.hpp"

namespace clob {

ContractConfig get_contract_config(uint64_t chain_id, bool neg_risk) {
    // Polygon Mainnet
    if (chain_id == POLYGON) {
        if (neg_risk) {
            return ContractConfig{
                "0xC5d563A36AE78145C45a50134d48A1215220f80a",  // NegRiskAdapter
                "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",  // USDC
                "0x4D97DCd97eC945f40cF65F87097ACe5EA0476045"   // CTF
            };
        } else {
            return ContractConfig{
                "0x4bFb41d5B3570DeFd03C39a9A4D8dE6Bd8B8982E",  // CTFExchange
                "0x2791Bca1f2de4661ED88A30C99A7a9449Aa84174",  // USDC
                "0x4D97DCd97eC945f40cF65F87097ACe5EA0476045"   // CTF
            };
        }
    }
    
    // Amoy Testnet
    if (chain_id == AMOY) {
        if (neg_risk) {
            return ContractConfig{
                "0xd91E80cF2E7be2e162c6513ceD06f1dD0dA35296",  // NegRiskAdapter
                "0x9c4e1703476e875070ee25b56a58b008cfb8fa78",  // USDC (testnet)
                "0x69308FB512518e39F9b16112fA8d994F4e2Bf8bB"   // CTF
            };
        } else {
            return ContractConfig{
                "0xdFE02Eb6733538f8Ea35D585af8DE5958AD99E40",  // CTFExchange
                "0x9c4e1703476e875070ee25b56a58b008cfb8fa78",  // USDC (testnet)
                "0x69308FB512518e39F9b16112fA8d994F4e2Bf8bB"   // CTF
            };
        }
    }
    
    // Default empty config for unknown chains
    return ContractConfig{"", "", ""};
}

} // namespace clob



