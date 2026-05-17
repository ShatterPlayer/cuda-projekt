#ifndef DSA_H
#define DSA_H

#include <string>
#include <vector>

#include "backend/backend_mode.h"
#include "crypto/big_int.h"

namespace crypto {

struct DsaParams {
    BigInt p;
    BigInt q;
    BigInt g;
};

struct DsaPublicKey {
    BigInt y;
};

struct DsaPrivateKey {
    BigInt x;
};

struct DsaKeyPair {
    DsaParams params;
    DsaPublicKey public_key;
    DsaPrivateKey private_key;
};

struct DsaSignature {
    BigInt r;
    BigInt s;
};

// Generuje parametry DSA oraz parę kluczy (p,q,g) i (x,y)
DsaKeyPair create_dsa_key_pair(BackendMode mode,
                               size_t p_bits = 512,
                               size_t q_bits = 128);
// Podpisuje wiadomość, zwraca strukturę (r,s)
DsaSignature dsa_sign(const std::string& message,
                      const DsaKeyPair& key_pair,
                      BackendMode mode);
// Weryfikuje podpis DSA dla danej wiadomości i parametrów
bool dsa_verify(const std::string& message,
                const DsaSignature& signature,
                const DsaKeyPair& key_pair,
                BackendMode mode);

// Wersje wsadowe podpisywania i weryfikacji dla wektora wiadomości
std::vector<DsaSignature> dsa_sign_many(const std::vector<std::string>& messages,
                                        const DsaKeyPair& key_pair,
                                        BackendMode mode);

std::vector<bool> dsa_verify_many(const std::vector<std::string>& messages,
                                  const std::vector<DsaSignature>& signatures,
                                  const DsaKeyPair& key_pair,
                                  BackendMode mode);

}

#endif
