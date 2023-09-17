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
        if constexpr (sizeof...(Fs) == 0) {
            return Result{stream, true};
        } else {
            return parse_rec<0>(stream);
        }
    }

private:
    template <std::size_t I>
        requires (I < sizeof...(Fs))
    [[nodiscard]] constexpr auto parse_rec(Stream stream) const noexcept -> Result
    {
        if constexpr (I == sizeof...(Fs)-1) {
            return std::get<I>(parsers)(stream);
        } else {
            auto res = std::get<I>(parsers)(stream);
            if (res) [[likely]] {
                return res;
            }
            return parse_rec<I+1>(res.stream);
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
