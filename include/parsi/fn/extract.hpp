#ifndef PARSI_FN_EXTRACT_HPP
#define PARSI_FN_EXTRACT_HPP

#include <concepts>
#include <type_traits>
#include <utility>

#include "parsi/base.hpp"

namespace parsi {

namespace fn {

/**
 * Visits the sub portion of the stream
 * that was successfully parsed via `parser`
 * by passing the subspan/substr to the `visitor`,
 * and if the parser fails, the `visitor` won't be called.
 * 
 * The `visitor` can optionally return a boolean to indicate
 * success or failure of the parser.
 */
template <is_parser F, std::invocable<std::string_view> G>
struct Extract {
    std::remove_cvref_t<F> parser;
    std::remove_cvref_t<G> visitor;

    [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
    {
        auto result = parser(stream);
        if (result) {
            const auto start = stream.data();
            const auto end = result.stream().data();

            const std::string_view substr = stream.as_string_view().substr(0, end - start);

            if constexpr (requires { { visitor(substr) } -> std::same_as<bool>; }) {
                if (!visitor(substr)) {
                    return Result{result.stream(), false};
                }
            }
            else {
                visitor(substr);
            }
        }

        return result;
    }
};

}  // namespace fn

/**
 * Creates a `parser` that extracts (non-owning) the portion
 * that was successfully parsed with the given `parser`,
 * and passes the subspan portion to the given `visitor`.
 * 
 * @see fn::Extract
 */
template <is_parser F, std::invocable<std::string_view> G>
[[nodiscard]] constexpr auto extract(F&& parser, G&& visitor) noexcept
    -> fn::Extract<std::remove_cvref_t<F>, std::remove_cvref_t<G>>
{
    return fn::Extract<std::remove_cvref_t<F>, std::remove_cvref_t<G>>{
        std::forward<F>(parser),
        std::forward<G>(visitor)
    };
}

}  // namespace parsi

#endif  // PARSI_FN_EXTRACT_HPP
