#ifndef PARSI_FN_EXTRACT_HPP
#define PARSI_FN_EXTRACT_HPP

#include "parsi/base.hpp"

namespace parsi {

namespace fn {

template <is_parser F, std::invocable<std::string_view> G>
struct Extract {
    std::remove_cvref_t<F> parser;
    std::remove_cvref_t<G> visitor;

    [[nodiscard]] constexpr auto operator()(Stream stream) const noexcept -> Result
    {
        auto result = parser(stream);
        if (result) {
            auto buffer = stream.buffer();
            auto start = stream.cursor();
            auto end = result.stream.cursor();

            auto subspan = buffer.subspan(start, end - start);
            auto substr = std::string_view(subspan.data(), subspan.size());

            if constexpr (requires {
                              {
                                  visitor(substr)
                              } -> std::same_as<bool>;
                          }) {
                if (!visitor(substr)) {
                    return Result{result.stream, false};
                }
            }
            else {
                visitor(substr);
            }
        }

        return result;
    }
};

}  // namespace fn

template <is_parser F, std::invocable<std::string_view> G>
[[nodiscard]] constexpr auto extract(F&& parser, G&& visitor)
{
    return fn::Extract<F, G>{std::forward<F>(parser), std::forward<G>(visitor)};
}

}  // namespace parsi

#endif  // PARSI_FN_EXTRACT_HPP
