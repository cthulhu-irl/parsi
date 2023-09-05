#ifndef PARSI_CHARSET_HPP
#define PARSI_CHARSET_HPP

#include <string_view>

#include "parsi/internal/bitset.hpp"

namespace parsi {

struct Charset {
    internal::Bitset<256> map;

    constexpr explicit Charset(std::string_view charset) noexcept
    {
        for (char character : charset) {
            map.set(static_cast<unsigned char>(character), true);
        }
    }

    [[nodiscard]] constexpr auto contains(char character) const noexcept -> bool
    {
        return map.test(static_cast<std::size_t>(character));
    }
};

}  // namespace parsi

#endif  // PARSI_CHARSET_HPP
