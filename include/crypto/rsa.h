#ifndef RSA_H
#define RSA_H

#include <string>
#include <vector>

#include "backend/backend_mode.h"
#include "crypto/big_int.h"

namespace crypto {

struct RsaPublicKey {
    BigInt n;
    BigInt e;
};

struct RsaPrivateKey {
    BigInt n;
    BigInt d;
};

struct RsaKeyPair {
    RsaPublicKey public_key;
    RsaPrivateKey private_key;
};

// Generuje parę kluczy RSA (publiczny/prywatny) na CPU lub z użyciem wybranego backendu
RsaKeyPair create_rsa_key_pair(BackendMode mode);
// Szyfruje (c = m^e mod n)
BigInt rsa_encrypt(const BigInt& message, const RsaPublicKey& key, BackendMode mode);
// Deszyfruje (m = c^d mod n)
BigInt rsa_decrypt(const BigInt& cipher, const RsaPrivateKey& key, BackendMode mode);

// Wersje wsadowe szyfrowania/deszyfrowania dla listy wiadomości
std::vector<BigInt> rsa_encrypt_many(const std::vector<BigInt>& messages,
                                       const RsaPublicKey& key,
                                       BackendMode mode);

std::vector<BigInt> rsa_decrypt_many(const std::vector<BigInt>& ciphers,
                                       const RsaPrivateKey& key,
                                       BackendMode mode);

}

#endif
