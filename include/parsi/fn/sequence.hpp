#ifndef PARSI_FN_SEQUENCE_HPP
#define PARSI_FN_SEQUENCE_HPP

#include <type_traits>
#include <utility>

#include "parsi/base.hpp"

namespace parsi::fn {

/**
 * Combines multiple parsers in consecutive order.
 * 
 * It starts by passing the incoming stream to the first parser,
 * and on success, its result stream to the second
 * and goes on up to the last parser.
 * If any of the parsers fail, it would return the failed result.
 */
template <is_parser... Fs>
struct Sequence {
    static_assert(sizeof...(Fs) >= 0);
};

template <is_parser F, is_parser... Fs>
struct Sequence<F, Fs...> {
    std::remove_cvref_t<F> parser;
    Sequence<std::remove_cvref_t<Fs>...> parsers;

    constexpr explicit Sequence(std::remove_cvref_t<F> parser, std::remove_cvref_t<Fs>... parsers) noexcept
        : parser(std::move(parser))
        , parsers(std::move(parsers)...)
    {
    }

    [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
    {
        auto res = parser(stream);
        if (!res) [[unlikely]] {
            return Result{stream, false};
        }
        return parsers(res.stream());
    }
};

template <>
struct Sequence<> {
    [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
    {
        return Result{stream, true};
    }
};

}  // namespace parsi::fn

#endif  // PARSI_FN_SEQUENCE_HPP
