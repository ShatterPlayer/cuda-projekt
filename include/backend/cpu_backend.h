#ifndef CPU_BACKEND_H
#define CPU_BACKEND_H

#include <cstdint>
#include <vector>

#include "backend/backend_mode.h"

namespace cpu_backend {

// Proste operacje modularne realizowane na CPU (dla typów 64-bitowych)
uint64_t mod_add(uint64_t a, uint64_t b, uint64_t mod);
uint64_t mod_sub(uint64_t a, uint64_t b, uint64_t mod);
uint64_t mod_mul(uint64_t a, uint64_t b, uint64_t mod);
uint64_t mod_pow(uint64_t base, uint64_t exponent, uint64_t mod);
uint64_t mod_inverse(uint64_t value, uint64_t mod);

std::vector<uint64_t> mod_pow_many(const std::vector<uint64_t>& base_values,
                                   const std::vector<uint64_t>& exponent_values,
                                   uint64_t mod,
                                   BackendMode mode);

}

#endif
