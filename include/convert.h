#ifndef CPPSV_INCLUDE_CONVERT_H
#define CPPSV_INCLUDE_CONVERT_H

#include <optional>
#include <algorithm>

namespace cppsv {
    // Convert a single character that represents
    // an integer digit (up to base 36) to its value representation
    template <typename CharT>
    inline constexpr int chrdigit(CharT chr, int base) noexcept {
        constexpr char digit_lut_base16[]{
             0, 1, 2, 3, 4, 5, 6, 7, 8, 9,-1,-1,-1,-1,-1,-1,
            -1,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,
            25,26,27,28,29,30,31,32,33,34,35,-1,-1,-1,-1,-1,
            -1,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,
            25,26,27,28,29,30,31,32,33,34,35
        };
        if (static_cast<unsigned int>(chr) < '0' || static_cast<unsigned int>(chr) > 'z')
            return -1; // Not in the lut
        int digit = digit_lut_base16[chr - '0'];
        return digit < base ? digit : -1; // Check if digit exists in base
    }

    // Convert an ASCII character to its lowercase counterpart
    template <typename CharT>
    inline constexpr int chrlower(CharT chr) noexcept {
        if (static_cast<unsigned int>(chr) >= 'A' && static_cast<unsigned int>(chr) <= 'Z')
            return chr + 0x20; // Upper to lower
        if (static_cast<unsigned int>(chr) >= 'a' && static_cast<unsigned int>(chr) <= 'z')
            return chr; // Already lower
        return -1; // Not an ASCII letter
    }

    // Convert a character range between first and last to an integer
    // Supports base 2, 10 and 16 prefixes, radixes 2-36
    template <typename Integer, typename It>
    inline constexpr std::optional<Integer> to_integer(It first, It last, Integer = {}, int radix = 10) noexcept {
        // Trim leading and trailing characters
        do {
            if (first == last) return std::nullopt;
            auto prev = last - 1;
            if (*first == ' ')
                ++first;
            else if(*prev == ' ' || *prev == '\0')
                last = prev;
            else break;
        } while (true);
        bool sign = *first == '-';
        if (sign) ++first;
        if (first == last) return std::nullopt;
        int base = radix;
        // 0x, 0o, 0b prefix notation check (can use uppercase)
        if (*first == '0') {
            It it_chr = ++first;
            if (it_chr == last) {
                return Integer{};
            }
            auto chr = *it_chr;
            first = ++it_chr;
            switch (chrlower(chr)) {
            case 'x':
                base = 16;
                break;
            case 'o':
                base = 8;
                break;
            case 'b':
                base = 2;
                break;
            default:
                if (chrdigit(chr, 10) < 0) return std::nullopt;
            }
        }
        Integer result{};
        while (first != last) {
            auto chr = *(first++);
            int digit = chrdigit(chr, base);
            if (digit < 0) return std::nullopt;
            result = result * base + digit;
        }
        // Return signed result
        return sign ? -result : result;
    }

    template <typename CharT>
    struct fp_constants {
        static constexpr CharT infinity[]{ 'i', 'n', 'f', 'i', 'n', 'i', 't', 'y' };
        static constexpr CharT inf[]{ 'i', 'n', 'f' };
        static constexpr CharT nan[]{ 'n', 'a', 'n' };
    };

    template <typename Fp, typename It>
    inline constexpr auto check_fp_constants(It first, It last, Fp default_result) noexcept {
        using value_type = typename std::iterator_traits<It>::value_type;
        using constants_type = fp_constants<typename std::iterator_traits<It>::value_type>;
        auto pred = [](value_type first, value_type second) { return chrlower(first) == second; };
        if (std::equal(first, last, std::begin(constants_type::infinity),
            std::end(constants_type::infinity), pred)
            || std::equal(first, last, std::begin(constants_type::inf),
            std::end(constants_type::inf), pred)) {
            return std::numeric_limits<Fp>::infinity();
        }
        else if (std::equal(first, last, std::begin(constants_type::nan),
            std::end(constants_type::nan), pred)) {
            return std::numeric_limits<Fp>::quiet_NaN();
        }
        else return default_result;
    }

    // Convert a character range between first and last to a floating point number
    // Supports normal and E notation, but not hexadecimal floating poing notation
    template <typename Fp, typename It>
    inline constexpr std::optional<Fp> to_floating_point(It first, It last, Fp = {}) noexcept {
        // Trim leading and trailing characters
        do {
            if (first == last) return std::nullopt;
            auto prev = last - 1;
            if (*first == ' ')
                ++first;
            else if(*prev == ' ' || *prev == '\0')
                last = prev;
            else break;
        } while (true);
        bool sign = *first == '-';
        if (sign) ++first;
        if (first == last) return std::nullopt;
        // Could be a FP constant ("nan", "inf", "infinity" in any case)
        if (chrlower(*first) == 'i' || chrlower(*first) == 'n') {
            Fp default_result = Fp{};
            Fp result = check_fp_constants(first, last, default_result);
            // Already matched the start of some non-numeric character sequence
            // return unconditionally
            if (result == default_result) return std::nullopt;
            else return sign ? -result : result;
        }
        // Find exponent part (e, E), if present
        It first_exp = first;
        while (++first_exp != last)
            if (chrlower(*first_exp) == 'e') break;
        // Calculate base 10 whole part
        // Iterate from most significant digit forward
        Fp result{};
        auto last_decimal = first_exp;
        while (first != last_decimal) {
            auto chr = *(first++);
            if (chr == '.') break;
            int digit = chrdigit(chr, 10);
            if (digit < 0) return std::nullopt;
            result = result * static_cast<Fp>(10.0) + static_cast<Fp>(digit);
        }
        // Calculate base 10 fractional part
        // Iterate from least significant digit backward
        Fp decimals{};
        while (first != last_decimal) {
            auto chr = *(--last_decimal);
            int digit = chrdigit(chr, 10);
            if (digit < 0) return std::nullopt;
            decimals = decimals / static_cast<Fp>(10.0) + static_cast<Fp>(digit);
        }
        // The first decimal digit is still in the whole part, adjust and add
        result += decimals / static_cast<Fp>(10.0);
        if (first_exp != last) {
            // Calculate exponent (integer only!)
            auto exponent_opt = to_integer(++first_exp, last, 0);
            if (!exponent_opt) return std::nullopt;
            int exponent = exponent_opt.value();
            if (exponent > 0)
                while (exponent--) result *= static_cast<Fp>(10.0);
            else if (exponent < 0)
                while (exponent++) result /= static_cast<Fp>(10.0);
        }
        // Return signed result
        return sign ? -result : result;
    }
}

#endif /* CPPSV_INCLUDE_CONVERT_H */
