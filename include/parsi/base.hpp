#ifndef PARSI_BASE_HPP
#define PARSI_BASE_HPP

#include <concepts>
#include <span>
#include <string_view>
#include <utility>

namespace parsi {

class Stream {
public:
    using buffer_type = std::span<const char>;

private:
    buffer_type _buffer{};
    std::size_t _cursor = 0;

public:
    constexpr Stream(const char* str) noexcept : Stream(str, std::strlen(str))
    {
    }

    constexpr Stream(const char* str, const std::size_t size) noexcept
        : _buffer(str, size), _cursor(0)
    {
    }

    constexpr Stream(std::string_view str) noexcept : _buffer(str.data(), str.size()), _cursor(0)
    {
    }

    constexpr Stream(buffer_type buffer) noexcept : _buffer(buffer), _cursor(0)
    {
    }

    [[nodicard]] constexpr auto advanced(std::size_t count) -> Stream
    {
        auto ret = Stream(_buffer);
        ret._cursor = _cursor + count;
        return ret;
    }

    [[nodicard]] constexpr auto starts_with(const char character) const noexcept -> bool
    {
        if (_cursor >= _buffer.size()) [[unlikely]] {
            return false;
        }

        return character == _buffer[_cursor];
    }

    [[nodicard]] constexpr auto starts_with(std::span<const char> span) const noexcept -> bool
    {
        if (size() < span.size()) {
            return false;
        }

        for (std::size_t idx = 0; idx < span.size(); ++idx) {
            if (_buffer[_cursor + idx] != span[idx]) [[unlikely]] {
                return false;
            }
        }

        return true;
    }

    [[nodiscard]] constexpr auto size() const noexcept -> std::size_t
    {
        return _buffer.size() - _cursor;
    }

    [[nodiscard]] constexpr auto buffer_size() const noexcept -> std::size_t
    {
        return _buffer.size();
    }

    [[nodiscard]] constexpr auto cursor() const noexcept -> std::size_t
    {
        return _cursor;
    }

    [[nodiscard]] constexpr auto buffer() const noexcept -> buffer_type
    {
        return _buffer;
    }

    [[nodiscard]] constexpr auto at(const std::size_t index) const noexcept -> const char
    {
        return _buffer[_cursor + index];
    }

    [[nodiscard]] constexpr auto front() const noexcept -> const char
    {
        return _buffer[_cursor];
    }
};

struct Result {
    Stream stream;
    bool valid = false;

    [[nodiscard]] constexpr operator bool() const noexcept
    {
        return valid;
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
