#include "crypto/mod_arithmetic.h"

namespace crypto {
// Dodaje dwie wartości BigInt modulo modulus i zwraca wynik
BigInt mod_add(const BigInt& a, const BigInt& b, const BigInt& modulus, BackendMode) {
    return crypto::mod(a + b, modulus);
}

// Odejmuje b od a w arytmetyce modularnej
BigInt mod_sub(const BigInt& a, const BigInt& b, const BigInt& modulus, BackendMode) {
    if (a >= b) {
        return crypto::mod(a - b, modulus);
    }
    BigInt diff = crypto::mod(b - a, modulus);
    if (diff.is_zero()) {
        return BigInt();
    }
    return crypto::mod(modulus - diff, modulus);
}

// Mnoży a i b w arytmetyce modularnej
BigInt mod_mul(const BigInt& a, const BigInt& b, const BigInt& modulus, BackendMode) {
    return crypto::mod(a * b, modulus);
}

// Podnosi base do potęgi exponent modulo modulus (możliwość wyboru backendu)
BigInt mod_pow(const BigInt& base, const BigInt& exponent, const BigInt& modulus, BackendMode) {
    return crypto::mod_pow(base, exponent, modulus);
}

// Zwraca odwrotność modularną value względem modulus (0 jeśli brak odwrotności)
BigInt mod_inverse(const BigInt& value, const BigInt& modulus, BackendMode) {
    return crypto::mod_inverse(value, modulus);
}

std::vector<BigInt> mod_pow_many(const std::vector<BigInt>& base_values,
                                   const std::vector<BigInt>& exponent_values,
                                   const BigInt& modulus,
                                   BackendMode mode) {
    // Wersja wsadowa (dla wielu potęgowań)
    std::vector<BigInt> output;
    size_t count = base_values.size();
    if (exponent_values.size() < count) {
        count = exponent_values.size();
    }

    output.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        output.push_back(crypto::mod_pow(base_values[i], exponent_values[i], modulus, mode));
    }
    return output;
}

}
