#include <iostream>
#include <string>
#include <vector>

#include "backend/backend_mode.h"
#include "crypto/dsa.h"
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

int main(int argc, char** argv) {
    BackendMode mode = parse_backend_mode(argc, argv);

    std::cout << "Backend: " << backend_name(mode) << '\n';
    if (mode != BackendMode::CpuOnly) {
        std::cout << "GPU stubs are active. Current build still runs CPU code.\n";
    }

    crypto::RsaKeyPair rsa_keys = crypto::create_rsa_key_pair(mode);
    crypto::BigInt rsa_message(65);
    crypto::BigInt rsa_cipher = crypto::rsa_encrypt(rsa_message, rsa_keys.public_key, mode);
    crypto::BigInt rsa_plain = crypto::rsa_decrypt(rsa_cipher, rsa_keys.private_key, mode);

    std::cout << "RSA\n";
    std::cout << "  message: " << rsa_message.to_string() << '\n';
    std::cout << "  cipher:  " << rsa_cipher.to_string() << '\n';
    std::cout << "  plain:   " << rsa_plain.to_string() << '\n';

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

    crypto::DsaKeyPair dsa_keys = crypto::create_dsa_key_pair(mode);
    std::string message = "Hello DSA";
    crypto::DsaSignature signature = crypto::dsa_sign(message, dsa_keys, mode);
    bool verified = crypto::dsa_verify(message, signature, dsa_keys, mode);

    std::cout << "DSA\n";
    std::cout << "  r: " << signature.r.to_string() << '\n';
    std::cout << "  s: " << signature.s.to_string() << '\n';
    std::cout << "  verified: " << (verified ? "true" : "false") << '\n';

    std::vector<std::string> dsa_messages = {"A", "B", "C"};
    std::vector<crypto::DsaSignature> dsa_signatures = crypto::dsa_sign_many(dsa_messages, dsa_keys, mode);
    std::vector<bool> dsa_results = crypto::dsa_verify_many(dsa_messages, dsa_signatures, dsa_keys, mode);

    std::cout << "DSA batch\n";
    for (size_t i = 0; i < dsa_messages.size(); ++i) {
        std::cout << "  " << dsa_messages[i] << " -> "
                  << (dsa_results[i] ? "true" : "false") << '\n';
    }

    return 0;
}
