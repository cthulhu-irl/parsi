#ifndef PARSI_PARSI_HPP
#define PARSI_PARSI_HPP

#include <concepts>
#include <type_traits>
#include <string_view>
#include <numeric>
#include <bitset>
#include <utility>
#include <optional>
#include <tuple>
#include <array>

namespace parsi {

struct Stream {
    // or std::span<char>
    std::string_view buffer;
    std::size_t cursor;

    constexpr Stream() noexcept
        : buffer()
        , cursor{0}
    {}

    constexpr Stream(const char* str) noexcept
        : Stream(std::string_view(str))
    {}

    constexpr Stream(std::string_view str) noexcept
        : buffer(str)
        , cursor(0)
    {}

    constexpr Stream(std::string_view str, std::size_t offset) noexcept
        : buffer(str)
        , cursor(offset)
    {}
};

struct Result {
    Stream stream;
    bool valid = false;

    constexpr operator bool() const noexcept { return valid; }
};

template <std::size_t N>
    requires (N > 0)
struct Bitset {
    using primary_type = std::size_t;

    constexpr static std::size_t k_cell_bitcount = sizeof(primary_type) * 8;
    constexpr static std::size_t k_array_size =
        (N < k_cell_bitcount) ? (N / k_cell_bitcount) : k_cell_bitcount;

    std::array<primary_type, k_array_size> bytes = {0};

    constexpr Bitset() noexcept {}

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
        } else {
            bytes[index / k_cell_bitcount] &= ~bit;
        }
    }
};

struct Charset {
    Bitset<256> map;

    constexpr explicit Charset(std::string_view charset) noexcept
    {
        for (char character : charset) {
            map.set(static_cast<unsigned char>(character), true);
        }
    }

    bool contains(char character) const noexcept
    {
        return map.test(character);
    }
};

template <typename T>
concept is_parser = requires(T instance) {
    { std::forward<T>(instance)(std::declval<Stream>()) } -> std::convertible_to<Result>;
};

// visitors/functors
namespace fn {

struct ExpectChar {
    char expected;

    constexpr Result operator()(Stream stream) const noexcept
    {
        if (stream.cursor >= stream.buffer.size()
                || stream.buffer[stream.cursor] != expected) {
            return Result{stream, false};
        }

        return Result{
            .stream = Stream(stream.buffer, stream.cursor + 1),
            .valid = true
        };
    }
};

struct ExpectCharset {
    Charset charset;

    constexpr Result operator()(Stream stream) const noexcept
    {
        if (stream.cursor >= stream.buffer.size()) {
            return Result{stream, false};
        }

        const char character = stream.buffer[stream.cursor];
        if (!charset.contains(character)) {
            return Result{stream, false};
        }

        return Result{
            .stream = Stream(stream.buffer, stream.cursor + 1),
            .valid = true
        };
    }
};

struct ExpectString {
    std::string expected;

    Result operator()(Stream stream) const noexcept
    {
        if (!stream.buffer.substr(stream.cursor).starts_with(expected)) {
            return Result{stream, false};
        }

        return Result{
            .stream = Stream(stream.buffer, stream.cursor + expected.size()),
            .valid = true
        };
    }
};

template <is_parser ...Fs>
struct Sequence {
    std::tuple<std::remove_cvref_t<Fs>...> parsers;

    constexpr explicit Sequence(std::remove_cvref_t<Fs> ...parsers) noexcept
        : parsers(std::move(parsers)...)
    {}

    constexpr Result operator()(Stream stream) const noexcept
    {
        return std::apply([stream]<typename ...Ts>(Ts&& ...parsers) {
            return parse<Ts...>(stream, std::forward<Ts>(parsers)...);
        }, parsers);
    }

private:
    template <typename First, typename ...Rest>
    static constexpr Result parse(Stream stream, First&& first, Rest&& ...rest) noexcept
    {
        auto result = first(stream);
        if (!result) {
            return result;
        }

        if constexpr (sizeof...(Rest) == 0) {
            return result;
        } else {
            return parse<Rest...>(result.stream, std::forward<Rest>(rest)...);
        }
    }
};

template <is_parser ...Fs>
struct AnyOf {
    std::tuple<std::remove_cvref_t<Fs>...> parsers;

    constexpr explicit AnyOf(std::remove_cvref_t<Fs> ...parsers) noexcept
        : parsers(std::move(parsers)...)
    {}

    constexpr Result operator()(Stream stream) const noexcept
    {
        return std::apply([stream]<typename ...Ts>(Ts&& ...parsers) {
            return parse<Ts...>(stream, std::forward<Ts>(parsers)...);
        }, parsers);
    }

private:
    template <typename First, typename ...Rest>
    static constexpr Result parse(Stream stream, First&& first, Rest&& ...rest) noexcept
    {
        auto result = first(stream);
        if (result) {
            return result;
        }

        if constexpr (sizeof...(Rest) == 0) {
            return Result{result.stream, false};
        } else {
            return parse<Rest...>(stream, std::forward<Rest>(rest)...);
        }
    }
};

template <is_parser F, std::invocable<std::string_view> G>
struct Visit {
    std::remove_cvref_t<F> parser;
    std::remove_cvref_t<G> visitor;

    constexpr Result operator()(Stream stream) const noexcept
    {
        auto result = parser(stream);
        if (result) {
            std::string_view buffer = stream.buffer;
            auto start = stream.cursor;
            auto end = result.stream.cursor;

            std::string_view substr = buffer.substr(start, end - start);

            if constexpr (requires { { visitor(substr) } -> std::same_as<bool>; }) {
                if (!visitor(substr)) {
                    return Result{result.stream, false};
                }
            } else {
                visitor(substr);
            }
        }

        return result;
    }
};

template <is_parser F>
struct Optional {
    std::remove_cvref_t<F> parser;

    constexpr Result operator()(Stream stream) const noexcept
    {
        auto result = parser(stream);
        if (!result) {
            return Result{stream, true};
        }

        return result;
    }
};

template <is_parser F,
          std::size_t Min = 0,
          std::size_t Max = std::numeric_limits<std::size_t>::max()>
struct Repeated {
    std::remove_cvref_t<F> parser;

    constexpr Result operator()(Stream stream) const noexcept
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

    constexpr Result operator()(Stream stream) const noexcept
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

template <is_parser F, is_parser G>
struct RepeatedWhile {
    std::remove_cvref_t<F> condition;
    std::remove_cvref_t<G> parser;

    constexpr Result operator()(Stream stream) const noexcept
    {
        auto res = Result{stream, true};
        while (true) {
            res = condition(stream);
            if (!res) {
                return Result{stream, true};
            }

            res = parser(res.stream);
            if (!res) {
                return Result{stream, true};
            }

            stream = res.stream;
        }

        return res;
    }
};

}  // namespace fn

namespace common {

constexpr auto charset_digit = parsi::Charset("0123456789");
constexpr auto charset_lower = parsi::Charset("abcdefghijklmnopqrstuvwxyz");
constexpr auto charset_upper = parsi::Charset("ABCDEFGHIJKLMNOPQRSTUVWXYZ");

}  // namespace common

inline auto expect(std::string expected)
{
    return fn::ExpectString{std::move(expected)};
}

constexpr auto expect(char expected)
{
    return fn::ExpectChar{expected};
}

constexpr auto expect(Charset expected)
{
    return fn::ExpectCharset{expected};
}

template <is_parser ...Fs>
constexpr auto sequence(Fs&& ...parsers)
{
    return fn::Sequence<Fs...>(std::forward<Fs>(parsers)...);
}

template <is_parser ...Fs>
constexpr auto anyof(Fs&& ...parsers)
{
    return fn::AnyOf<Fs...>(std::forward<Fs>(parsers)...);
}

template <is_parser F>
constexpr auto optional(F&& parser)
{
    return fn::Optional<F>{std::forward<F>(parser)};
}

template <std::size_t Min = 0, std::size_t Max = std::numeric_limits<std::size_t>::max(), is_parser F>
constexpr auto repeat(F&& parser)
{
    return fn::Repeated<F, Min, Max>{std::forward<F>(parser)};
}

template <is_parser F>
constexpr auto repeat(F&& parser, std::size_t count)
{
    return fn::RepeatedRanged<F>{std::forward<F>(parser), count, count};
}

template <is_parser F>
constexpr auto repeat(F&& parser, std::size_t min, std::size_t max)
{
    return fn::RepeatedRanged<F>{std::forward<F>(parser), min, max};
}

template <is_parser F, is_parser G>
constexpr auto repeat_while(F&& condition, G&& parser)
{
    return fn::RepeatedWhile<F, G>{std::forward<F>(condition), std::forward<G>(parser)};
}

template <is_parser F, std::invocable<std::string_view> G>
constexpr auto extract(F&& parser, G&& visitor)
{
    return fn::Visit<F, G>{std::forward<F>(parser), std::forward<G>(visitor)};
}

}  // namespace parsi

#endif  // PARSI_PARSI_HPP
