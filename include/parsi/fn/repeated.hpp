#ifndef PARSI_FN_REPEATED_HPP
#define PARSI_FN_REPEATED_HPP

#include "parsi/base.hpp"

namespace parsi {

namespace fn {

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

template <std::size_t Min = 0, std::size_t Max = std::numeric_limits<std::size_t>::max(),
          is_parser F>
[[nodiscard]] constexpr auto repeat(F&& parser)
{
    return fn::Repeated<F, Min, Max>{std::forward<F>(parser)};
}

template <is_parser F>
[[nodiscard]] constexpr auto repeat(F&& parser, std::size_t count)
{
    return fn::RepeatedRanged<F>{std::forward<F>(parser), count, count};
}

template <is_parser F>
[[nodiscard]] constexpr auto repeat(F&& parser, std::size_t min, std::size_t max)
{
    return fn::RepeatedRanged<F>{std::forward<F>(parser), min, max};
}

}  // namespace parsi

#endif  // PARSI_FN_REPEATED_HPP
