#ifndef PARSI_FN_EXPECT_HPP
#define PARSI_FN_EXPECT_HPP

#include <string>

#include "parsi/base.hpp"
#include "parsi/fixed_string.hpp"

namespace parsi {

namespace fn {

/**
 * A parser that expects the stream to start with the given character.
 */
struct ExpectChar {
    char expected;
    bool negate = false;

    [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
    {
        const bool is_valid = (stream.size() > 0) & (negate ^ stream.starts_with(expected));
        stream.advance(is_valid);
        return Result{stream, is_valid};
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

        if (!charset.contains(stream.front())) {
            return Result{stream, false};
        }

        stream.advance(1);

        return Result{stream, true};
    }
};

template <std::size_t SizeV, typename CharT = const char>
struct ExpectFixedString {
    FixedString<SizeV, CharT> expected;

    [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
    {
        const auto expected_strview = expected.as_string_view();
        const bool starts_with = stream.starts_with(expected_strview);

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
        const bool starts_with = stream.starts_with(expected);

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
    -> fn::ExpectChar
{
    return fn::ExpectChar{expected};
}

/**
 * Creates a parser that expects the stream to start with the given character.
 */
[[nodiscard]] constexpr auto expect_not(char expected) noexcept
    -> fn::ExpectChar
{
    return fn::ExpectChar{ .expected = expected, .negate = true };
}

/**
 * Creates a parser that expects the stream to
 * start with a character the is in the given charset.
 */
[[nodiscard]] constexpr auto expect(Charset expected) noexcept
    -> fn::ExpectCharset
{
    return fn::ExpectCharset{expected};
}

/**
 * Creates a parser that expects the stream to
 * start with a character the is in the given charset.
 */
[[nodiscard]] constexpr auto expect_not(Charset expected) noexcept
    -> fn::ExpectCharset
{
    return fn::ExpectCharset{expected.opposite()};
}

}  // namespace parsi

#endif  // PARSI_FN_EXPECT_HPP
