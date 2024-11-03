#ifndef PARSI_BASE_HPP
#define PARSI_BASE_HPP

#include <concepts>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>
#include <utility>

namespace parsi {

/**
 * A wrapper for non-owning byte stream buffer.
 */
class Stream {
public:
    const char* _cursor = nullptr;
    std::size_t _size = 0;

public:
    constexpr Stream(const char* str) noexcept : Stream(str, std::strlen(str))
    {
    }

    constexpr Stream(const char* str, const std::size_t size) noexcept : _cursor(str), _size(size)
    {
    }

    constexpr Stream(std::string_view str) noexcept : _cursor(str.data()), _size(str.size())
    {
    }

    constexpr Stream(std::span<const char> buffer) noexcept : _cursor(buffer.data()), _size(buffer.size())
    {
    }

    /**
     * advance the cursor forward by given `count`.
     */
    constexpr void advance(std::size_t count) noexcept
    {
        _cursor += count;
        _size -= count;
    }

    /**
     * returns a copy of this stream that is advanced forward by `count` bytes.
     */
    [[nodiscard]] constexpr auto advanced(std::size_t count) const noexcept -> Stream
    {
        return Stream(_cursor + count, _size - count);
    }

    /**
     * size of the available buffer in stream.
     */
    [[nodiscard]] constexpr auto size() const noexcept -> std::size_t
    {
        return _size;
    }

    /**
     * underlying buffer.
     */
    [[nodiscard]] constexpr auto data() const noexcept -> const char*
    {
        return _cursor;
    }

    [[nodiscard]] constexpr auto as_string_view() const noexcept -> std::string_view
    {
        return std::string_view(_cursor, _size);
    }

    /**
     * first byte in the remainder of buffer.
     */
    [[nodiscard]] constexpr auto front() const noexcept -> char
    {
        return _cursor[0];
    }
};

class Result {
    static constexpr std::size_t valid_bit_offset = 63;
    static constexpr std::size_t size_mask = 0xFFFFFFFF;

    const char* _cursor = nullptr;
    std::size_t _size_and_bits = 0;

public:
    constexpr Result(Stream stream, bool is_valid) noexcept
        : _cursor(stream.data())
        , _size_and_bits(stream.size() | (static_cast<std::size_t>(is_valid) << valid_bit_offset))
    {
    }

    [[nodiscard]] constexpr auto cursor() const noexcept -> const char*
    {
        return _cursor;
    }

    [[nodiscard]] constexpr auto size() const noexcept -> std::size_t
    {
        return _size_and_bits & size_mask;
    }

    [[nodiscard]] constexpr auto stream() const noexcept -> Stream
    {
        return Stream(_cursor, _size_and_bits & size_mask);
    }

    [[nodiscard]] constexpr auto is_valid() const noexcept -> bool
    {
        return _size_and_bits & (1ull << valid_bit_offset);
    }

    [[nodiscard]] constexpr operator bool() const noexcept
    {
        return is_valid();
    }
};

template <typename T>
concept is_parser = requires(T instance) {
    {
        std::forward<T>(instance)(std::declval<Stream>())
    } -> std::convertible_to<Result>;
};

}  // namespace parsi

#endif  // PARSI_BASE_HPP
