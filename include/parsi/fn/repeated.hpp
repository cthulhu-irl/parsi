#ifndef PARSI_FN_REPEATED_HPP
#define PARSI_FN_REPEATED_HPP

#include "parsi/base.hpp"

namespace parsi {

namespace fn {

/**
 * A parser combinator that repeats the parser on stream
 * for a `Min` up to `Max` times consecutively.
 * 
 * The parser must succeed to consecutively parse the stream
 * for at least `Min` times and at most `Max` times,
 * otherwise it will result in failure.
 * 
 * `Min` and `Max` are compile-time for optimization purposes.
 */
template <is_parser F, std::size_t Min = 0,
          std::size_t Max = std::numeric_limits<std::size_t>::max()>
struct Repeated {
    std::remove_cvref_t<F> parser;

    [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
    {
        if constexpr (Max == 0) {
            return Result{stream, true};
        }

        Result last{stream, false};
        std::size_t count = 0;

        Result result = parser(stream);
        while (result.valid) {
            ++count;
            last = result;

            if (Max < count) {
                break;
            }

            result = parser(result.stream);
        }

        if (count < Min || Max < count) {
            return Result{last.stream, false};
        }

        return Result{last.stream, true};
    }
};

/**
 * A parser combinator that repeats the parser on stream
 * for a range of miniumum and maximum count consecutively.
 * 
 * The parser must succeed to consecutively parse the stream
 * for at least `min` times and at most `max` times,
 * otherwise it will fail.
 */
template <is_parser F>
struct RepeatedRanged {
    std::remove_cvref_t<F> parser;
    std::size_t min = 0;
    std::size_t max = std::numeric_limits<std::size_t>::max();

    [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
    {
        if (min > max) {
            return Result{stream, false};
        }

        Result last;
        std::size_t count = 0;

        auto result = parser(stream);
        while (result.valid) {
            last = result;

            if (count > max) {
                break;
            }

            result = parser(stream);
        }

        if (count < min || max < count) {
            return Result{last.stream, false};
        }

        return Result{last.stream, true};
    }
};

}  // namespace fn

/**
 * Creates a repeated parser combinator,
 * a combinator which combines the given parser repeatedly
 * for a given minimum and maximum times.
 * 
 * By default, the range is from 0 to almost infinite,
 * expecting the parser to be repeated from none at all to any amount of times.
 * 
 * @see fn::Repeated
 */
template <std::size_t Min = 0, std::size_t Max = std::numeric_limits<std::size_t>::max(),
          is_parser F>
[[nodiscard]] constexpr auto repeat(F&& parser) noexcept
    -> fn::Repeated<std::remove_cvref_t<F>, Min, Max>
{
    return fn::Repeated<std::remove_cvref_t<F>, Min, Max>{std::forward<F>(parser)};
}

/**
 * Creates a parser that repeats the given `parser`
 * with itself consecutively for exactly `count` times.
 * 
 * @see fn::RepeatedRanged
 */
template <is_parser F>
[[nodiscard]] constexpr auto repeat(F&& parser, std::size_t count) noexcept
    -> fn::RepeatedRanged<std::remove_cvref_t<F>>
{
    return fn::RepeatedRanged<std::remove_cvref_t<F>>{std::forward<F>(parser), count, count};
}

/**
 * Creates a parser that repeats the given `parser`
 * with itself consecutively for at least `min` times and maximum `max` times.
 * 
 * If the parser can successfully parse the stream consecutively
 * within `min` and `max` range, then the parsing will be valid,
 * otherwise parsing will fail.
 * 
 * @see fn::RepeatedRanged
 */
template <is_parser F>
[[nodiscard]] constexpr auto repeat(F&& parser, std::size_t min, std::size_t max) noexcept
    -> fn::RepeatedRanged<std::remove_cvref_t<F>>
{
    return fn::RepeatedRanged<std::remove_cvref_t<F>>{std::forward<F>(parser), min, max};
}

}  // namespace parsi

#endif  // PARSI_FN_REPEATED_HPP
