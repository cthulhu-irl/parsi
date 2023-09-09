#ifndef PARSI_FN_ANYOF_HPP
#define PARSI_FN_ANYOF_HPP

#include "parsi/base.hpp"

namespace parsi {

namespace fn {

/**
 * A parser combinator where it tries the given `parsers`
 * on the given stream and at least one of them must succeed
 * which its result will be returned,
 * otherwise the result of the last one to fail will be returned.
 */
template <is_parser... Fs>
struct AnyOf {
    std::tuple<std::remove_cvref_t<Fs>...> parsers;

    constexpr explicit AnyOf(std::remove_cvref_t<Fs>... parsers) noexcept
        : parsers(std::move(parsers)...)
    {
    }

    [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
    {
        return std::apply(
            [stream]<typename... Ts>(Ts&&... parsers) {
                return parse<Ts...>(stream, std::forward<Ts>(parsers)...);
            },
            parsers);
    }

private:
    template <typename First, typename... Rest>
    [[nodiscard]] static constexpr auto parse(Stream stream, First&& first, Rest&&... rest) noexcept
        -> Result
    {
        auto result = first(stream);
        if (result) {
            return result;
        }

        if constexpr (sizeof...(Rest) == 0) {
            return Result{result.stream, false};
        }
        else {
            return parse<Rest...>(stream, std::forward<Rest>(rest)...);
        }
    }
};

}  // namespace fn

/**
 * Creates a parser by combining the given `parsers`
 * where the result of only the one that succeeds
 * or the result of last one that fails will be returned.
 * 
 * @see fn::AnyOf
 */
template <is_parser... Fs>
[[nodiscard]] constexpr auto anyof(Fs&&... parsers) noexcept
    -> fn::AnyOf<std::remove_cvref_t<Fs>...>
{
    return fn::AnyOf<std::remove_cvref_t<Fs>...>(std::forward<Fs>(parsers)...);
}

}  // namespace parsi

#endif  // PARSI_FN_ANYOF_HPP
