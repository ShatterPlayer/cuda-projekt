# RSA + AES-CTR CPU + CUDA Boilerplate

Prosty szkielet projektu do dalszej rozbudowy. Obecna implementacja działa w 100% na CPU.

## Co zawiera

- działająca implementacja RSA na CPU (z prostym pomiarem czasu),
- prosta implementacja AES-CTR na CPU (z pomiarem czasu),
- własny prosty BigInt do arytmetyki wielosłownej,
- przełącznik backendu uruchamiany z linii poleceń,
- pliki-stuby pod przyszłe kernele CUDA dla batch/arytmetyki oraz AES-CTR.

## Build

```bash
cmake -S . -B build
cmake --build build
```

Opcja `ENABLE_CUDA=ON` tylko dołącza puste pliki `.cu` jako miejsce na przyszłą implementację.

## Uruchomienie

```bash
./build/rsa_app --backend=cpu
```

Program wypisuje wyniki RSA i AES-CTR oraz czasy szyfrowania/deszyfrowania.

Możliwe wartości `--backend`:

- `cpu`
- `gpu-batch`
- `gpu-arithmetic`
- `gpu-hybrid`
