#include "crypto/big_int.h"

#include <algorithm>
#include <stdexcept>
#include <tuple>

namespace crypto {

namespace {
// Struktury pomocnicze używane wewnętrznie do rozszerzonego algorytmu Euklidesa
struct SignedBigInt {
    BigInt value;
    bool negative;
};

struct EgcdResult {
    SignedBigInt x;
    SignedBigInt y;
    BigInt gcd;
};

// Porównuje wartości bez znaku (bezwzględne) dwóch BigIntów
int compare_abs(const BigInt& left, const BigInt& right) {
    if (left.digits.size() != right.digits.size()) {
        return left.digits.size() < right.digits.size() ? -1 : 1;
    }

    for (size_t i = left.digits.size(); i > 0; --i) {
        size_t index = i - 1;
        if (left.digits[index] != right.digits[index]) {
            return left.digits[index] < right.digits[index] ? -1 : 1;
        }
    }

    return 0;
}

// Dodaje bezwzględne reprezentacje dwóch BigIntów (bez uwzględniania znaku)
BigInt add_abs(const BigInt& left, const BigInt& right) {
    BigInt result;
    uint64_t carry = 0;
    size_t count = left.digits.size();
    if (right.digits.size() > count) {
        count = right.digits.size();
    }

    result.digits.resize(count);
    for (size_t i = 0; i < count; ++i) {
        uint64_t sum = carry;
        if (i < left.digits.size()) {
            sum += left.digits[i];
        }
        if (i < right.digits.size()) {
            sum += right.digits[i];
        }
        result.digits[i] = static_cast<uint32_t>(sum % BigInt::base);
        carry = sum / BigInt::base;
    }

    if (carry > 0) {
        result.digits.push_back(static_cast<uint32_t>(carry));
    }

    result.trim();
    return result;
}

// Odejmowanie bezwzględne: left - right (zakłada left >= right)
BigInt subtract_abs(const BigInt& left, const BigInt& right) {
    BigInt result;
    result.digits.resize(left.digits.size());

    int64_t carry = 0;
    for (size_t i = 0; i < left.digits.size(); ++i) {
        int64_t current = static_cast<int64_t>(left.digits[i]) - carry;
        if (i < right.digits.size()) {
            current -= right.digits[i];
        }
        if (current < 0) {
            current += BigInt::base;
            carry = 1;
        } else {
            carry = 0;
        }
        result.digits[i] = static_cast<uint32_t>(current);
    }

    result.trim();
    return result;
}

// Tworzy SignedBigInt z BigInt i flagi znaku
SignedBigInt make_signed(BigInt value, bool negative) {
    if (value.is_zero()) {
        negative = false;
    }
    SignedBigInt result;
    result.value = value;
    result.negative = negative;
    return result;
}

// Dodawanie z uwzględnieniem znaków SignedBigInt
SignedBigInt signed_add(const SignedBigInt& left, const SignedBigInt& right) {
    if (left.negative == right.negative) {
        return make_signed(add_abs(left.value, right.value), left.negative);
    }

    int comparison = compare_abs(left.value, right.value);
    if (comparison == 0) {
        return make_signed(BigInt(), false);
    }
    if (comparison > 0) {
        return make_signed(subtract_abs(left.value, right.value), left.negative);
    }
    return make_signed(subtract_abs(right.value, left.value), right.negative);
}

// Odejmowanie SignedBigInt (znaki obsługiwane)
SignedBigInt signed_sub(const SignedBigInt& left, const SignedBigInt& right) {
    SignedBigInt flipped = right;
    flipped.negative = !flipped.negative;
    return signed_add(left, flipped);
}

// Mnożenie SignedBigInt przez bezwzględny BigInt (zachowuje znak left)
SignedBigInt signed_mul(const SignedBigInt& left, const BigInt& right) {
    return make_signed(left.value * right, left.negative);
}

// Konwertuje BigInt na SignedBigInt z dodatnim znakiem
SignedBigInt signed_from_big(const BigInt& value) {
    return make_signed(value, false);
}

// Rozszerzony algorytm Euklidesa: zwraca x,y,gcd takie, że x*left + y*right = gcd
EgcdResult extended_gcd(const BigInt& left, const BigInt& right) {
    if (right.is_zero()) {
        EgcdResult result;
        result.x = signed_from_big(BigInt(1));
        result.y = signed_from_big(BigInt());
        result.gcd = left;
        return result;
    }

    std::pair<BigInt, BigInt> qr = divmod(left, right);
    EgcdResult recursive = extended_gcd(right, qr.second);

    EgcdResult result;
    result.x = recursive.y;
    SignedBigInt qy = signed_mul(recursive.y, qr.first);
    result.y = signed_sub(recursive.x, qy);
    result.gcd = recursive.gcd;
    return result;
}

}

// Konstruktor domyślny (wartość 0)
BigInt::BigInt() {}

// Konstruktor z uint64_t
BigInt::BigInt(uint64_t value) {
    if (value == 0) {
        return;
    }

    while (value > 0) {
        digits.push_back(static_cast<uint32_t>(value % base));
        value /= base;
    }
}

// Konstruktor z napisu dziesiętnego
BigInt::BigInt(const std::string& text) {
    size_t end = text.size();
    while (end > 0) {
        size_t start = 0;
        if (end > 9) {
            start = end - 9;
        }

        uint32_t chunk = static_cast<uint32_t>(std::stoul(text.substr(start, end - start)));
        digits.push_back(chunk);

        if (start == 0) {
            break;
        }
        end = start;
    }

    trim();
}

// Usuwa wiodące zera w wewnętrznym wektorze cyfr
void BigInt::trim() {
    while (!digits.empty() && digits.back() == 0) {
        digits.pop_back();
    }
}

// Zwraca true jeśli liczba jest równa zero
bool BigInt::is_zero() const {
    return digits.empty();
}

// Zwraca true jeśli liczba jest nieparzysta
bool BigInt::is_odd() const {
    if (digits.empty()) {
        return false;
    }
    return (digits.front() & 1U) != 0U;
}

// Zwraca długość liczby w bitach (liczba iteracji dzielenia przez 2)
size_t BigInt::bit_length() const {
    if (digits.empty()) {
        return 0;
    }

    BigInt temp = *this;
    size_t bits = 0;
    while (!temp.is_zero()) {
        temp.div2();
        ++bits;
    }
    return bits;
}

// Zwraca dziesiętną reprezentację liczby
std::string BigInt::to_string() const {
    if (digits.empty()) {
        return "0";
    }

    std::string output = std::to_string(digits.back());
    for (size_t i = digits.size(); i > 1; --i) {
        size_t index = i - 2;
        std::string part = std::to_string(digits[index]);
        output += std::string(9 - part.size(), '0');
        output += part;
    }
    return output;
}

// Dzieli liczbę przez 2 (przesunięcie bitowe w dół)
void BigInt::div2() {
    uint64_t carry = 0;
    for (size_t i = digits.size(); i > 0; --i) {
        size_t index = i - 1;
        uint64_t current = digits[index] + carry * base;
        digits[index] = static_cast<uint32_t>(current / 2U);
        carry = current % 2U;
    }
    trim();
}

// Dodaje małą wartość (mniejszą niż `base`) do liczby
void BigInt::add_small(uint32_t value) {
    uint64_t carry = value;
    size_t index = 0;

    while (carry > 0) {
        if (index == digits.size()) {
            digits.push_back(0);
        }

        uint64_t current = digits[index] + carry;
        digits[index] = static_cast<uint32_t>(current % base);
        carry = current / base;
        ++index;
    }
}

// Mnoży BigInt przez małą wartość (używane w algorytmach pomocniczych)
BigInt BigInt::mul_small(uint64_t value) const {
    BigInt result;
    if (is_zero() || value == 0) {
        return result;
    }

    result.digits.resize(digits.size());
    uint64_t carry = 0;
    for (size_t i = 0; i < digits.size(); ++i) {
        uint64_t current = static_cast<uint64_t>(digits[i]) * value + carry;
        result.digits[i] = static_cast<uint32_t>(current % base);
        carry = current / base;
    }

    while (carry > 0) {
        result.digits.push_back(static_cast<uint32_t>(carry % base));
        carry /= base;
    }

    result.trim();
    return result;
}

// Operatory porównania
bool operator==(const BigInt& left, const BigInt& right) {
    return left.digits == right.digits;
}

bool operator!=(const BigInt& left, const BigInt& right) {
    return !(left == right);
}

bool operator<(const BigInt& left, const BigInt& right) {
    return compare_abs(left, right) < 0;
}

bool operator<=(const BigInt& left, const BigInt& right) {
    return !(right < left);
}

bool operator>(const BigInt& left, const BigInt& right) {
    return right < left;
}

bool operator>=(const BigInt& left, const BigInt& right) {
    return !(left < right);
}

// Dodawanie BigInt (bezwzględne)
BigInt operator+(const BigInt& left, const BigInt& right) {
    return add_abs(left, right);
}

// Odejmowanie BigInt (zakłada left >= right)
BigInt operator-(const BigInt& left, const BigInt& right) {
    if (left < right) {
        throw std::runtime_error("BigInt subtraction requires left >= right");
    }
    return subtract_abs(left, right);
}

// Mnożenie BigInt (standardowa metoda "schoolbook")
BigInt operator*(const BigInt& left, const BigInt& right) {
    BigInt result;
    if (left.is_zero() || right.is_zero()) {
        return result;
    }

    result.digits.assign(left.digits.size() + right.digits.size(), 0);
    for (size_t i = 0; i < left.digits.size(); ++i) {
        uint64_t carry = 0;
        for (size_t j = 0; j < right.digits.size(); ++j) {
            uint64_t current = result.digits[i + j] + static_cast<uint64_t>(left.digits[i]) * right.digits[j] + carry;
            result.digits[i + j] = static_cast<uint32_t>(current % BigInt::base);
            carry = current / BigInt::base;
        }

        size_t index = i + right.digits.size();
        while (carry > 0) {
            uint64_t current = result.digits[index] + carry;
            result.digits[index] = static_cast<uint32_t>(current % BigInt::base);
            carry = current / BigInt::base;
            ++index;
        }
    }

    result.trim();
    return result;
}

// Dzielenie całkowite: zwraca (iloraz, reszta)
std::pair<BigInt, BigInt> divmod(const BigInt& left, const BigInt& right) {
    if (right.is_zero()) {
        throw std::runtime_error("division by zero");
    }

    if (left < right) {
        return std::make_pair(BigInt(), left);
    }

    BigInt remainder;
    BigInt quotient;
    quotient.digits.resize(left.digits.size());

    for (size_t index = left.digits.size(); index > 0; --index) {
        remainder = remainder.mul_small(BigInt::base);
        remainder.add_small(left.digits[index - 1]);

        uint32_t low = 0;
        uint32_t high = BigInt::base - 1;
        uint32_t best = 0;

        while (low <= high) {
            uint32_t mid = low + (high - low) / 2U;
            BigInt candidate = right.mul_small(mid);

            if (candidate <= remainder) {
                best = mid;
                low = mid + 1U;
            } else {
                if (mid == 0U) {
                    break;
                }
                high = mid - 1U;
            }
        }

        quotient.digits[index - 1] = best;
        if (best > 0U) {
            remainder = remainder - right.mul_small(best);
        }
    }

    quotient.trim();
    remainder.trim();
    return std::make_pair(quotient, remainder);
}

// Zwraca resztę z dzielenia value mod modulus
BigInt mod(const BigInt& value, const BigInt& modulus) {
    return divmod(value, modulus).second;
}

// Oblicza największy wspólny dzielnik (algorytm Euklidesa)
BigInt gcd(BigInt left, BigInt right) {
    while (!right.is_zero()) {
        BigInt remainder = divmod(left, right).second;
        left = right;
        right = remainder;
    }
    return left;
}

// Oblicza odwrotność modularną value^{-1} mod modulus (jeśli istnieje)
BigInt mod_inverse(const BigInt& value, const BigInt& modulus) {
    EgcdResult result = extended_gcd(mod(value, modulus), modulus);
    if (result.gcd != BigInt(1)) {
        return BigInt();
    }

    BigInt inverse = mod(result.x.value, modulus);
    if (result.x.negative && !inverse.is_zero()) {
        inverse = modulus - inverse;
    }
    return inverse;
}

// Szybkie potęgowanie modularne: base_value^{exponent} mod modulus
BigInt mod_pow(BigInt base_value, BigInt exponent, const BigInt& modulus) {
    if (modulus.is_zero()) {
        return BigInt();
    }

    BigInt result(1);
    base_value = mod(base_value, modulus);

    while (!exponent.is_zero()) {
        if (exponent.is_odd()) {
            result = mod(result * base_value, modulus);
        }
        exponent.div2();
        base_value = mod(base_value * base_value, modulus);
    }

    return result;
}

}
