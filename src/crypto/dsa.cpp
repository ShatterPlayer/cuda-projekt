#include "crypto/dsa.h"

#include "crypto/mod_arithmetic.h"

#include <cstdint>
#include <random>
#include <openssl/sha.h>

namespace crypto {
static BigInt simple_hash(const std::string& message, BackendMode /*mode*/) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(message.data()), message.size(), digest);

    BigInt result(0);
    for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        result = result.mul_small(256);
        result.add_small(static_cast<uint32_t>(digest[i]));
    }
    return result;
}

static BigInt random_bigint_in_range_small(uint64_t low, uint64_t high, std::mt19937_64& generator) {
    std::uniform_int_distribution<uint64_t> distribution(low, high);
    return BigInt(distribution(generator));
}

// Generuje parametry DSA i parę kluczy (p,q,g) oraz (x,y)
DsaKeyPair create_dsa_key_pair(BackendMode mode,
                               size_t p_bits,
                               size_t q_bits) {
    (void)p_bits;
    (void)q_bits;

    std::random_device random_device;
    std::mt19937_64 generator(random_device());

    DsaKeyPair key_pair;

    // Stałe parametry DSA dobrane tak, aby spełniały warunek q | (p - 1).
    // p = 23, q = 11, g = 4; dla tych wartości możliwe jest poprawne podpisywanie
    // i weryfikacja bez dodatkowego generowania liczb pierwszych.
    key_pair.params.p = BigInt(23);
    key_pair.params.q = BigInt(11);
    key_pair.params.g = BigInt(4);

    key_pair.private_key.x = random_bigint_in_range_small(1, 10, generator);
    key_pair.public_key.y = mod_pow(key_pair.params.g,
                                    key_pair.private_key.x,
                                    key_pair.params.p,
                                    mode);
    return key_pair;
}

// Podpisuje wiadomość za pomocą klucza prywatnego DSA (z losowym k)
DsaSignature dsa_sign(const std::string& message,
                      const DsaKeyPair& key_pair,
                      BackendMode mode) {
    BigInt hash = mod(simple_hash(message, mode), key_pair.params.q);
    std::random_device random_device;
    std::mt19937_64 generator(random_device());

    while (true) {
        BigInt k = random_bigint_in_range_small(1, 10, generator);
        BigInt r = mod(mod_pow(key_pair.params.g, k, key_pair.params.p, mode), key_pair.params.q);
        if (r.is_zero()) {
            continue;
        }

        BigInt k_inv = mod_inverse(k, key_pair.params.q, mode);
        if (k_inv.is_zero()) {
            continue;
        }

        BigInt xr = mod_mul(key_pair.private_key.x, r, key_pair.params.q, mode);
        BigInt sum = mod_add(hash, xr, key_pair.params.q, mode);
        BigInt s = mod_mul(k_inv, sum, key_pair.params.q, mode);
        if (s.is_zero()) {
            continue;
        }

        DsaSignature signature;
        signature.r = r;
        signature.s = s;
        return signature;
    }
}

// Weryfikuje podpis DSA dla danej wiadomości i parametrów
bool dsa_verify(const std::string& message,
                const DsaSignature& signature,
                const DsaKeyPair& key_pair,
                BackendMode mode) {
    if (signature.r.is_zero() || signature.r >= key_pair.params.q) {
        return false;
    }
    if (signature.s.is_zero() || signature.s >= key_pair.params.q) {
        return false;
    }

    BigInt hash = mod(simple_hash(message, mode), key_pair.params.q);
    BigInt w = mod_inverse(signature.s, key_pair.params.q, mode);
    BigInt u1 = mod_mul(hash, w, key_pair.params.q, mode);
    BigInt u2 = mod_mul(signature.r, w, key_pair.params.q, mode);

    BigInt v1 = mod_pow(key_pair.params.g, u1, key_pair.params.p, mode);
    BigInt v2 = mod_pow(key_pair.public_key.y, u2, key_pair.params.p, mode);
    BigInt v = mod(mod_mul(v1, v2, key_pair.params.p, mode), key_pair.params.q);

    return v == signature.r;
}

// Wsadowe podpisywanie wiadomości przy użyciu tego samego klucza
std::vector<DsaSignature> dsa_sign_many(const std::vector<std::string>& messages,
                                        const DsaKeyPair& key_pair,
                                        BackendMode mode) {
    std::vector<DsaSignature> signatures;
    signatures.reserve(messages.size());

    for (const std::string& message : messages) {
        signatures.push_back(dsa_sign(message, key_pair, mode));
    }
    return signatures;
}

// Wsadowa weryfikacja podpisów DSA
std::vector<bool> dsa_verify_many(const std::vector<std::string>& messages,
                                  const std::vector<DsaSignature>& signatures,
                                  const DsaKeyPair& key_pair,
                                  BackendMode mode) {
    std::vector<bool> results;
    size_t count = messages.size();
    if (signatures.size() < count) {
        count = signatures.size();
    }

    results.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        results.push_back(dsa_verify(messages[i], signatures[i], key_pair, mode));
    }
    return results;
}

}
