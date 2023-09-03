#ifndef PARSI_PARSI_HPP
#define PARSI_PARSI_HPP

#include <array>
#include <bitset>
#include <concepts>
#include <numeric>
#include <span>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace parsi {

namespace internal {

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

}  // namespace internal

class Stream {
public:
    using buffer_type = std::span<const char>;

private:
    buffer_type _buffer{};
    std::size_t _cursor = 0;

public:
    constexpr Stream(const char* str) noexcept
        : Stream(str, std::strlen(str))
    {
    }

    constexpr Stream(const char* str, const std::size_t size) noexcept
        : _buffer(str, size)
        , _cursor(0)
    {
    }

    constexpr Stream(std::string_view str) noexcept
        : _buffer(str.data(), str.size())
        , _cursor(0)
    {
    }

    constexpr Stream(buffer_type buffer) noexcept
        : _buffer(buffer)
        , _cursor(0)
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

template <typename T>
concept is_parser = requires(T instance) {
    {
        std::forward<T>(instance)(std::declval<Stream>())
    } -> std::convertible_to<Result>;
};

// visitors
namespace fn {

struct ExpectChar {
    char expected;

    [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
    {
        if (!stream.starts_with(expected)) {
            return Result{stream, false};
        }

        return Result{stream.advanced(1), true};
    }
};

struct ExpectCharset {
    Charset charset;

    [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
    {
        if (stream.size() <= 0) {
            return Result{stream, false};
        }

        const char character = stream.front();
        if (!charset.contains(character)) {
            return Result{stream, false};
        }

        return Result{stream.advanced(1), true};
    }
};

struct ExpectString {
    std::string expected;

    [[nodiscard]] auto operator()(Stream stream) const noexcept -> Result
    {
        if (!stream.starts_with(expected)) {
            return Result{stream, false};
        }

        return Result{stream.advanced(expected.size()), true};
    }
};

template <is_parser... Fs>
struct Sequence {
    std::tuple<std::remove_cvref_t<Fs>...> parsers;

    constexpr explicit Sequence(std::remove_cvref_t<Fs>... parsers) noexcept
        : parsers(std::move(parsers)...)
    {
    }

    [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
    {
        return std::apply(
            [stream]<typename... Ts>(Ts&&... parsers) {
                return parse<Ts...>(stream, std::forward<Ts>(parsers)...);
            },
            parsers);
    }

private:
    template <typename First, typename... Rest>
    [[nodiscard]] static constexpr auto parse(Stream stream, First&& first, Rest&&... rest) noexcept
        -> Result
    {
        auto result = first(stream);
        if (!result) {
            return result;
        }

        if constexpr (sizeof...(Rest) == 0) {
            return result;
        }
        else {
            return parse<Rest...>(result.stream, std::forward<Rest>(rest)...);
        }
    }
};

template <is_parser... Fs>
struct AnyOf {
    std::tuple<std::remove_cvref_t<Fs>...> parsers;

    constexpr explicit AnyOf(std::remove_cvref_t<Fs>... parsers) noexcept
        : parsers(std::move(parsers)...)
    {
    }

    [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
    {
        return std::apply(
            [stream]<typename... Ts>(Ts&&... parsers) {
                return parse<Ts...>(stream, std::forward<Ts>(parsers)...);
            },
            parsers);
    }

private:
    template <typename First, typename... Rest>
    [[nodiscard]] static constexpr auto parse(Stream stream, First&& first, Rest&&... rest) noexcept
        -> Result
    {
        auto result = first(stream);
        if (result) {
            return result;
        }

        if constexpr (sizeof...(Rest) == 0) {
            return Result{result.stream, false};
        }
        else {
            return parse<Rest...>(stream, std::forward<Rest>(rest)...);
        }
    }
};

template <is_parser F, std::invocable<std::string_view> G>
struct Visit {
    std::remove_cvref_t<F> parser;
    std::remove_cvref_t<G> visitor;

    [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
    {
        auto result = parser(stream);
        if (result) {
            auto buffer = stream.buffer();
            auto start = stream.cursor();
            auto end = result.stream.cursor();

            auto subspan = buffer.subspan(start, end - start);
            auto substr = std::string_view(subspan.data(), subspan.size());

            if constexpr (requires {
                              {
                                  visitor(substr)
                              } -> std::same_as<bool>;
                          }) {
                if (!visitor(substr)) {
                    return Result{result.stream, false};
                }
            }
            else {
                visitor(substr);
            }
        }

        return result;
    }
};

template <is_parser F>
struct Optional {
    std::remove_cvref_t<F> parser;

    [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
    {
        auto result = parser(stream);
        if (!result) {
            return Result{stream, true};
        }

        return result;
    }
};

template <is_parser F, std::size_t Min = 0,
          std::size_t Max = std::numeric_limits<std::size_t>::max()>
struct Repeated {
    std::remove_cvref_t<F> parser;

    [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
    {
        if constexpr (Max == 0) {
            return Result{stream, true};
        }

        Result last{stream, false};
        std::size_t count = 0;

        Result result = parser(stream);
        while (result.valid) {
            ++count;
            last = result;

            if (Max < count) {
                break;
            }

            result = parser(result.stream);
        }

        if (count < Min || Max < count) {
            return Result{last.stream, false};
        }

        return Result{last.stream, true};
    }
};

template <is_parser F>
struct RepeatedRanged {
    std::remove_cvref_t<F> parser;
    std::size_t min = 0;
    std::size_t max = std::numeric_limits<std::size_t>::max();

    [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
    {
        if (min > max) {
            return Result{stream, false};
        }

        Result last;
        std::size_t count = 0;

        auto result = parser(stream);
        while (result.valid) {
            last = result;

            if (count > max) {
                break;
            }

            result = parser(stream);
        }

        if (count < min || max < count) {
            return Result{last.stream, false};
        }

        return Result{last.stream, true};
    }
};

}  // namespace fn

[[nodiscard]] inline auto expect(std::string expected)
{
    return fn::ExpectString{std::move(expected)};
}

[[nodiscard]] constexpr auto expect(char expected) noexcept
{
    return fn::ExpectChar{expected};
}

[[nodiscard]] constexpr auto expect(Charset expected) noexcept
{
    return fn::ExpectCharset{expected};
}

[[nodiscard]] constexpr auto eos() noexcept
{
    return [](Stream stream) -> Result {
        if (stream.size() > 0) {
            return Result{stream, false};
        }

        return Result{stream, true};
    };
}

template <is_parser... Fs>
[[nodiscard]] constexpr auto sequence(Fs&&... parsers)
{
    return fn::Sequence<Fs...>(std::forward<Fs>(parsers)...);
}

template <is_parser... Fs>
constexpr auto anyof(Fs&&... parsers)
{
    return fn::AnyOf<Fs...>(std::forward<Fs>(parsers)...);
}

template <is_parser F>
[[nodiscard]] constexpr auto optional(F&& parser)
{
    return fn::Optional<F>{std::forward<F>(parser)};
}

template <std::size_t Min = 0, std::size_t Max = std::numeric_limits<std::size_t>::max(),
          is_parser F>
[[nodiscard]] constexpr auto repeat(F&& parser)
{
    return fn::Repeated<F, Min, Max>{std::forward<F>(parser)};
}

template <is_parser F>
[[nodiscard]] constexpr auto repeat(F&& parser, std::size_t count)
{
    return fn::RepeatedRanged<F>{std::forward<F>(parser), count, count};
}

template <is_parser F>
[[nodiscard]] constexpr auto repeat(F&& parser, std::size_t min, std::size_t max)
{
    return fn::RepeatedRanged<F>{std::forward<F>(parser), min, max};
}

template <is_parser F, std::invocable<std::string_view> G>
[[nodiscard]] constexpr auto extract(F&& parser, G&& visitor)
{
    return fn::Visit<F, G>{std::forward<F>(parser), std::forward<G>(visitor)};
}

}  // namespace parsi

#endif  // PARSI_PARSI_HPP
