#ifndef BIG_INT_H
#define BIG_INT_H

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace crypto {

class BigInt {
public:
    static constexpr uint32_t base = 1000000000U;

    // Konstruktor: inicjalizuje wartość równą 0
    BigInt();
    // Konstruktor: inicjalizuje z 64-bitowej wartości całkowitej
    BigInt(uint64_t value);
    // Konstruktor: inicjalizuje z tekstowej reprezentacji dziesiętnej
    explicit BigInt(const std::string& text);

    // Zwraca true jeśli liczba jest równa zero
    bool is_zero() const;
    // Zwraca true jeśli liczba jest nieparzysta
    bool is_odd() const;
    // Zwraca długość w bitach (pozycję najbardziej znaczącego bitu)
    size_t bit_length() const;
    // Zwraca dziesiętną reprezentację jako string
    std::string to_string() const;

    // Dzieli liczbę przez 2 (podział bitowy)
    void div2();
    // Dodaje małą wartość (używaną przy budowaniu/liczbach pomocniczych)
    void add_small(uint32_t value);
    // Mnoży przez małą wartość (używane wewnętrznie przy dzieleniu/mnożeniu)
    BigInt mul_small(uint64_t value) const;

    friend bool operator==(const BigInt& left, const BigInt& right);
    friend bool operator!=(const BigInt& left, const BigInt& right);
    friend bool operator<(const BigInt& left, const BigInt& right);
    friend bool operator<=(const BigInt& left, const BigInt& right);
    friend bool operator>(const BigInt& left, const BigInt& right);
    friend bool operator>=(const BigInt& left, const BigInt& right);

    friend BigInt operator+(const BigInt& left, const BigInt& right);
    friend BigInt operator-(const BigInt& left, const BigInt& right);
    friend BigInt operator*(const BigInt& left, const BigInt& right);

    // Zwraca parę (iloraz, reszta) z dzielenia całkowitego
    friend std::pair<BigInt, BigInt> divmod(const BigInt& left, const BigInt& right);

    std::vector<uint32_t> digits;

    // Usuwa wiodące zera w wewnętrznym buforze cyfr
    void trim();
};

BigInt mod(const BigInt& value, const BigInt& modulus);
BigInt gcd(BigInt left, BigInt right);
BigInt mod_inverse(const BigInt& value, const BigInt& modulus);
BigInt mod_pow(BigInt base, BigInt exponent, const BigInt& modulus);

}

#endif
