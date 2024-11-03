#ifndef PARSI_FN_EXPECT_HPP
#define PARSI_FN_EXPECT_HPP

#include <string>

#include "parsi/base.hpp"
#include "parsi/charset.hpp"
#include "parsi/fixed_string.hpp"

namespace parsi {

namespace fn {

struct Negation {
    bool negated = false;
};

/**
 * A parser that expects the stream to start with the given character.
 */
template <Negation NegationV = Negation{.negated = false}>
struct ExpectChar {
    char expected;

    [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
    {
        if (stream.size() <= 0) [[unlikely]] {
            return Result{stream, false};
        }
        const bool is_valid = stream.as_string_view().starts_with(expected);
        stream.advance(1);
        if constexpr (NegationV.negated) {
            return Result{stream, !is_valid};
        } else {
            return Result{stream, is_valid};
        }
    }
};

/**
 * A parser that expects the stream to start with a character that is in the given charset.
 */
struct ExpectCharset {
    Charset charset;

    [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
    {
        if (stream.size() <= 0) [[unlikely]] {
            return Result{stream, false};
        }
        const bool is_valid = charset.contains(stream.front());
        stream.advance(1);
        return Result{stream, is_valid};
    }
};

/**
 * A parser that expects the stream to start with a character
 * that is in one of the given character ranges.
 */
template <std::size_t SizeV>
struct ExpectCharRangeSet {
    std::array<CharRange, SizeV> charset_ranges;

    [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
    {
        if (stream.size() <= 0) [[unlikely]] {
            return Result{stream, false};
        }

        const bool is_valid = check_against_ranges(stream.front(), std::make_index_sequence<SizeV>());
        stream.advance(1);
        return Result{stream, is_valid};
    }

private:
    template <std::size_t ...Is>
    [[nodiscard]] constexpr auto check_against_ranges(char chr, std::index_sequence<Is...>) const noexcept -> bool
    {
        return (false || ... || (charset_ranges[Is].begin <= chr && chr <= charset_ranges[Is].end));
    }
};

template <std::size_t SizeV, typename CharT = const char>
struct ExpectFixedString {
    FixedString<SizeV, CharT> expected;

    [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
    {
        const auto expected_strview = expected.as_string_view();
        const bool starts_with = stream.as_string_view().starts_with(expected_strview);
        return Result{stream.advanced(starts_with * expected_strview.size()), starts_with};
    }
};

/**
 * A parser that expects the stream to start with the given string.
 */
struct ExpectString {
    std::string expected;

    [[nodiscard]] auto operator()(Stream stream) const noexcept -> Result
    {
        const bool starts_with = stream.as_string_view().starts_with(expected);
        return Result{stream.advanced(starts_with * expected.size()), starts_with};
    }
};

}  // namespace fn

/**
 * Creates a parser that expects the stream to start with the given fixed string.
 * Parser's expected string is fixed and cannot be changed. better performance for string literals.
 */
template <std::size_t SizeV>
[[nodiscard]] constexpr auto expect(const char (&str)[SizeV]) noexcept
    -> fn::ExpectFixedString<SizeV, const char>
{
    return fn::ExpectFixedString<SizeV, const char>{FixedString<SizeV, const char>::make(str, SizeV).value()};
}

/**
 * Creates a parser that expects the stream to start with the given fixed string.
 * Parser's expected string is fixed and cannot be changed. better performance for string literals.
 */
template <std::size_t SizeV, typename CharT = const char>
[[nodiscard]] constexpr auto expect(FixedString<SizeV, CharT> expected) noexcept
    -> fn::ExpectFixedString<SizeV, CharT>
{
    return fn::ExpectFixedString<SizeV, CharT>{expected};
}

/**
 * Creates a parser that expects the stream to start with the given string.
 */
[[nodiscard]] inline auto expect(std::string expected) noexcept
    -> fn::ExpectString
{
    return fn::ExpectString{std::move(expected)};
}

/**
 * Creates a parser that expects the stream to start with the given character.
 */
[[nodiscard]] constexpr auto expect(char expected) noexcept
    -> fn::ExpectChar<>
{
    return fn::ExpectChar<>{expected};
}

/**
 * Creates a parser that expects the stream to start with the given character.
 */
[[nodiscard]] constexpr auto expect_not(char expected) noexcept
    -> fn::ExpectChar<fn::Negation{.negated = true}>
{
    return fn::ExpectChar<fn::Negation{.negated = true}>{expected};
}

/**
 * Creates a parser that expects the stream to
 * start with a character that is in the given charset.
 */
[[nodiscard]] constexpr auto expect(Charset expected) noexcept
    -> fn::ExpectCharset
{
    return fn::ExpectCharset{expected};
}

/**
 * Creates a parser that expects the stream to
 * start with a character that is in the given charset.
 */
[[nodiscard]] constexpr auto expect_not(Charset expected) noexcept
    -> fn::ExpectCharset
{
    return fn::ExpectCharset{expected.opposite()};
}

/**
 * Creates a parser that expects the stream to
 * start with a character that is in one the given char ranges.
 */
template <std::same_as<CharRange> ...Ts>
[[nodiscard]] constexpr auto expect(CharRange first, Ts ...rest) noexcept
    -> fn::ExpectCharRangeSet<1 + sizeof...(Ts)>
{
    return fn::ExpectCharRangeSet<1 + sizeof...(Ts)>{.charset_ranges = {first, rest...}};
}

}  // namespace parsi

#endif  // PARSI_FN_EXPECT_HPP
