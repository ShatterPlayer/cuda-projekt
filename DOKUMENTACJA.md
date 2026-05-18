# Dokumentacja projektu: RSA/DSA CPU + CUDA

## Spis treści

1. [Ogólny opis projektu](#1-ogólny-opis-projektu)
2. [Architektura i struktura plików](#2-architektura-i-struktura-plików)
3. [Moduł BigInt – arytmetyka wielosłowna](#3-moduł-bigint--arytmetyka-wielosłowna)
4. [Moduł arytmetyki modularnej](#4-moduł-arytmetyki-modularnej)
5. [Algorytm RSA](#5-algorytm-rsa)
6. [Algorytm DSA](#6-algorytm-dsa)
7. [Warstwa backendu](#7-warstwa-backendu)
8. [Szkielety kerneli CUDA](#8-szkielety-kerneli-cuda)
9. [Punkt wejścia – main.cpp](#9-punkt-wejścia--maincpp)
10. [Budowanie projektu](#10-budowanie-projektu)
11. [Uruchomienie](#11-uruchomienie)
12. [Przykładowe wyjście](#12-przykładowe-wyjście)
13. [Plany na przyszłość](#13-plany-na-przyszłość)

---

## 1. Ogólny opis projektu

Projekt to szkielet aplikacji implementującej dwa klasyczne algorytmy kryptografii asymetrycznej:

- **RSA** (Rivest–Shamir–Adleman) – szyfrowanie i deszyfrowanie.
- **DSA** (Digital Signature Algorithm) – podpisywanie i weryfikacja podpisów.

Całość działa **w pełni na CPU**. Jednocześnie projekt zawiera gotową warstwę abstrakcji backendu oraz puste pliki kerneli CUDA, dzięki czym można w przyszłości przenieść obliczenia na GPU bez zmiany logiki wyższego poziomu.

Projekt powstał jako zalążek pod pracę semestralną z przedmiotu *Architektura i programowanie GPU*. Celem docelowym jest zrównoleglenie kosztownych operacji modularnych (potęgowanie, mnożenie) na karcie graficznej.

---

## 2. Architektura i struktura plików

```
cuda-projekt/
├── CMakeLists.txt                  # System budowania (CMake 3.16+)
├── README.md
├── include/
│   ├── backend/
│   │   ├── backend_mode.h          # Enum BackendMode + pomocnicze inline'y
│   │   ├── cpu_backend.h           # Interfejs CPU (operacje na uint64_t)
│   │   └── gpu_backend.h           # Interfejs GPU (na razie fallback do CPU)
│   └── crypto/
│       ├── big_int.h               # Klasa BigInt (arytmetyka wielosłowna)
│       ├── mod_arithmetic.h        # Arytmetyka modularna na BigInt
│       ├── rsa.h                   # API algorytmu RSA
│       └── dsa.h                   # API algorytmu DSA
├── src/
│   ├── main.cpp                    # Punkt wejścia, demonstracja
│   ├── backend/
│   │   ├── cpu_backend.cpp         # Implementacja operacji modularnych na CPU
│   │   └── gpu_backend.cpp         # Stub GPU (deleguje do CPU)
│   └── crypto/
│       ├── big_int.cpp             # Implementacja BigInt
│       ├── mod_arithmetic.cpp      # Implementacja mod_arithmetic
│       ├── rsa.cpp                 # Implementacja RSA
│       └── dsa.cpp                 # Implementacja DSA
└── gpu/
    ├── kernels_batch.cu            # Stub kernela CUDA (paralelizm na poziomie operacji)
    └── kernels_arithmetic.cu       # Stub kernela CUDA (paralelizm arytmetyki BigInt)
```

### Dwie warstwy arytmetyki

Projekt posiada **dwie oddzielne warstwy arytmetyki**, które nie są ze sobą połączone:

| Warstwa | Typ danych | Użycie |
|---------|-----------|--------|
| `cpu_backend` / `gpu_backend` | `uint64_t` | Pomocnicza, używana wewnętrznie do operacji 64-bitowych |
| `crypto::BigInt` + `crypto::mod_*` | `BigInt` | Główna – używana przez RSA i DSA |

---

## 3. Moduł BigInt – arytmetyka wielosłowna

**Pliki:** `include/crypto/big_int.h`, `src/crypto/big_int.cpp`

### Reprezentacja

Klasa `BigInt` przechowuje liczby całkowite dowolnej długości jako wektor cyfr w systemie o podstawie **10⁹** (`base = 1'000'000'000`):

```
digits = [d0, d1, d2, ...]    gdzie liczba = d0 + d1*10⁹ + d2*10¹⁸ + ...
```

Cyfry przechowywane są w kolejności **od najmniej znaczącej**. Dzięki podstawie 10⁹ każda cyfra mieści się w `uint32_t`, a iloczyn dwóch cyfr nie przekracza `uint64_t` – co pozwala wykonywać mnożenie bez 128-bitowych typów.

### Konstruktory

| Konstruktor | Opis |
|-------------|------|
| `BigInt()` | Inicjalizuje zero (pusty wektor cyfr) |
| `BigInt(uint64_t value)` | Inicjalizuje z liczby 64-bitowej, rozkładając ją w systemie 10⁹ |
| `BigInt(const std::string& text)` | Parsuje dziesiętny napis, porcjami po 9 cyfr |

### Metody pomocnicze

| Metoda | Działanie |
|--------|-----------|
| `is_zero()` | Zwraca `true` jeśli wektor cyfr jest pusty |
| `is_odd()` | Sprawdza bit 0 pierwszej (najmniej znaczącej) cyfry |
| `bit_length()` | Liczy bity iteracyjnie przez `div2()` |
| `to_string()` | Odtwarza dziesiętną reprezentację, dopełniając wewnętrzne grupy zerami do 9 cyfr |
| `div2()` | Dzielenie przez 2 – przejście od cyfry najbardziej znaczącej, propagacja reszty |
| `add_small(uint32_t)` | Dodaje małą wartość z propagacją przeniesienia |
| `mul_small(uint64_t)` | Mnoży każdą cyfrę przez skalar, używane w `divmod` |
| `trim()` | Usuwa wiodące zera (elementy `0` na końcu wektora) |

### Operatory arytmetyczne

#### Dodawanie (`operator+`)
Standardowe dodawanie kolumnowe z przeniesieniem, iteracja po dłuższym z wektorów.

#### Odejmowanie (`operator-`)
Zakłada `left >= right` (rzuca wyjątek w przeciwnym razie). Odejmowanie kolumnowe z pożyczką.

#### Mnożenie (`operator*`)
Klasyczny algorytm "schoolbook": dla każdej cyfry `left[i]` mnoży przez każdą cyfrę `right[j]` i dodaje do wyniku na pozycji `i+j`. Złożoność O(n²).

#### Dzielenie całkowite (`divmod`)
Algorytm restoring division z **binarnym wyszukiwaniem** cyfry ilorazu:
1. Przetwarzamy cyfry `left` od najbardziej znaczącej.
2. Dla każdej pozycji szukamy binarnie największego `best` ∈ [0, base-1] takiego, że `right * best ≤ remainder`.
3. Odejmujemy `right * best` od `remainder`.

Zwraca parę `(quotient, remainder)`.

### Funkcje wolnostojące

| Funkcja | Opis |
|---------|------|
| `mod(value, modulus)` | Zwraca `divmod(value, modulus).second` |
| `gcd(a, b)` | Algorytm Euklidesa iteracyjny |
| `mod_inverse(value, modulus)` | Rozszerzony algorytm Euklidesa (przez `extended_gcd`) |
| `mod_pow(base, exp, mod)` | Szybkie potęgowanie modularne (binary exponentiation) |

#### Rozszerzony algorytm Euklidesa (`extended_gcd`)
Funkcja wewnętrzna (anonimowy namespace). Zwraca `(x, y, gcd)` takie, że `x*a + y*b = gcd`. Implementacja korzysta z pomocniczej struktury `SignedBigInt` (wartość + bit znaku), ponieważ `BigInt` jest bez znaku.

#### Szybkie potęgowanie modularne (`mod_pow`)
Algorytm "square and multiply" (binary exponentiation):
```
wynik = 1
while exponent > 0:
    if exponent jest nieparzyste: wynik = (wynik * base) mod m
    base = (base * base) mod m
    exponent >>= 1
```
Złożoność: O(log(exponent)) mnożeń.

---

## 4. Moduł arytmetyki modularnej

**Pliki:** `include/crypto/mod_arithmetic.h`, `src/crypto/mod_arithmetic.cpp`

Warstwa pośrednia między kryptografią (RSA/DSA) a implementacją. Każda funkcja przyjmuje parametr `BackendMode` – docelowo decyduje on, czy obliczenia trafią na CPU czy GPU.

| Funkcja | Wzór | Aktualna implementacja |
|---------|------|----------------------|
| `mod_add(a, b, mod, mode)` | `(a + b) mod m` | `crypto::mod(a + b, modulus)` |
| `mod_sub(a, b, mod, mode)` | `(a - b) mod m` | Obsługa ujemnej różnicy przez `modulus - diff` |
| `mod_mul(a, b, mod, mode)` | `(a * b) mod m` | `crypto::mod(a * b, modulus)` |
| `mod_pow(base, exp, mod, mode)` | `base^exp mod m` | `crypto::mod_pow(...)` |
| `mod_inverse(value, mod, mode)` | `value⁻¹ mod m` | `crypto::mod_inverse(...)` |
| `mod_pow_many(bases, exps, mod, mode)` | `base[i]^exp[i] mod m` ∀i | Pętla po elementach (docelowo: kernel GPU) |

Parametr `BackendMode` jest obecnie **ignorowany** – wszystkie wywołania trafiają do implementacji CPU.

---

## 5. Algorytm RSA

**Pliki:** `include/crypto/rsa.h`, `src/crypto/rsa.cpp`

### Struktury danych

```cpp
struct RsaPublicKey  { BigInt n, e; };   // n = p*q, e = wykładnik publiczny
struct RsaPrivateKey { BigInt n, e; };   // n = p*q, d = wykładnik prywatny
struct RsaKeyPair    { RsaPublicKey public_key; RsaPrivateKey private_key; };
```

### Generowanie kluczy – `create_rsa_key_pair(mode)`

Używa **stałych, małych liczb pierwszych** zamiast losowych:

```
p = 101,  q = 103
n = p * q = 10403
phi(n) = (p-1)*(q-1) = 100 * 102 = 10200
e = 65537  (sprawdzamy gcd(e, phi) == 1; jeśli nie, inkrementujemy e o 2)
d = e⁻¹ mod phi(n)
```

> **Uwaga:** Użycie małych, stałych liczb pierwszych to celowe uproszczenie dla demonstracji. W rzeczywistym RSA `p` i `q` powinny być losowymi liczbami pierwszymi o długości 1024+ bitów.

### Szyfrowanie – `rsa_encrypt(message, public_key, mode)`

```
cipher = message^e mod n
```

Realizowane przez `mod_pow(message, e, n, mode)`.

### Deszyfrowanie – `rsa_decrypt(cipher, private_key, mode)`

```
plain = cipher^d mod n
```

Realizowane przez `mod_pow(cipher, d, n, mode)`.

### Wersje wsadowe

| Funkcja | Opis |
|---------|------|
| `rsa_encrypt_many(messages, key, mode)` | Tworzy wektor wykładników `[e, e, ...]` i wywołuje `mod_pow_many` |
| `rsa_decrypt_many(ciphers, key, mode)` | Analogicznie dla `d` |

Wersje wsadowe to przygotowanie pod przyszłą równoległą realizację na GPU – każda operacja `message[i]^e mod n` jest niezależna od pozostałych.

---

## 6. Algorytm DSA

**Pliki:** `include/crypto/dsa.h`, `src/crypto/dsa.cpp`

### Struktury danych

```cpp
struct DsaParams    { BigInt p, q, g; };  // parametry globalne grupy
struct DsaPublicKey { BigInt y; };        // y = g^x mod p
struct DsaPrivateKey{ BigInt x; };        // x – tajny klucz
struct DsaKeyPair   { DsaParams params; DsaPublicKey public_key; DsaPrivateKey private_key; };
struct DsaSignature { BigInt r, s; };     // podpis (r, s)
```

### Parametry grupy

Projekt używa **małych stałych parametrów DSA** spełniających warunek `q | (p - 1)`:

```
p = 23,  q = 11,  g = 4
```

Klucz prywatny `x` jest losowany z zakresu `[1, 10]` za pomocą `std::mt19937_64`.

> W produkcyjnym DSA `p` ma 1024 bity, `q` – 160 lub 256 bitów.

### Funkcja skrótu – `simple_hash(message, mode)`

Oblicza **SHA-256** (przez OpenSSL `SHA256()`), a następnie interpretuje 32 bajty wyniku jako liczbę BigInt (big-endian, mnożąc iteracyjnie przez 256 i dodając kolejny bajt). Wynik jest redukowany modulo `q` przed użyciem w algorytmie.

### Generowanie kluczy – `create_dsa_key_pair(mode, p_bits, q_bits)`

```
x ← losowe z [1, 10]
y = g^x mod p
```

Argumenty `p_bits`, `q_bits` są przyjmowane, ale **ignorowane** w obecnej wersji (parametry są stałe).

### Podpisywanie – `dsa_sign(message, key_pair, mode)`

Standardowy algorytm DSA:

```
h = SHA256(message) mod q
loop:
    k ← losowe z [1, 10]
    r = (g^k mod p) mod q           (jeśli r == 0: ponów)
    k_inv = k⁻¹ mod q               (jeśli nie istnieje: ponów)
    s = k_inv * (h + x*r) mod q     (jeśli s == 0: ponów)
    return (r, s)
```

### Weryfikacja – `dsa_verify(message, signature, key_pair, mode)`

```
h = SHA256(message) mod q
w = s⁻¹ mod q
u1 = h * w mod q
u2 = r * w mod q
v = (g^u1 * y^u2 mod p) mod q
return v == r
```

### Wersje wsadowe

| Funkcja | Opis |
|---------|------|
| `dsa_sign_many(messages, key_pair, mode)` | Pętla wywołań `dsa_sign` (docelowo: równolegle na GPU) |
| `dsa_verify_many(messages, signatures, key_pair, mode)` | Pętla wywołań `dsa_verify` |

---

## 7. Warstwa backendu

**Pliki:** `include/backend/backend_mode.h`, `include/backend/cpu_backend.h`, `include/backend/gpu_backend.h`, i odpowiednie `.cpp`

### BackendMode

```cpp
enum class BackendMode {
    CpuOnly,               // tylko CPU
    GpuBatch,              // GPU dla operacji wsadowych (batch)
    GpuArithmetic,         // GPU dla arytmetyki BigInt
    GpuBatchAndArithmetic  // GPU dla obu
};
```

Dwa pomocnicze inline'y: `uses_gpu_batch(mode)` i `uses_gpu_arithmetic(mode)`.

### cpu_backend

Namespace `cpu_backend` dostarcza operacje modularne na typach **`uint64_t`**:

| Funkcja | Szczegóły |
|---------|-----------|
| `mod_add` | Bezpieczne dodawanie: `a %= mod; b %= mod; if (a >= mod-b) return a-(mod-b); return a+b;` – unika przepełnienia |
| `mod_sub` | Podobnie bezpieczne odejmowanie z owijaniem |
| `mod_mul` | Algorytm "binary multiplication" (double-and-add), unika przepełnienia przy użyciu samego `mod_add` |
| `mod_pow` | Binary exponentiation na `uint64_t` |
| `mod_inverse` | Rozszerzony algorytm Euklidesa na `long long` |
| `mod_pow_many` | Pętla `mod_pow` dla wektora par (base, exponent) |

### gpu_backend

Namespace `gpu_backend` definiuje **ten sam interfejs co `cpu_backend`**, ale każda funkcja deleguje do `cpu_backend`. `is_available()` zawsze zwraca `false`. To przygotowanie struktury pod przyszłe wywołania kerneli CUDA.

---

## 8. Szkielety kerneli CUDA

**Pliki:** `gpu/kernels_batch.cu`, `gpu/kernels_arithmetic.cu`

Pliki kompilują się tylko przy `-DENABLE_CUDA=ON`. Zawierają wyłącznie deklaracje pustych kerneli z komentarzem TODO:

```cuda
// kernels_batch.cu
__global__ void rsa_dsa_batch_kernel() {
    // TODO: implement batch level parallelism for independent RSA/DSA operations.
}

// kernels_arithmetic.cu
__global__ void rsa_dsa_arithmetic_kernel() {
    // TODO: implement arithmetic level parallelism for big integer operations.
}
```

### Planowane strategie zrównoleglenia

**`kernels_batch.cu` – paralelizm na poziomie operacji:**
Każdy wątek GPU wykonuje jedną niezależną operację RSA/DSA (np. szyfrowanie jednej wiadomości). Wiadomości `m[0], m[1], ..., m[N-1]` są rozkładane po wątkach. Wymaga przeniesienia BigInt lub sprowadzenia kluczy do `uint64_t`.

**`kernels_arithmetic.cu` – paralelizm arytmetyki BigInt:**
Wewnątrz jednej operacji modularnej (np. `mod_pow`) zrównoleglane są same mnożenia BigInt. Trudniejsze, wymaga reprezentacji BigInt zrozumiałej przez GPU (tablice `uint32_t` w pamięci globalnej lub shared).

---

## 9. Punkt wejścia – main.cpp

`src/main.cpp` demonstruje wszystkie możliwości projektu:

### 1. Parsowanie trybu backendu

```cpp
BackendMode mode = parse_backend_mode(argc, argv);
```

Parsuje argument `--backend=<wartość>` z linii poleceń.

### 2. RSA – pojedyncza wiadomość

```cpp
RsaKeyPair rsa_keys = create_rsa_key_pair(mode);
BigInt rsa_message(65);
BigInt rsa_cipher  = rsa_encrypt(rsa_message, rsa_keys.public_key, mode);
BigInt rsa_plain   = rsa_decrypt(rsa_cipher,  rsa_keys.private_key, mode);
```

Szyfruje liczbę 65, wypisuje szyfrogram i odszyfrowany wynik (powinien być z powrotem 65).

### 3. RSA – wsadowe (batch)

```cpp
vector<BigInt> msgs = {42, 43, 44, 45};
auto ciphers = rsa_encrypt_many(msgs, rsa_keys.public_key, mode);
auto plains  = rsa_decrypt_many(ciphers, rsa_keys.private_key, mode);
```

### 4. DSA – pojedyncza wiadomość

```cpp
DsaKeyPair dsa_keys = create_dsa_key_pair(mode);
DsaSignature sig = dsa_sign("Hello DSA", dsa_keys, mode);
bool ok = dsa_verify("Hello DSA", sig, dsa_keys, mode);
```

Wypisuje `r`, `s` i wynik weryfikacji (`true`).

### 5. DSA – wsadowe

```cpp
vector<string> msgs = {"A", "B", "C"};
auto sigs    = dsa_sign_many(msgs, dsa_keys, mode);
auto results = dsa_verify_many(msgs, sigs, dsa_keys, mode);
```

---

## 10. Budowanie projektu

### Wymagania

- CMake ≥ 3.16
- Kompilator C++17 (GCC, Clang)
- OpenSSL (paczka `libssl-dev` lub odpowiednik)
- Opcjonalnie: CUDA Toolkit (dla stubbów GPU)

### Budowanie bez CUDA (domyślnie)

```bash
cmake -S . -B build
cmake --build build
```

### Budowanie ze stubbami CUDA

```bash
cmake -S . -B build -DENABLE_CUDA=ON
cmake --build build
```

Opcja `ENABLE_CUDA=ON` dodaje pliki `.cu` do kompilacji. Kernele są puste – nie zmienia to działania programu.

### Zależności CMake

```cmake
find_package(OpenSSL REQUIRED)
target_link_libraries(rsa_dsa_app PRIVATE OpenSSL::Crypto)
```

OpenSSL jest wymagane bezwarunkowo (SHA-256 w DSA).

---

## 11. Uruchomienie

```bash
./build/rsa_dsa_app [--backend=<tryb>]
```

| Argument | BackendMode | Opis |
|----------|-------------|------|
| `--backend=cpu` | `CpuOnly` | Domyślny tryb, pełna implementacja CPU |
| `--backend=gpu-batch` | `GpuBatch` | Stub – faktycznie wykonuje CPU |
| `--backend=gpu-arithmetic` | `GpuArithmetic` | Stub – faktycznie wykonuje CPU |
| `--backend=gpu-hybrid` | `GpuBatchAndArithmetic` | Stub – faktycznie wykonuje CPU |

Brak argumentu `--backend` traktowany jest jak `--backend=cpu`.

---

## 12. Przykładowe wyjście

```
Backend: CPU
RSA
  message: 65
  cipher:  8437
  plain:   65
RSA batch
  42 -> 7373 -> 42
  43 -> 8743 -> 43
  44 -> 1534 -> 44
  45 -> 6273 -> 45
DSA
  r: 6
  s: 3
  verified: true
DSA batch
  A -> true
  B -> true
  C -> true
```

(Dokładne wartości `cipher`, `r`, `s` mogą się różnić między uruchomieniami, bo klucz prywatny DSA i parametr `k` są losowe.)

---

## 13. Plany na przyszłość

### Krótkoterminowe

- **Rzeczywiste generowanie liczb pierwszych** dla RSA i DSA z użyciem np. testu Millera-Rabina (zastąpienie stałych `p = 101`, `q = 103`, `p = 23`, `q = 11`).
- **Randomizacja kluczy DSA** – aktualnie `p_bits` i `q_bits` są przyjmowane jako argumenty, ale ignorowane.
- **Benchmarki** – pomiar czasu CPU dla różnych rozmiarów wsadu (`N = 100, 1000, 10000`).

### Średnioterminowe – implementacja GPU

- **`kernels_batch.cu`** – kernel uruchamiający każdą operację RSA/DSA w osobnym wątku CUDA:
  - Przesłanie tablic `message[]`, `n`, `e` do pamięci globalnej GPU.
  - Każdy wątek wykonuje `mod_pow` na typach 64-bitowych lub 128-bitowych.
  - Zebranie wyników z powrotem na CPU.

- **`kernels_arithmetic.cu`** – kernel zrównoleglający arytmetykę BigInt:
  - Reprezentacja BigInt jako tablicy `uint32_t[]` w pamięci shared.
  - Równoległe mnożenie cyfr w algorytmie schoolbook lub Karatsuba.

- **Podłączenie `gpu_backend`** do faktycznych wywołań kerneli

### Długoterminowe

- Porównanie wydajności CPU vs GPU dla różnych rozmiarów kluczy (512, 1024, 2048, 4096 bitów).
- Optymalizacja: algorytm Montgomery'ego do szybkiego mnożenia modularnego bez dzielenia.
- Wsparcie dla bibliotek jak cuBLAS do operacji macierzowych wspomagających kryptografię.
