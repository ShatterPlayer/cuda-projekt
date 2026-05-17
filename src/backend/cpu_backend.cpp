#include "backend/cpu_backend.h"

namespace cpu_backend {

// Dodaje dwie wartości modulo `mod` (bez przepełnienia 64-bitowego)
uint64_t mod_add(uint64_t a, uint64_t b, uint64_t mod) {
    a %= mod;
    b %= mod;
    if (a >= mod - b) {
        return a - (mod - b);
    }
    return a + b;
}

// Odejmuje b od a w arytmetyce modulo
uint64_t mod_sub(uint64_t a, uint64_t b, uint64_t mod) {
    a %= mod;
    b %= mod;
    if (a >= b) {
        return a - b;
    }
    return mod - (b - a);
}

// Mnożenie modularne przy użyciu dodawania (bez użycia 128-bitowych typów)
uint64_t mod_mul(uint64_t a, uint64_t b, uint64_t mod) {
    a %= mod;
    b %= mod;

    uint64_t result = 0;
    while (b > 0) {
        if (b & 1ULL) {
            result = mod_add(result, a, mod);
        }
        a = mod_add(a, a, mod);
        b >>= 1ULL;
    }
    return result;
}

// Szybkie potęgowanie modularne dla typów 64-bitowych
uint64_t mod_pow(uint64_t base, uint64_t exponent, uint64_t mod) {
    base %= mod;
    uint64_t result = 1 % mod;

    while (exponent > 0) {
        if (exponent & 1ULL) {
            result = mod_mul(result, base, mod);
        }
        base = mod_mul(base, base, mod);
        exponent >>= 1ULL;
    }
    return result;
}

// Oblicza odwrotność modularną dla 64-bitowych wartości (jeśli istnieje)
uint64_t mod_inverse(uint64_t value, uint64_t mod) {
    long long t = 0;
    long long next_t = 1;
    long long r = static_cast<long long>(mod);
    long long next_r = static_cast<long long>(value % mod);

    while (next_r != 0) {
        long long q = r / next_r;

        long long temp_t = t - q * next_t;
        t = next_t;
        next_t = temp_t;

        long long temp_r = r - q * next_r;
        r = next_r;
        next_r = temp_r;
    }

    if (r != 1) {
        return 0;
    }

    if (t < 0) {
        t += static_cast<long long>(mod);
    }

    return static_cast<uint64_t>(t);
}

// Wsadowa wersja potęgowania dla 64-bitowych wartości
std::vector<uint64_t> mod_pow_many(const std::vector<uint64_t>& base_values,
                                   const std::vector<uint64_t>& exponent_values,
                                   uint64_t mod,
                                   BackendMode) {
    std::vector<uint64_t> output;
    size_t count = base_values.size();
    if (exponent_values.size() < count) {
        count = exponent_values.size();
    }

    output.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        output.push_back(mod_pow(base_values[i], exponent_values[i], mod));
    }
    return output;
}

}
