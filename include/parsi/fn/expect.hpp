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
        if (!stream.starts_with(expected)) {
            return Result{stream, false};
        }

        return Result{stream.advanced(1), true};
    }
};

/**
 * A parser that expects the stream to start with a character that is in the given charset.
 */
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

/**
 * A parser that expects the stream to start with the given string.
 */
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
