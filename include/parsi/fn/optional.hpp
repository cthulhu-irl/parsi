#ifndef PARSI_FN_OPTIONAL_HPP
#define PARSI_FN_OPTIONAL_HPP

#include <type_traits>
#include <utility>

#include "parsi/base.hpp"

namespace parsi::fn {

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
        if (!result) [[likely]] {
            return Result{stream, true};
        }
        return result;
    }
};

}  // namespace parsi::fn

#endif  // PARSI_FN_OPTIONAL_HPP
