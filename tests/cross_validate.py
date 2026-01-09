#!/usr/bin/env python3
"""
Cross-validation script to compare EIP-712 signatures across implementations.

This script generates signatures using the Python client and compares them
with the C++ client to ensure compatibility.
"""

import sys
import os
import json
from pathlib import Path

# Add py-clob-client to path
sys.path.insert(0, str(Path(__file__).parent.parent.parent / "py-clob-client"))

try:
    from py_clob_client.signer import Signer as PySigner
    from py_clob_client.signing.eip712 import sign_clob_auth_message, get_clob_auth_domain
    from py_clob_client.signing.model import ClobAuth
    from eth_utils import keccak
    from poly_eip712_structs import make_domain
except ImportError as e:
    print(f"Error: {e}")
    print("Please install py-clob-client dependencies:")
    print("  cd ../py-clob-client && pip install -r requirements.txt")
    sys.exit(1)

# Test private key (publicly known, DO NOT use in production)
TEST_PRIVATE_KEY = "0xac0974bec39a17e36ba4a6b4d238ff944bacb478cbed5efcae784d7bf4f2ff80"
EXPECTED_ADDRESS = "0xf39Fd6e51aad88F6F4ce6aB8827279cffFb92266"

def test_clob_auth_signature():
    """Test ClobAuth message signing."""
    print("=" * 80)
    print("Testing ClobAuth Signature")
    print("=" * 80)
    
    # Create signer
    signer = PySigner(TEST_PRIVATE_KEY, 80002)  # Amoy testnet
    
    print(f"Private Key: {TEST_PRIVATE_KEY}")
    print(f"Address: {signer.address()}")
    print(f"Expected: {EXPECTED_ADDRESS}")
    
    assert signer.address().lower() == EXPECTED_ADDRESS.lower(), "Address mismatch!"
    
    # Create ClobAuth message with EXACT same parameters as C++ test
    timestamp = 10000000
    nonce = 23
    
    signature = sign_clob_auth_message(signer, timestamp, nonce)
    
    print(f"\nSignature: {signature}")
    print(f"Signature length: {len(signature)}")
    
    # This should match the Rust test output
    expected_rust_sig = "0xf62319a987514da40e57e2f4d7529f7bac38f0355bd88bb5adbb3768d80de6c1682518e0af677d5260366425f4361e7b70c25ae232aff0ab2331e2b164a1aedc1b"
    
    print(f"\nExpected (from Rust): {expected_rust_sig}")
    print(f"Matches Rust: {signature.lower() == expected_rust_sig.lower()}")
    
    if signature.lower() != expected_rust_sig.lower():
        print("\nWARNING: Signature does not match Rust implementation!")
        return False
    else:
        print("\nSignature matches Rust implementation!")
        return True

def test_order_signature():
    """Test Order message signing."""
    print("\n" + "=" * 80)
    print("Testing Order Signature")
    print("=" * 80)
    
    from py_order_utils.builders import OrderBuilder
    from py_order_utils.signer import Signer as UtilsSigner
    from py_order_utils.model import OrderData, BUY
    
    # Create signer
    chain_id = 137  # Polygon mainnet
    signer = PySigner(TEST_PRIVATE_KEY, chain_id)
    
    print(f"Chain ID: {chain_id}")
    print(f"Address: {signer.address()}")
    
    # Create order with known values
    order_data = OrderData(
        maker=signer.address(),
        taker="0x0000000000000000000000000000000000000000",
        tokenId="1234567890",
        makerAmount="100000000",
        takerAmount="50000000",
        side=BUY,
        feeRateBps="0",
        nonce="0",
        signer=signer.address(),
        expiration="0",
        signatureType=0,
    )
    
    # Use the real CTFExchange address for Polygon
    exchange_address = "0x4bFb41d5B3570DeFd03C39a9A4D8dE6Bd8B8982E"
    
    # Create order builder
    order_builder = OrderBuilder(
        exchange_address,
        chain_id,
        UtilsSigner(key=signer.private_key)
    )
    
    # Sign the order
    signed_order = order_builder.build_signed_order(order_data)
    
    print(f"\nOrder signature: {signed_order.signature}")
    print(f"Signature length: {len(signed_order.signature)}")
    
    # Save test data for C++ to compare
    test_data = {
        "private_key": TEST_PRIVATE_KEY,
        "chain_id": chain_id,
        "address": signer.address(),
        "exchange": exchange_address,
        "order": {
            "salt": signed_order.salt,
            "maker": signed_order.maker,
            "signer": signed_order.signer,
            "taker": signed_order.taker,
            "tokenId": signed_order.tokenId,
            "makerAmount": signed_order.makerAmount,
            "takerAmount": signed_order.takerAmount,
            "side": signed_order.side,
            "expiration": signed_order.expiration,
            "nonce": signed_order.nonce,
            "feeRateBps": signed_order.feeRateBps,
            "signatureType": signed_order.signatureType
        },
        "signature": signed_order.signature
    }
    
    output_file = Path(__file__).parent / "test_vectors.json"
    with open(output_file, 'w') as f:
        json.dump(test_data, f, indent=2)
    
    print(f"\nTest data saved to: {output_file}")
    return True

def test_keccak256():
    """Test keccak256 hash function."""
    print("\n" + "=" * 80)
    print("Testing Keccak-256")
    print("=" * 80)
    
    test_cases = [
        ("", "0xc5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470"),
        ("Hello, World!", "0xacaf3289d7b601cbd114fb36c4d29c85bbfd5e133f14cb355c3fd8d99367964f"),
    ]
    
    all_passed = True
    for input_str, expected in test_cases:
        result = "0x" + keccak(text=input_str).hex()
        passed = result == expected
        
        print(f"\nInput: '{input_str}'")
        print(f"Expected: {expected}")
        print(f"Got:      {result}")
        print(f"Status: {'[PASS]' if passed else '[FAIL]'}")
        
        if not passed:
            all_passed = False
    
    return all_passed

def main():
    print("\n=== Cross-Validation Test Suite ===")
    print("Comparing Python and C++ EIP-712 implementations\n")
    
    results = []
    
    # Test keccak256
    results.append(("Keccak-256", test_keccak256()))
    
    # Test ClobAuth signing
    results.append(("ClobAuth Signature", test_clob_auth_signature()))
    
    # Test Order signing
    results.append(("Order Signature", test_order_signature()))
    
    # Summary
    print("\n" + "=" * 80)
    print("SUMMARY")
    print("=" * 80)
    
    for name, passed in results:
        status = "[PASS]" if passed else "[FAIL]"
        print(f"{name:30s} {status}")
    
    all_passed = all(r[1] for r in results)
    
    if all_passed:
        print("\nAll tests passed!")
        return 0
    else:
        print("\nSome tests failed!")
        return 1

if __name__ == "__main__":
    sys.exit(main())



