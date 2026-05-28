#ifndef AES_H
#define AES_H

#include <array>
#include <cstdint>
#include <vector>

#include "backend/backend_mode.h"

namespace crypto {

using Byte = uint8_t;
using AesBlock = std::array<Byte, 16>;

// AES-CTR: encryption and decryption are identical (XOR with keystream).
std::vector<Byte> aes_ctr_crypt(const std::vector<Byte>& input,
                               const AesBlock& key,
                               const AesBlock& iv,
                               BackendMode mode);

void aes_ctr_crypt_inplace(std::vector<Byte>& data,
                           const AesBlock& key,
                           const AesBlock& iv,
                           BackendMode mode);

}

#endif
