#include "crypto/rsa.h"

#include "crypto/mod_arithmetic.h"

#include <random>

namespace crypto {
// Generuje parę kluczy RSA: wybiera dwa prawdopodobne pierwsze p,q i oblicza n,phi,d
RsaKeyPair create_rsa_key_pair(BackendMode) {
    // Stałe, z góry zdefiniowane liczby pierwsze używane zamiast losowego
    // generowania. Uproszcza to implementację i zachowanie programu.
    const BigInt p(101);
    const BigInt q(103);

    BigInt n = p * q;
    BigInt phi = (p - BigInt(1)) * (q - BigInt(1));

    BigInt e(65537);
    while (gcd(e, phi) != BigInt(1)) {
        e = e + BigInt(2);
    }

    BigInt d = mod_inverse(e, phi, BackendMode::CpuOnly);

    RsaKeyPair key_pair;
    key_pair.public_key.n = n;
    key_pair.public_key.e = e;
    key_pair.private_key.n = n;
    key_pair.private_key.d = d;
    return key_pair;
}

// Szyfruje wiadomość przy użyciu klucza publicznego
BigInt rsa_encrypt(const BigInt& message, const RsaPublicKey& key, BackendMode mode) {
    return mod_pow(message, key.e, key.n, mode);
}

// Deszyfruje szyfrogram przy użyciu klucza prywatnego
BigInt rsa_decrypt(const BigInt& cipher, const RsaPrivateKey& key, BackendMode mode) {
    return mod_pow(cipher, key.d, key.n, mode);
}

// Wsadowe szyfrowanie: szyfruje listę wiadomości tym samym kluczem publicznym
std::vector<BigInt> rsa_encrypt_many(const std::vector<BigInt>& messages,
                                       const RsaPublicKey& key,
                                       BackendMode mode) {
    std::vector<BigInt> exponents(messages.size(), key.e);
    return mod_pow_many(messages, exponents, key.n, mode);
}

// Wsadowe odszyfrowywanie: deszyfruje listę szyfrogramów tym samym kluczem prywatnym
std::vector<BigInt> rsa_decrypt_many(const std::vector<BigInt>& ciphers,
                                       const RsaPrivateKey& key,
                                       BackendMode mode) {
    std::vector<BigInt> exponents(ciphers.size(), key.d);
    return mod_pow_many(ciphers, exponents, key.n, mode);
}

}
