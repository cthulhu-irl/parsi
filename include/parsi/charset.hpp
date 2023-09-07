#ifndef PARSI_CHARSET_HPP
#define PARSI_CHARSET_HPP

#include <span>
#include <string_view>

#include "parsi/internal/bitset.hpp"

namespace parsi {

class Charset {
    internal::Bitset<256> _map;

public:
    constexpr explicit Charset(const char* charset) noexcept
    {
        for (std::size_t index = 0; charset[index] != '\0'; ++index) {
            _map.set(static_cast<unsigned char>(charset[index]), true);
        }
    }

    constexpr explicit Charset(const char* charset, std::size_t size) noexcept
        : Charset(std::string_view(charset, size))
    {
    }

    constexpr explicit Charset(const unsigned char* charset, std::size_t size) noexcept
        : Charset(std::span(charset, size))
    {
    }

    constexpr explicit Charset(std::string_view charset) noexcept
    {
        for (auto character : charset) {
            _map.set(static_cast<unsigned char>(character), true);
        }
    }

    constexpr explicit Charset(std::span<const std::uint8_t> byteset) noexcept
    {
        for (auto byte : byteset) {
            _map.set(byte, true);
        }
    }

    constexpr explicit Charset(std::initializer_list<std::uint8_t> byteset) noexcept
    {
        for (auto byte : byteset) {
            _map.set(byte, true);
        }
    }

    [[nodiscard]] constexpr auto contains(std::uint8_t character) const noexcept -> bool
    {
        return _map.test(static_cast<std::size_t>(character));
    }
};

}  // namespace parsi

#endif  // PARSI_CHARSET_HPP
