# RSA / DSA CPU + CUDA Boilerplate

Prosty szkielet projektu do dalszej rozbudowy. Obecna implementacja działa w 100% na CPU.

## Co zawiera

- działające implementacje RSA i DSA na CPU,
- własny prosty BigInt do arytmetyki wielosłownej,
- przełącznik backendu uruchamiany z linii poleceń,
- pliki-stuby pod przyszłe kernele CUDA dla warstwy batch i arytmetyki.

## Build

```bash
cmake -S . -B build
cmake --build build
```

Opcja `ENABLE_CUDA=ON` tylko dołącza puste pliki `.cu` jako miejsce na przyszłą implementację.

## Uruchomienie

```bash
./build/rsa_dsa_app --backend=cpu
```

Możliwe wartości `--backend`:

- `cpu`
- `gpu-batch`
- `gpu-arithmetic`
- `gpu-hybrid`
