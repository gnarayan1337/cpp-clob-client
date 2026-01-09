// SPDX-License-Identifier: MIT
// Standalone Keccak-256 implementation
// Based on the Keccak reference implementation (public domain)

#pragma once

#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace clob {
namespace keccak {

namespace detail {

inline constexpr uint64_t ROUND_CONSTANTS[24] = {
    0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808aULL,
    0x8000000080008000ULL, 0x000000000000808bULL, 0x0000000080000001ULL,
    0x8000000080008081ULL, 0x8000000000008009ULL, 0x000000000000008aULL,
    0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000aULL,
    0x000000008000808bULL, 0x800000000000008bULL, 0x8000000000008089ULL,
    0x8000000000008003ULL, 0x8000000000008002ULL, 0x8000000000000080ULL,
    0x000000000000800aULL, 0x800000008000000aULL, 0x8000000080008081ULL,
    0x8000000000008080ULL, 0x0000000080000001ULL, 0x8000000080008008ULL
};

inline constexpr int ROTATIONS[24] = {
    1, 3, 6, 10, 15, 21, 28, 36, 45, 55, 2, 14,
    27, 41, 56, 8, 25, 43, 62, 18, 39, 61, 20, 44
};

inline constexpr int PILN[24] = {
    10, 7, 11, 17, 18, 3, 5, 16, 8, 21, 24, 4,
    15, 23, 19, 13, 12, 2, 20, 14, 22, 9, 6, 1
};

inline uint64_t rotl64(uint64_t x, int y) {
    return (x << y) | (x >> (64 - y));
}

inline void keccakf(uint64_t st[25]) {
    uint64_t t, bc[5];

    for (int r = 0; r < 24; r++) {
        // Theta
        for (int i = 0; i < 5; i++)
            bc[i] = st[i] ^ st[i + 5] ^ st[i + 10] ^ st[i + 15] ^ st[i + 20];

        for (int i = 0; i < 5; i++) {
            t = bc[(i + 4) % 5] ^ rotl64(bc[(i + 1) % 5], 1);
            for (int j = 0; j < 25; j += 5)
                st[j + i] ^= t;
        }

        // Rho Pi
        t = st[1];
        for (int i = 0; i < 24; i++) {
            int j = PILN[i];
            bc[0] = st[j];
            st[j] = rotl64(t, ROTATIONS[i]);
            t = bc[0];
        }

        // Chi
        for (int j = 0; j < 25; j += 5) {
            for (int i = 0; i < 5; i++)
                bc[i] = st[j + i];
            for (int i = 0; i < 5; i++)
                st[j + i] ^= (~bc[(i + 1) % 5]) & bc[(i + 2) % 5];
        }

        // Iota
        st[0] ^= ROUND_CONSTANTS[r];
    }
}

} // namespace detail

// Keccak-256: Ethereum's hash function (NOT SHA3-256)
inline std::array<uint8_t, 32> hash256(const uint8_t* data, size_t len) {
    constexpr size_t RATE = 136;  // (1600 - 256*2) / 8
    
    uint64_t st[25] = {0};
    uint8_t temp[RATE];
    size_t pt = 0;

    // Absorb
    for (size_t i = 0; i < len; i++) {
        temp[pt++] = data[i];
        if (pt == RATE) {
            for (size_t j = 0; j < RATE / 8; j++) {
                uint64_t lane = 0;
                for (int k = 0; k < 8; k++)
                    lane |= static_cast<uint64_t>(temp[j * 8 + k]) << (8 * k);
                st[j] ^= lane;
            }
            detail::keccakf(st);
            pt = 0;
        }
    }

    // Pad (Keccak uses 0x01, not SHA3's 0x06)
    std::memset(temp + pt, 0, RATE - pt);
    temp[pt] = 0x01;
    temp[RATE - 1] |= 0x80;

    for (size_t j = 0; j < RATE / 8; j++) {
        uint64_t lane = 0;
        for (int k = 0; k < 8; k++)
            lane |= static_cast<uint64_t>(temp[j * 8 + k]) << (8 * k);
        st[j] ^= lane;
    }
    detail::keccakf(st);

    // Squeeze
    std::array<uint8_t, 32> hash;
    for (size_t i = 0; i < 4; i++) {
        for (int j = 0; j < 8; j++)
            hash[i * 8 + j] = static_cast<uint8_t>(st[i] >> (8 * j));
    }
    return hash;
}

inline std::array<uint8_t, 32> hash256(const std::vector<uint8_t>& data) {
    return hash256(data.data(), data.size());
}

inline std::array<uint8_t, 32> hash256(const std::string& data) {
    return hash256(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}

} // namespace keccak
} // namespace clob

