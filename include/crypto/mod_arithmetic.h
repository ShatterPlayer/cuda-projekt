#ifndef MOD_ARITHMETIC_H
#define MOD_ARITHMETIC_H

#include <utility>
#include <vector>

#include "backend/backend_mode.h"
#include "crypto/big_int.h"

namespace crypto {

// Dodaje a i b modulo `mod` (wynik w przedziale [0, mod))
BigInt mod_add(const BigInt& a, const BigInt& b, const BigInt& mod, BackendMode mode);
// Odejmuje b od a modulo `mod` (wynik w przedziale [0, mod))
BigInt mod_sub(const BigInt& a, const BigInt& b, const BigInt& mod, BackendMode mode);
// Mnoży a i b modulo `mod`
BigInt mod_mul(const BigInt& a, const BigInt& b, const BigInt& mod, BackendMode mode);
// Podnosi `base` do potęgi `exponent` modulo `mod`
BigInt mod_pow(const BigInt& base, const BigInt& exponent, const BigInt& mod, BackendMode mode);
// Zwraca odwrotność modularną value^{-1} mod `mod`, lub 0 jeśli nie istnieje
BigInt mod_inverse(const BigInt& value, const BigInt& mod, BackendMode mode);

// Wykonuje wiele potęgowań modularnych: wynik[i] = base_values[i]^{exponent_values[i]} mod `mod`
std::vector<BigInt> mod_pow_many(const std::vector<BigInt>& base_values,
                                   const std::vector<BigInt>& exponent_values,
                                   const BigInt& mod,
                                   BackendMode mode);

}

#endif
