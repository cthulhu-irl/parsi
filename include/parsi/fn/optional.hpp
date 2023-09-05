#ifndef PARSI_FN_OPTIONAL_HPP
#define PARSI_FN_OPTIONAL_HPP

#include "parsi/base.hpp"

namespace parsi {

namespace fn {

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

template <is_parser F>
[[nodiscard]] constexpr auto optional(F&& parser)
{
    return fn::Optional<F>{std::forward<F>(parser)};
}

}  // namespace parsi

#endif  // PARSI_FN_OPTIONAL_HPP
