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
    requires(N > 0)
struct Bitset {
    using primary_type = std::size_t;

    constexpr static std::size_t k_cell_bitcount = sizeof(primary_type) * 8;
    constexpr static std::size_t k_array_size =
        (N < k_cell_bitcount) ? (N / k_cell_bitcount) : k_cell_bitcount;

    std::array<primary_type, k_array_size> bytes = {0};

    constexpr Bitset() noexcept
    {
    }

    [[nodiscard]] constexpr auto test(const std::size_t index) const noexcept -> bool
    {
        if (index >= N) {
            return false;
        }

        const auto bit = 1ull << static_cast<primary_type>(index % k_cell_bitcount);

        return bytes[index / k_cell_bitcount] & bit;
    }

    constexpr void set(const std::size_t index, const bool value) noexcept
    {
        if (index >= N) {
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
