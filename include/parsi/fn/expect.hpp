#ifndef PARSI_FN_EXPECT_HPP
#define PARSI_FN_EXPECT_HPP

#include "parsi/base.hpp"

namespace parsi {

namespace fn {

/**
 * A parser that expects the stream to start with the given character.
 */
struct ExpectChar {
    char expected;

    [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
    {
        const bool starts_with = stream.starts_with(expected);

        return Result{stream.advanced(starts_with), starts_with};
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

        const bool contains = charset.contains(stream.front());

        return Result{stream.advanced(contains), contains};
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
 * Creates a parser that expects the stream to
 * start with a character the is in the given charset.
 */
[[nodiscard]] constexpr auto expect(Charset expected) noexcept
    -> fn::ExpectCharset
{
    return fn::ExpectCharset{expected};
}

}  // namespace parsi

#endif  // PARSI_FN_EXPECT_HPP
