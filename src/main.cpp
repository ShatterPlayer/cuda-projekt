#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "backend/backend_mode.h"
#include "crypto/aes.h"
#include "crypto/rsa.h"

// Parsuje argumenty linii poleceń i zwraca wybrany BackendMode
static BackendMode parse_backend_mode(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        std::string argument = argv[i];
        if (argument == "--backend=cpu") {
            return BackendMode::CpuOnly;
        }
        if (argument == "--backend=gpu-batch") {
            return BackendMode::GpuBatch;
        }
        if (argument == "--backend=gpu-arithmetic") {
            return BackendMode::GpuArithmetic;
        }
        if (argument == "--backend=gpu-hybrid") {
            return BackendMode::GpuBatchAndArithmetic;
        }
    }
    return BackendMode::CpuOnly;
}

// Zwraca czytelną nazwę dla danego BackendMode (używane przy wypisywaniu)
static const char* backend_name(BackendMode mode) {
    switch (mode) {
        case BackendMode::CpuOnly:
            return "CPU";
        case BackendMode::GpuBatch:
            return "GPU batch";
        case BackendMode::GpuArithmetic:
            return "GPU arithmetic";
        case BackendMode::GpuBatchAndArithmetic:
            return "GPU hybrid";
    }
    return "CPU";
}

static std::string bytes_to_hex(const std::vector<crypto::Byte>& bytes) {
    std::ostringstream stream;
    stream << std::hex << std::setfill('0');
    for (crypto::Byte value : bytes) {
        stream << std::setw(2) << static_cast<int>(value);
    }
    return stream.str();
}

int main(int argc, char** argv) {
    BackendMode mode = parse_backend_mode(argc, argv);

    std::cout << "Backend: " << backend_name(mode) << '\n';
    if (mode != BackendMode::CpuOnly) {
        std::cout << "GPU stubs are active. Current build still runs CPU code.\n";
    }

    crypto::RsaKeyPair rsa_keys = crypto::create_rsa_key_pair(mode);
    crypto::BigInt rsa_message(65);
    auto rsa_encrypt_start = std::chrono::steady_clock::now();
    crypto::BigInt rsa_cipher = crypto::rsa_encrypt(rsa_message, rsa_keys.public_key, mode);
    auto rsa_encrypt_end = std::chrono::steady_clock::now();
    auto rsa_encrypt_us = std::chrono::duration_cast<std::chrono::microseconds>(rsa_encrypt_end - rsa_encrypt_start).count();

    auto rsa_decrypt_start = std::chrono::steady_clock::now();
    crypto::BigInt rsa_plain = crypto::rsa_decrypt(rsa_cipher, rsa_keys.private_key, mode);
    auto rsa_decrypt_end = std::chrono::steady_clock::now();
    auto rsa_decrypt_us = std::chrono::duration_cast<std::chrono::microseconds>(rsa_decrypt_end - rsa_decrypt_start).count();

    std::cout << "RSA\n";
    std::cout << "  message: " << rsa_message.to_string() << '\n';
    std::cout << "  cipher:  " << rsa_cipher.to_string() << '\n';
    std::cout << "  plain:   " << rsa_plain.to_string() << '\n';
    std::cout << "  encrypt time: " << rsa_encrypt_us << " us\n";
    std::cout << "  decrypt time: " << rsa_decrypt_us << " us\n";

    std::vector<crypto::BigInt> rsa_batch_messages = {crypto::BigInt(42), crypto::BigInt(43), crypto::BigInt(44), crypto::BigInt(45)};
    std::vector<crypto::BigInt> rsa_batch_ciphers = crypto::rsa_encrypt_many(rsa_batch_messages,
                                                                             rsa_keys.public_key,
                                                                             mode);
    std::vector<crypto::BigInt> rsa_batch_plain = crypto::rsa_decrypt_many(rsa_batch_ciphers,
                                                                           rsa_keys.private_key,
                                                                           mode);

    std::cout << "RSA batch\n";
    for (size_t i = 0; i < rsa_batch_messages.size(); ++i) {
        std::cout << "  " << rsa_batch_messages[i].to_string() << " -> " << rsa_batch_ciphers[i].to_string()
                  << " -> " << rsa_batch_plain[i].to_string() << '\n';
    }

    crypto::AesBlock aes_key = {0x00, 0x01, 0x02, 0x03,
                                0x04, 0x05, 0x06, 0x07,
                                0x08, 0x09, 0x0a, 0x0b,
                                0x0c, 0x0d, 0x0e, 0x0f};
    crypto::AesBlock aes_iv = {0x0f, 0x0e, 0x0d, 0x0c,
                               0x0b, 0x0a, 0x09, 0x08,
                               0x07, 0x06, 0x05, 0x04,
                               0x03, 0x02, 0x01, 0x00};
    std::string aes_message = "Hello AES-CTR Hello AES-CTR Hello AES-CTR Hello AES-CTR Hello AES-CTR Hello AES-CTR";
    std::vector<crypto::Byte> aes_plain(aes_message.begin(), aes_message.end());

    auto aes_encrypt_start = std::chrono::steady_clock::now();
    std::vector<crypto::Byte> aes_cipher = crypto::aes_ctr_crypt(aes_plain, aes_key, aes_iv, mode);
    auto aes_encrypt_end = std::chrono::steady_clock::now();
    auto aes_encrypt_us = std::chrono::duration_cast<std::chrono::microseconds>(aes_encrypt_end - aes_encrypt_start).count();

    auto aes_decrypt_start = std::chrono::steady_clock::now();
    std::vector<crypto::Byte> aes_decrypted = crypto::aes_ctr_crypt(aes_cipher, aes_key, aes_iv, mode);
    auto aes_decrypt_end = std::chrono::steady_clock::now();
    auto aes_decrypt_us = std::chrono::duration_cast<std::chrono::microseconds>(aes_decrypt_end - aes_decrypt_start).count();
    std::string aes_roundtrip(aes_decrypted.begin(), aes_decrypted.end());

    std::cout << "AES-CTR\n";
    std::cout << "  plain:  " << aes_message << '\n';
    std::cout << "  cipher: " << bytes_to_hex(aes_cipher) << '\n';
    std::cout << "  roundtrip: " << aes_roundtrip << '\n';
    std::cout << "  encrypt time: " << aes_encrypt_us << " us\n";
    std::cout << "  decrypt time: " << aes_decrypt_us << " us\n";

    return 0;
}
