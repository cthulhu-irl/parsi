#ifndef PARSI_INTERNAL_OPTIMIZER_HPP
#define PARSI_INTERNAL_OPTIMIZER_HPP

#include "parsi/base.hpp"
#include "parsi/fn/expect.hpp"
#include "parsi/fn/repeated.hpp"

namespace parsi::internal {

template <typename ParserT>
struct Optimizer {
    static constexpr auto optimize(const ParserT& parser) -> ParserT { return parser; }
    static constexpr auto optimize(ParserT&& parser) -> ParserT { return parser; }
};

template <fn::Negation NegationV>
struct Optimizer<fn::Repeated<fn::ExpectChar<NegationV>, 0, std::numeric_limits<std::size_t>::max()>> {
    struct RepeatedZeroToInfCharacter {
        char expected;

        constexpr auto operator()(Stream stream) const noexcept -> Result
        {
            while (stream.size() > 0 && stream.front() == expected) {
                stream.advance(1);
            }
            return Result{stream, !NegationV.negated};
        };
    };

    using parser_type = fn::Repeated<fn::ExpectChar<NegationV>, 0, std::numeric_limits<std::size_t>::max()>;

    static constexpr auto optimize(const parser_type& parser) -> RepeatedZeroToInfCharacter
    {
        return RepeatedZeroToInfCharacter{parser.parser.expected};
    }
};

template <std::size_t SetSizeV>
struct Optimizer<fn::Repeated<fn::ExpectCharRangeSet<SetSizeV>, 0, std::numeric_limits<std::size_t>::max()>> {
    struct RepeatedZeroToInfCharRangeSet {
        std::array<CharRange, SetSizeV> charset_ranges;

        constexpr auto operator()(Stream stream) const noexcept -> Result
        {
            const auto is_in_range = [this](char chr) {
                return [this, chr]<std::size_t ...Is>(std::index_sequence<Is...>) {
                    return (false || ... || (charset_ranges[Is].begin <= chr && chr <= charset_ranges[Is].end));
                }(std::make_index_sequence<SetSizeV>());
            };

            while (stream.size() > 0 && is_in_range(stream.front())) {
                stream.advance(1);
            }

            return Result{stream, true};
        };
    };

    using parser_type = fn::Repeated<fn::ExpectCharRangeSet<SetSizeV>, 0, std::numeric_limits<std::size_t>::max()>;

    static constexpr auto optimize(const parser_type& parser) -> RepeatedZeroToInfCharRangeSet
    {
        return RepeatedZeroToInfCharRangeSet{parser.parser.charset_ranges};
    }
};

template <>
struct Optimizer<fn::Repeated<fn::ExpectCharset, 0, std::numeric_limits<std::size_t>::max()>> {
    struct RepeatedZeroToInfCharset {
        Charset charset;

        constexpr auto operator()(Stream stream) const noexcept -> Result
        {
            while (stream.size() > 0 && charset.contains(stream.front())) {
                stream.advance(1);
            }
            return Result{stream, true};
        };
    };

    using parser_type = fn::Repeated<fn::ExpectCharset, 0, std::numeric_limits<std::size_t>::max()>;

    static constexpr auto optimize(const parser_type& parser) -> RepeatedZeroToInfCharset
    {
        return RepeatedZeroToInfCharset{parser.parser.charset};
    }
};

template <is_parser ParserT>
constexpr auto optimize(ParserT&& parser)
{
    using type = std::remove_cvref_t<ParserT>;
    return Optimizer<ParserT>::optimize(std::forward<ParserT>(parser));
}

}  // namespace parsi::internal

#endif  // PARSI_INTERNAL_OPTIMIZER_HPP
