#ifndef PARSI_INTERNAL_BITSET_HPP
#define PARSI_INTERNAL_BITSET_HPP

#include <array>
#include <cstdint>

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
    constexpr static std::size_t k_array_size =
        (N < k_cell_bitcount) ? (N / k_cell_bitcount) : k_cell_bitcount;

    std::array<primary_type, k_array_size> bytes = {0};

public:
    constexpr Bitset() noexcept
    {
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

        return bytes[index / k_cell_bitcount] & bit;
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
            bytes[index / k_cell_bitcount] |= bit;
        }
        else {
            bytes[index / k_cell_bitcount] &= ~bit;
        }
    }
};

}  // namespace parsi::internal

#endif  // PARSI_INTERNAL_BITSET_HPP