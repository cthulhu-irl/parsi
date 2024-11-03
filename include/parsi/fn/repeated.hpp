#ifndef PARSI_FN_REPEATED_HPP
#define PARSI_FN_REPEATED_HPP

#include <limits>
#include <type_traits>
#include <utility>

#include "parsi/base.hpp"

namespace parsi::fn {

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
        if (Max < Min || Max == 0) {
            return Result{stream, true};
        }

        std::size_t count = 0;

        for (; count < Min; ++count) {
            const Result result = parser(stream);
            if (!result) [[unlikely]] {
                return result;
            }
            stream = result.stream();
        }

        for (; count <= Max; ++count) {
            const Result result = parser(stream);
            if (!result) [[unlikely]] {
                return Result{stream, true};
            }
            stream = result.stream();
        }

        return Result{stream, false};
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

}  // namespace parsi::fn

#endif  // PARSI_FN_REPEATED_HPP
