#ifndef PARSI_FN_EOS_HPP
#define PARSI_FN_EOS_HPP

#include "parsi/base.hpp"

namespace parsi {

namespace fn {

/**
 * A parser that expects the stream to have come to its end.
 */
struct Eos {
    [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
    {
        if (stream.size() > 0) {
            return Result{stream, false};
        }

        return Result{stream, true};
    };
};

}  // namespace fn

/**
 * Creates a parser that expects the stream to have come to its end.
 */
[[nodiscard]] constexpr auto eos() noexcept -> fn::Eos
{
    return fn::Eos{};
}

}  // namespace parsi

#endif  // PARSI_FN_EOS_HPP
