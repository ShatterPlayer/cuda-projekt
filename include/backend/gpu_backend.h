#ifndef GPU_BACKEND_H
#define GPU_BACKEND_H

#include <cstdint>
#include <vector>

namespace gpu_backend {

// Zwraca true jeśli GPU backend jest dostępny (czy kernele można uruchomić)
bool is_available();

// Interfejs operacji modularnych przyspieszonych na GPU (obecnie fallback do CPU)
uint64_t mod_add(uint64_t a, uint64_t b, uint64_t mod);
uint64_t mod_sub(uint64_t a, uint64_t b, uint64_t mod);
uint64_t mod_mul(uint64_t a, uint64_t b, uint64_t mod);
uint64_t mod_pow(uint64_t base, uint64_t exponent, uint64_t mod);

// Wsadowe potęgowania modularne na GPU
std::vector<uint64_t> mod_pow_many(const std::vector<uint64_t>& base_values,
                                   const std::vector<uint64_t>& exponent_values,
                                   uint64_t mod);

}

#endif
