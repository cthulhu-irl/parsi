#ifndef PARSI_FN_REPEATED_HPP
#define PARSI_FN_REPEATED_HPP

#include <limits>
#include <type_traits>
#include <utility>

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
    static_assert(Min <= Max, "Min cannot be greater than Max.");

    std::remove_cvref_t<F> parser;

    [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
    {
        if constexpr (Max == 0) {
            return Result{stream, true};
        }

        return apply_loop(parser, stream);
    }

private:
    /// the generic/general unspecialized loop. 
    [[nodiscard]] static constexpr auto apply_loop(const auto& parser, Stream stream) noexcept -> Result
    {
        std::size_t count = 0;

        while (const Result result = parser(stream)) {
            if constexpr (Min != 0 || Max != std::numeric_limits<std::size_t>::max()) {
                ++count;
            }

            if constexpr (Max != std::numeric_limits<std::size_t>::max()) {
                if (count > Max) [[unlikely]] {
                    return Result{stream, false};
                }
            }

            stream = result.stream();
        }

        if constexpr (Min != 0) {
            return Result{stream, Min <= count};
        } else {
            return Result{stream, true};
        }
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
        if (min > max) [[unlikely]] {
            return Result{stream, false};
        }

        std::size_t count = 0;

        while (const Result result = parser(stream)) {
            if (count > max) [[unlikely]] {
                break;
            }

            stream = result.stream();
        }

        if (count < min || max < count) [[unlikely]] {
            return Result{stream, false};
        }

        return Result{stream, true};
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
