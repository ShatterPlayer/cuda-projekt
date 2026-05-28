#include "crypto/aes.h"

#include <array>
#include <cstddef>

#include "backend/backend_mode.h"

namespace crypto {

namespace {

// Tablica podstawien AES (S-box) dla kroku SubBytes.
constexpr Byte kSbox[256] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16
};

// Do KeyExpansion
constexpr Byte kRcon[11] = {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36};

// Mnozenie przez x w GF(2^8) (uzywane w MixColumns).
Byte xtime(Byte value) {
    Byte shifted = static_cast<Byte>(value << 1);
    if (value & 0x80) {
        shifted ^= 0x1b;
    }
    return shifted;
}

// Mnozenie przez 2 w GF(2^8).
Byte mul_by_2(Byte value) {
    return xtime(value);
}

// Mnozenie przez 3 w GF(2^8).
Byte mul_by_3(Byte value) {
    return static_cast<Byte>(xtime(value) ^ value);
}

// XOR stanu z kluczem rundy.
void add_round_key(Byte* state, const Byte* round_key) {
    for (size_t i = 0; i < 16; ++i) {
        state[i] = static_cast<Byte>(state[i] ^ round_key[i]);
    }
}

// Zastepuje bajty w stanie zgodnie z S-box.
void sub_bytes(Byte* state) {
    for (size_t i = 0; i < 16; ++i) {
        state[i] = kSbox[state[i]];
    }
}

// Przesuniecia wierszy w macierzy stanu.
void shift_rows(Byte* state) {
    Byte temp[16];
    temp[0] = state[0];
    temp[1] = state[5];
    temp[2] = state[10];
    temp[3] = state[15];

    temp[4] = state[4];
    temp[5] = state[9];
    temp[6] = state[14];
    temp[7] = state[3];

    temp[8] = state[8];
    temp[9] = state[13];
    temp[10] = state[2];
    temp[11] = state[7];

    temp[12] = state[12];
    temp[13] = state[1];
    temp[14] = state[6];
    temp[15] = state[11];

    for (size_t i = 0; i < 16; ++i) {
        state[i] = temp[i];
    }
}

// Mieszanie kolumn w GF(2^8).
void mix_columns(Byte* state) {
    for (size_t col = 0; col < 4; ++col) {
        size_t base = col * 4;
        Byte a0 = state[base + 0];
        Byte a1 = state[base + 1];
        Byte a2 = state[base + 2];
        Byte a3 = state[base + 3];

        state[base + 0] = static_cast<Byte>(mul_by_2(a0) ^ mul_by_3(a1) ^ a2 ^ a3);
        state[base + 1] = static_cast<Byte>(a0 ^ mul_by_2(a1) ^ mul_by_3(a2) ^ a3);
        state[base + 2] = static_cast<Byte>(a0 ^ a1 ^ mul_by_2(a2) ^ mul_by_3(a3));
        state[base + 3] = static_cast<Byte>(mul_by_3(a0) ^ a1 ^ a2 ^ mul_by_2(a3));
    }
}

// Rozwija klucz 128-bit do 11 kluczy rundowych (176 bajtow).
void key_expansion(const AesBlock& key, std::array<Byte, 176>& round_keys) {
    for (size_t i = 0; i < 16; ++i) {
        round_keys[i] = key[i];
    }

    size_t bytes_generated = 16;
    size_t rcon_index = 1;
    Byte temp[4];

    while (bytes_generated < round_keys.size()) {
        for (size_t i = 0; i < 4; ++i) {
            temp[i] = round_keys[bytes_generated - 4 + i];
        }

        if (bytes_generated % 16 == 0) {
            Byte swap = temp[0];
            temp[0] = temp[1];
            temp[1] = temp[2];
            temp[2] = temp[3];
            temp[3] = swap;

            for (size_t i = 0; i < 4; ++i) {
                temp[i] = kSbox[temp[i]];
            }

            temp[0] = static_cast<Byte>(temp[0] ^ kRcon[rcon_index]);
            ++rcon_index;
        }

        for (size_t i = 0; i < 4; ++i) {
            round_keys[bytes_generated] = static_cast<Byte>(round_keys[bytes_generated - 16] ^ temp[i]);
            ++bytes_generated;
        }
    }
}

// Szyfruje pojedynczy blok AES-128 (16 bajtow).
void aes_encrypt_block(const AesBlock& input, AesBlock& output, const std::array<Byte, 176>& round_keys) {
    Byte state[16];
    for (size_t i = 0; i < 16; ++i) {
        state[i] = input[i];
    }

    add_round_key(state, round_keys.data());

    for (size_t round = 1; round <= 9; ++round) {
        sub_bytes(state);
        shift_rows(state);
        mix_columns(state);
        add_round_key(state, round_keys.data() + round * 16);
    }

    sub_bytes(state);
    shift_rows(state);
    add_round_key(state, round_keys.data() + 10 * 16);

    for (size_t i = 0; i < 16; ++i) {
        output[i] = state[i];
    }
}

// Inkremenuje 128-bitowy licznik (CTR) w porzadku big-endian.
void increment_counter(AesBlock& counter) {
    for (int i = 15; i >= 0; --i) {
        Byte next = static_cast<Byte>(counter[i] + 1);
        counter[i] = next;
        if (next != 0) {
            break;
        }
    }
}

// CPU implementacja AES-CTR: generuje strumien klucza i XOR z danymi.
std::vector<Byte> aes_ctr_crypt_cpu(const std::vector<Byte>& input,
                                   const AesBlock& key,
                                   const AesBlock& iv) {
    std::array<Byte, 176> round_keys;
    key_expansion(key, round_keys);

    std::vector<Byte> output(input.size());
    AesBlock counter = iv;
    AesBlock keystream;

    size_t offset = 0;
    while (offset < input.size()) {
        aes_encrypt_block(counter, keystream, round_keys);
        size_t block_size = input.size() - offset;
        if (block_size > 16) {
            block_size = 16;
        }

        for (size_t i = 0; i < block_size; ++i) {
            output[offset + i] = static_cast<Byte>(input[offset + i] ^ keystream[i]);
        }

        offset += block_size;
        increment_counter(counter);
    }

    return output;
}

}

// Publiczny wrapper: docelowo wybiera GPU, obecnie zawsze CPU.
std::vector<Byte> aes_ctr_crypt(const std::vector<Byte>& input,
                               const AesBlock& key,
                               const AesBlock& iv,
                               BackendMode mode) {
    if (uses_gpu_batch(mode)) {
        // TODO: replace with GPU batch kernel when available.
        return aes_ctr_crypt_cpu(input, key, iv);
    }

    return aes_ctr_crypt_cpu(input, key, iv);
}

// Wersja in-place z tym samym API.
void aes_ctr_crypt_inplace(std::vector<Byte>& data,
                           const AesBlock& key,
                           const AesBlock& iv,
                           BackendMode mode) {
    std::vector<Byte> output = aes_ctr_crypt(data, key, iv, mode);
    data.swap(output);
}

}
