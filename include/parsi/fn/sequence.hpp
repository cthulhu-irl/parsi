#ifndef PARSI_FN_SEQUENCE_HPP
#define PARSI_FN_SEQUENCE_HPP

#include "parsi/base.hpp"

namespace parsi {

namespace fn {

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
    std::tuple<std::remove_cvref_t<Fs>...> parsers;

    constexpr explicit Sequence(std::remove_cvref_t<Fs>... parsers) noexcept
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
            if (!res) {
                return res;
            }
            return parse_rec<I+1>(res.stream);
        }
    }
};

// template <is_parser F>
// struct Sequence<F> {
//     std::remove_cvref_t<F> parser;

//     constexpr explicit Sequence(std::remove_cvref_t<F> parser) noexcept
//         : parser(std::move(parser))
//     {
//     }

//     [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
//     {
//         return parser(stream);
//     }
// };

// template <is_parser F, is_parser G>
// struct Sequence<F, G> {
//     std::remove_cvref_t<F> parser_f;
//     std::remove_cvref_t<G> parser_g;

//     constexpr explicit Sequence(std::remove_cvref_t<F> parser_1, std::remove_cvref_t<G> parser_2) noexcept
//         : parser_f(std::move(parser_1))
//         , parser_g(std::move(parser_2))
//     {
//     }

//     [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
//     {
//         auto res = parser_f(stream);
//         if (!res) {
//             return res;
//         }
//         return parser_g(res.stream);
//     }
// };

}  // namespace fn

/**
 * Creates an instance of fn::Sequence;
 * a combinator to combine multiple parsers
 * to parse a stream sequentially and consecutively.
 * 
 * @see parsi::fn::Sequence
 */
template <is_parser... Fs>
[[nodiscard]] constexpr auto sequence(Fs&&... parsers) noexcept
    -> fn::Sequence<std::remove_cvref_t<Fs>...>
{
    return fn::Sequence<std::remove_cvref_t<Fs>...>(std::forward<Fs>(parsers)...);
}

}  // namespace parsi

#endif  // PARSI_FN_SEQUENCE_HPP
