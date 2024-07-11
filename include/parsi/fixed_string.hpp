#ifndef PARSI_FIXED_STRING_HPP
#define PARSI_FIXED_STRING_HPP

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>
#include <span>

namespace parsi {

template <std::size_t SizeV, typename CharT = char>
class FixedString {
    static_assert(sizeof(CharT) == 1);
    static_assert(std::is_arithmetic_v<CharT>);

public:
    using char_type = std::remove_cvref_t<CharT>;
    constexpr static std::size_t array_size = SizeV + 1;

private:
    const std::array<char_type, array_size> _arr = {};
    const std::size_t _size = array_size - 1;

    constexpr static auto internal_make_array(std::span<const char_type> bytes) noexcept
        -> std::array<char_type, array_size>
    {
        const std::size_t min_size = (bytes.size() >= array_size) ? array_size : bytes.size();

        std::array<char_type, array_size> arr = {0};
        for (std::size_t index = 0; index < min_size; ++index) {
            arr[index] = bytes[index];
        }
        return arr;
    }

    constexpr static std::size_t strlen(const char_type* data) noexcept
    {
        std::size_t len = 0;
        while (len < array_size && data[len] != char_type{}) {
            ++len;
        }
        return len;
    }

public:
    constexpr FixedString(const char_type (&arr)[SizeV]) noexcept : _arr(internal_make_array(std::span(arr, SizeV))), _size(strlen(_arr.data())) {}

    constexpr FixedString(const std::array<char_type, array_size>& arr) noexcept : _arr(arr), _size(strlen(_arr.data())) {}

    [[nodiscard]] constexpr static auto make(const char_type* arr, std::size_t size)  noexcept -> std::optional<FixedString>
    {
        if (size >= array_size) {
            return std::nullopt;
        }
        return FixedString(internal_make_array(std::span<const char_type>{arr, size}));
    }

    [[nodiscard]] constexpr static auto make(const char_type (&arr)[SizeV])  noexcept -> std::optional<FixedString>
    {
        return FixedString(internal_make_array(arr));
    }

    [[nodiscard]] constexpr static auto make(const std::array<char_type, array_size>& arr) noexcept -> std::optional<FixedString>
    {
        return FixedString(arr);
    }

    [[nodiscard]] constexpr static auto make(std::string_view str) noexcept -> std::optional<FixedString>
    {
        if (str.size() >= array_size) {
            return std::nullopt;
        }

        return FixedString(internal_make_array(str));
    }

    [[nodiscard]] constexpr const char_type* data() const noexcept { return _arr.data(); }

    [[nodiscard]] constexpr std::size_t size() const noexcept { return _size; }

    [[nodiscard]] constexpr auto as_string_view() const noexcept
        -> std::string_view
    {
        return std::string_view(data(), size());
    }

    [[nodiscard]] constexpr auto as_span() const noexcept
        -> std::span<const char_type>
    {
        return std::span(data(), size());
    }

    [[nodiscard]] constexpr auto as_sized_span() const noexcept
        -> std::span<const char_type, array_size>
    {
        return std::span<const char_type, array_size>(data());
    }

    template <std::size_t OtherSizeV>
    [[nodiscard]] constexpr bool equals(const FixedString<OtherSizeV>& other) const noexcept
    {
        if (size() != other.size()) {
            return false;
        }
        return _arr == other._arr;
    }

    [[nodiscard]] constexpr bool equals(std::string_view other) const noexcept
    {
        return as_string_view() == other;
    }

    [[nodiscard]] constexpr const char_type* begin() const noexcept { return data(); }
    [[nodiscard]] constexpr const char_type* end() const noexcept { return data() + size(); }

    [[nodiscard]] constexpr char_type operator[](std::size_t index) const noexcept { return _arr[index]; }

    [[nodiscard]] constexpr operator std::string_view() const& noexcept { return as_string_view(); }
    [[nodiscard]] constexpr operator std::span<const char_type>() const& noexcept { return as_span(); }

    template <std::size_t RhsSizeV>
    [[nodiscard]] friend constexpr bool operator==(const FixedString& lhs, const FixedString<RhsSizeV>& rhs) noexcept
    {
        return lhs.equals(rhs);
    }

    [[nodiscard]] friend constexpr bool operator==(const FixedString& lhs, std::string_view rhs) noexcept
    {
        return lhs.equals(rhs);
    }

    template <std::size_t RhsSizeV>
    [[nodiscard]] friend constexpr bool operator!=(const FixedString& lhs, const FixedString<RhsSizeV>& rhs) noexcept
    {
        return !lhs.equals(rhs);
    }

    [[nodiscard]] friend constexpr bool operator!=(const FixedString& lhs, std::string_view rhs) noexcept
    {
        return !lhs.equals(rhs);
    }
};

}  // namespace parsi

#endif  // PARSI_FIXED_STRING_HPP
