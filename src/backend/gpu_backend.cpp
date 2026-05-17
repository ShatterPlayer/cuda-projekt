#include "backend/gpu_backend.h"

#include "backend/backend_mode.h"
#include "backend/cpu_backend.h"

namespace gpu_backend {

// Sprawdza, czy GPU backend jest dostępny (aktualnie zawsze false)
bool is_available() {
    return false;
}

uint64_t mod_add(uint64_t a, uint64_t b, uint64_t mod) {
    return cpu_backend::mod_add(a, b, mod);
}

uint64_t mod_sub(uint64_t a, uint64_t b, uint64_t mod) {
    return cpu_backend::mod_sub(a, b, mod);
}

uint64_t mod_mul(uint64_t a, uint64_t b, uint64_t mod) {
    return cpu_backend::mod_mul(a, b, mod);
}

uint64_t mod_pow(uint64_t base, uint64_t exponent, uint64_t mod) {
    return cpu_backend::mod_pow(base, exponent, mod);
}

std::vector<uint64_t> mod_pow_many(const std::vector<uint64_t>& base_values,
                                   const std::vector<uint64_t>& exponent_values,
                                   uint64_t mod) {
    return cpu_backend::mod_pow_many(base_values, exponent_values, mod, BackendMode::CpuOnly);
}

}
