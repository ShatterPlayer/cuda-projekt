cmake_minimum_required(VERSION 3.16)

project(rsa_dsa_cuda_boilerplate LANGUAGES CXX)

option(ENABLE_CUDA "Build CUDA stub files" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(rsa_dsa_app
    src/main.cpp
    src/crypto/big_int.cpp
    src/backend/cpu_backend.cpp
    src/backend/gpu_backend.cpp
    src/crypto/mod_arithmetic.cpp
    src/crypto/rsa.cpp
    src/crypto/dsa.cpp
)

target_include_directories(rsa_dsa_app PRIVATE include)

find_package(OpenSSL REQUIRED)
target_link_libraries(rsa_dsa_app PRIVATE OpenSSL::Crypto)

if(ENABLE_CUDA)
    enable_language(CUDA)
    target_sources(rsa_dsa_app PRIVATE
        gpu/kernels_batch.cu
        gpu/kernels_arithmetic.cu
    )
    set_target_properties(rsa_dsa_app PROPERTIES
        CUDA_SEPARABLE_COMPILATION ON
    )
endif()
