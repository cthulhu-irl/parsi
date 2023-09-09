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
        if (!result) {
            return result;
        }

        if constexpr (sizeof...(Rest) == 0) {
            return result;
        }
        else {
            return parse<Rest...>(result.stream, std::forward<Rest>(rest)...);
        }
    }
};

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
