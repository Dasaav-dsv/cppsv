#ifndef CPPSV_INCLUDE_CPPSV_COMMON_H
#define CPPSV_INCLUDE_CPPSV_COMMON_H

#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <iterator>

namespace cppsv {
    // Standard cppsv csv header
    // It is validated before parsing the csv string
    template <typename CharT>
    struct cppsv_header {
        static constexpr CharT value[]{ '"', 'c', 'p', 'p', 's', 'v', '"', '\n' };
        static constexpr size_t size = std::size(value);

        template <typename T>
        static constexpr bool has_header(T&& iterable) noexcept {
            auto begin = std::begin(iterable);
            return std::distance(begin, std::end(iterable)) >= size
                && std::equal(std::begin(value), std::end(value), begin);
        }
    };
}

#endif /* CPPSV_INCLUDE_CPPSV_COMMON_H */