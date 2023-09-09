#ifndef PARSI_FN_OPTIONAL_HPP
#define PARSI_FN_OPTIONAL_HPP

#include "parsi/base.hpp"

namespace parsi {

namespace fn {

/**
 * Makes a parser to be optional and resort back to original stream
 * if the given parser fails to parse,
 * otherwise it will return the succeeded parse result.
 * 
 * This parser combinator will always succeed and never fail.
 */
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

}  // namespace fn

/**
 * Creates an optional parser out of given `parser`
 * that will return a valid succeeded result with
 * the original stream if the `parser` fails.
 * 
 * @see fn::Optional
 */
template <is_parser F>
[[nodiscard]] constexpr auto optional(F&& parser) noexcept
    -> fn::Optional<std::remove_cvref_t<F>>
{
    return fn::Optional<std::remove_cvref_t<F>>{std::forward<F>(parser)};
}

}  // namespace parsi

#endif  // PARSI_FN_OPTIONAL_HPP
