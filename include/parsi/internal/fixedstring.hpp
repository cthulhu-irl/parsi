#ifndef PARSI_INTERNAL_FIXEDSTRING_HPP
#define PARSI_INTERNAL_FIXEDSTRING_HPP

#include <array>
#include <cstdint>
#include <span>
#include <string_view>

namespace parsi::internal {

template <std::size_t N>
class FixedString {
    std::array<char, N> _arr;

public:
    constexpr FixedString(const char str[N]) noexcept
    {
        std::copy(std::begin(str), std::end(str), std::begin(_arr));
    }

    // constexpr FixedString(const char* str, std::size_t size) noexcept
    // {
    //     // TODO
    // }

    // template <std::size_t Offset, std::size_t Count>
    //     requires (Count > 0) && (Offset + Count <= N)
    // constexpr auto substr() const noexcept
    // {
    //     return FixedString(_arr.data() + Offset, Count);
    // }

    constexpr auto begin() noexcept -> char* { return _arr.begin(); }
    constexpr auto begin() const noexcept -> const char* { return _arr.begin(); }
    
    constexpr auto end() noexcept -> char* { return _arr.end(); }
    constexpr auto end() const noexcept -> const char* { return _arr.end(); }

    constexpr auto as_span() noexcept -> std::span<char, N>
    {
        return std::span(_arr.data(), _arr.size());
    }

    constexpr auto as_span() const noexcept -> std::span<const char, N>
    {
        return std::span(_arr.data(), _arr.size());
    }

    constexpr auto as_string_view() const noexcept -> std::string_view
    {
        return std::string_view(_arr.data(), _arr.size());
    }

    friend constexpr bool operator==(const FixedString& lhs, const FixedString& rhs) noexcept
    {
        return lhs._arr == rhs._arr;
    }

    template <std::size_t M> requires (N != M)
    friend constexpr bool operator==(const FixedString& lhs, const FixedString<M>& rhs) noexcept
    {
        return false;
    }

    template <std::size_t M> requires (N != M)
    friend constexpr bool operator!=(const FixedString& lhs, const FixedString<M>& rhs) noexcept
    {
        return true;
    }

    friend constexpr bool operator!=(const FixedString& lhs, const FixedString& rhs) noexcept
    {
        return lhs._arr != rhs._arr;
    }
    
    template <std::size_t M> requires (M >= N)
    constexpr operator std::span<char, M>() noexcept { return as_span(); }

    template <std::size_t M> requires (M >= N)
    constexpr operator std::span<const char, M>() const noexcept { return as_span(); }

    constexpr operator std::string_view() const noexcept { return as_string_view(); }
};

template <std::size_t N>
FixedString(const char str[N]) -> FixedString<N>;

}  // namespace parsi::internal

#endif  // PARSI_INTERNAL_FIXEDSTRING_HPP
