#ifndef PARSI_FN_EXPECT_HPP
#define PARSI_FN_EXPECT_HPP

#include "parsi/base.hpp"

namespace parsi {

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

}  // namespace parsi

#endif  // PARSI_FN_EXPECT_HPP
