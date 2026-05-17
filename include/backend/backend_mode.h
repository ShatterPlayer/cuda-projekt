#ifndef BACKEND_MODE_H
#define BACKEND_MODE_H

// Tryb działania backendu: tylko CPU lub różne tryby użycia GPU
enum class BackendMode {
    CpuOnly,
    GpuBatch,
    GpuArithmetic,
    GpuBatchAndArithmetic
};

// Zwraca true jeśli wybrano tryb wykorzystujący wsadowe potęgowania na GPU
inline bool uses_gpu_batch(BackendMode mode) {
    return mode == BackendMode::GpuBatch || mode == BackendMode::GpuBatchAndArithmetic;
}

// Zwraca true jeśli wybrano tryb wykorzystujący arytmetykę na GPU
inline bool uses_gpu_arithmetic(BackendMode mode) {
    return mode == BackendMode::GpuArithmetic || mode == BackendMode::GpuBatchAndArithmetic;
}

#endif
