#ifndef PARSI_INTERNAL_BITSET_HPP
#define PARSI_INTERNAL_BITSET_HPP

#include <array>
#include <cstdint>
#include <span>

namespace parsi::internal {

/**
 * Bitset provides a fast Bitset container
 * similar to std::bitset, but constexpr friendly.
 */
template <std::size_t N>
class Bitset {
    static_assert(N > 0);

    using primary_type = std::size_t;

    constexpr static std::size_t k_cell_bitcount = sizeof(primary_type) * 8;
    constexpr static std::size_t k_array_size = std::max(N / k_cell_bitcount, k_cell_bitcount);

    std::array<primary_type, k_array_size> _bytes = {0};

public:
    constexpr Bitset() noexcept
    {
    }
    
    [[nodiscard]] constexpr auto as_byte_span() const noexcept -> std::span<const primary_type, k_array_size>
    {
        return _bytes;
    }

    /**
     * test whether the bit at given index is set or not.
     */
    [[nodiscard]] constexpr auto test(const std::size_t index) const noexcept -> bool
    {
        if (index >= N) [[unlikely]] {
            return false;
        }

        const auto bit = 1ull << static_cast<primary_type>(index % k_cell_bitcount);

        return _bytes[index / k_cell_bitcount] & bit;
    }

    /**
     * set the bit value at given index.
     */
    constexpr void set(const std::size_t index, const bool value) noexcept
    {
        if (index >= N) [[unlikely]] {
            return;
        }

        const auto bit = 1ull << static_cast<primary_type>(index % k_cell_bitcount);

        if (value) {
            _bytes[index / k_cell_bitcount] |= bit;
        }
        else {
            _bytes[index / k_cell_bitcount] &= ~bit;
        }
    }

    template <std::size_t M>
    constexpr void set(const Bitset<M>& other) noexcept
    {
        static_assert(M <= N, "given bitset is a larger set.");

        auto other_bytes = other.as_byte_span();
        for (std::size_t index = 0; index < std::size(other_bytes); ++index) {
            _bytes[index] |= other_bytes[index];
        }
    }

    template <std::size_t M>
    [[nodiscard]] constexpr auto joined(const Bitset<M>& other) const noexcept -> Bitset<std::max(N, M)>
    {
        Bitset<std::max(N, M)> bitset;

        bitset.set(*this);
        bitset.set(other);

        return bitset;
    }

    template <std::size_t M>
    [[nodiscard]] friend constexpr bool operator==(const Bitset& lhs, const Bitset<M>& rhs) noexcept
    {
        if constexpr (N != M) {
            return false;
        } else {
            return lhs._bytes == rhs._bytes;
        }
    }

    template <std::size_t M>
    [[nodiscard]] friend constexpr bool operator!=(const Bitset& lhs, const Bitset<M>& rhs) noexcept
    {
        if constexpr (N != M) {
            return true;
        } else {
            return lhs._bytes != rhs._bytes;
        }
    }
};

}  // namespace parsi::internal

#endif  // PARSI_INTERNAL_BITSET_HPP
