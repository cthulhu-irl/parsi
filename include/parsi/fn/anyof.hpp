#ifndef PARSI_FN_ANYOF_HPP
#define PARSI_FN_ANYOF_HPP

#include <type_traits>
#include <utility>

#include "parsi/base.hpp"

namespace parsi::fn {

/**
 * A parser combinator where it tries the given `parsers`
 * on the given stream and at least one of them must succeed
 * which its result will be returned,
 * otherwise the result of the last one to fail will be returned.
 */
template <is_parser... Fs>
struct AnyOf {
    static_assert(sizeof...(Fs) >= 0);
};

template <is_parser F, is_parser... Fs>
struct AnyOf<F, Fs...> {
    std::remove_cvref_t<F> parser;
    AnyOf<std::remove_cvref_t<Fs>...> parsers;

    constexpr explicit AnyOf(std::remove_cvref_t<F> parser, std::remove_cvref_t<Fs>... parsers) noexcept
        : parser(std::move(parser))
        , parsers(std::move(parsers)...)
    {
    }

    [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
    {
        if (auto res = parser(stream); res) [[unlikely]] {
            return res;
        }
        return parsers(stream);
    }
};

template <>
struct AnyOf<> {
    [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
    {
        return Result{stream, false};
    }
};

}  // namespace parsi::fn

#endif  // PARSI_FN_ANYOF_HPP
