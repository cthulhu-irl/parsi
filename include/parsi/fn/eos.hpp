#ifndef PARSI_FN_EOS_HPP
#define PARSI_FN_EOS_HPP

#include "parsi/base.hpp"

namespace parsi::fn {

/**
 * A parser that expects the stream to have come to its end.
 */
struct Eos {
    [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
    {
        return Result{stream, stream.size() <= 0};
    };
};

}  // namespace parsi::fn

#endif  // PARSI_FN_EOS_HPP
