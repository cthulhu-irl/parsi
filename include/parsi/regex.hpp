#ifndef PARSI_REGEX_HPP
#define PARSI_REGEX_HPP

#include "parsi/base.hpp"
#include "parsi/charset.hpp"
#include "parsi/fn/expect.hpp"
#include "parsi/fn/anyof.hpp"
#include "parsi/fn/extract.hpp"
#include "parsi/fn/optional.hpp"
#include "parsi/fn/sequence.hpp"
#include "parsi/fn/repeated.hpp"
#include "parsi/fn/eos.hpp"
#include "parsi/internal/fixedstring.hpp"

namespace parsi {

namespace internal {

constexpr auto charset_digit = parsi::Charset("0123456789");
constexpr auto charset_lower = parsi::Charset("abcdefghijklmnopqrstuvwxyz");
constexpr auto charset_upper = parsi::Charset("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
constexpr auto charset_hex = parsi::Charset("0123456789abcdefABCDEF");
constexpr auto charset_escapes_in_group = parsi::Charset(".+*?^$()[]{}|\\");
constexpr auto charset_escapes_in_bracket = parsi::Charset("TODO"); // TODO
constexpr auto charset_normal = charset_digit + charset_lower + charset_upper + parsi::Charset("...");  // TODO

//--- tags

struct Quantification {
    std::size_t minimum = 0;
    std::size_t maximum = std::numeric_limits<std::size_t>::max();
    bool optional = false;
};

struct regex_eos_tag{};
struct regex_or_tag{};
struct regex_group_start_tag{};
struct regex_group_end_tag{};
struct regex_backreference_tag{ unsigned char num; };
struct regex_quantifier_tag{ Quantification counts; };
struct regex_charset_tag { parsi::Charset charset; };
struct regex_char_tag { char character; };
struct regex_unknown_tag{};

//--- regex combinator/accumulator and state

template <typename ParserT>
struct RegexCreatorState {
    ParserT parser;
    bool is_valid = true;

    constexpr bool() const noexcept { return is_valid; }
};

struct RegexCreatorWriter {
    template <typename StateT>
    constexpr auto operator()(StateT state, regex_char_tag) {}

    template <typename StateT>
    constexpr auto operator()(StateT state, regex_charset_tag) {}

    template <typename StateT>
    constexpr auto operator()(StateT state, regex_or_tag) {}

    template <typename StateT>
    constexpr auto operator()(StateT state, regex_group_start_tag) {}

    template <typename StateT>
    constexpr auto operator()(StateT state, regex_group_end_tag) {}

    template <typename StateT>
    constexpr auto operator()(StateT state, regex_eos_tag) {}

    template <typename StateT>
    constexpr auto operator()(StateT state, regex_unknown_tag) {}
};

//--- regex parser

struct ExpectNegatableCharset {
    bool negated = false;
    parsi::Charset charset;

    constexpr auto operator()(Stream stream) const noexcept -> Result
    {
        if (stream.size() <= 0) {
            return Result{stream, false};
        }
        const bool is_valid = charset.contains(stream.front()) ^ negated;
        return Result{stream.advanced(is_valid), is_valid};
    }
};

template <typename T>
struct CompiledResult {
    std::optional<T> value;
    Stream stream;
};

struct RegexExactMatcher {
    static constexpr auto parse(Stream stream) -> CompiledResult<parsi::fn::ExpectChar>
    {
        constexpr auto parser_plain_characters = parsi::expect(
           charset_digit + charset_lower + charset_upper /* + TODO */
        );
        return { .value = std::nullopt, .stream = stream };
    }
};

struct RegexEscapedMatcher {
    static constexpr auto parse(Stream stream) -> CompiledResult<parsi::fn::ExpectChar>
    {
        constexpr auto parser_escapes = parsi::sequence(  // special characters
            parsi::expect('\\'),
            parsi::anyof(
                parsi::expect(charset_escapes_in_group),  // escapes
                parsi::expect(parsi::Charset("ntr")),  // ascii escaped character repr
                parsi::repeat<3, 3>(parsi::expect(charset_digit)),  // octal ascii character repr
                parsi::repeat<2, 2>(parsi::expect(charset_hex)),  // hex ascii character reprc
                parsi::repeat<4, 4>(parsi::expect(charset_hex)),  // hex unicode 4 hex character repr
                parsi::repeat<8, 8>(parsi::expect(charset_hex))   // hex unicode 8 hex character repr
            )
        );
        return { .value = std::nullopt, .stream = stream };
    }
};

struct RegexCharacterClass {
    static constexpr auto parse(Stream stream) -> CompiledResult<ExpectNegatableCharset>
    {
        char char_class;
        auto parser_character_class = parsi::sequence(
            parsi::expect('\\'),
            parsi::extract(
                parsi::expect(parsi::Charset("SsWwDd")),
                [&](std::string_view str) { char_class = str[0]; }
            )
        );
        auto res = parser_character_class(stream);
        if (!res) {
            return { .value = std::nullopt, .stream = stream };
        }
        constexpr auto whitespaces = parsi::Charset("");
        constexpr auto wordchars = parsi::Charset("");
        constexpr auto digits = parsi::Charsest("0123456789");

        ExpectNegatableCharset parser;
        switch (char_class) {
            case 'S': parser = ExpectNegatableCharset{ .negated = true, .charset = whitespaces }; break;
            case 's': parser = ExpectNegatableCharset{ .negated = false, .charset = whitespaces }; break;
            case 'W': parser = ExpectNegatableCharset{ .negated = true, .charset = wordchars }; break;
            case 'w': parser = ExpectNegatableCharset{ .negated = false, .charset = wordchars }; break;
            case 'D': parser = ExpectNegatableCharset{ .negated = true, .charset = digits }; break;
            case 'd': parser = ExpectNegatableCharset{ .negated = false, .charset = digits }; break;
            default:
                return { .value = std::nullopt, .stream = res.stream };
        }
        return { .value = std::move(parser), .stream = res.stream };
    }
};

struct RegexBracketCharset {
    static constexpr auto parse(Stream stream) -> CompiledResult<parsi::fn::ExpectCharset>
    {
        ExpectNegatableCharset parser;
        auto range_extractor = [&](std::string_view rangestr) {
            const char start = rangestr[0];
            const char end = rangestr[2];
            for (char chr = start; chr <= end; ++chr) {
                parser.charset.set(static_cast<std::size_t>(chr));
            }
        };
        constexpr auto parser_bracket = parsi::sequence(  // character class / bracket list
            parsi::expect('['),
            parsi::optional(parsi::extract(parsi::expect('^'), [&](auto&&) { parser.negated = true; })),
            parsi::repeat(
                parsi::anyof(  // character ranges
                    parsi::extract(
                        parsi::sequence(parsi::expect(charset_lower), parsi::expect('-'). parsi::expect(charset_lower)),
                        range_extractor
                    ),
                    parsi::sequence(parsi::expect('\\'), ...),  // escapes
                    parsi::expect(...),  // normally usable chars
                )
            ),
            parsi::expect(']')
        );
        return { .value = std::nullopt, .stream = stream };
    }
};

struct RegexQuantifier {
    static constexpr auto parse(Stream stream) -> CompiledResult<Quantification>
    {
        Quantification quantification;
        auto parser_quantifiers = parsi::anyof(  // unary quantifer operators
            parsi::extract(parsi::expect('*'), [&](auto&&) { quantification = Quantification{}); }),
            parsi::extract(parsi::expect('+'), [&](auto&&) { quantification = Quantification{ .minimum = 1 }; }),
            parsi::extract(parsi::expect('?'), [&](auto&&) { quantification = Quantification{ .optional = true }; }),
            parsi::sequence(  // quantifier specifying only the minimum
                parsi::expect('{'),
                parsi::extract(
                    parsi::repeat<1>(parsi::expect(charset_digit)),
                    [&](std::string_view str) {
                        auto num = std::stoi(str);  // TODO error handling for overflow
                        quantification.minimum = num
                        quantification.maximim = num
                    }
                ),
                parsi::optional(parsi::extract(
                    parsi::expect(','),
                    [&](auto&&) { quantification.maximum = std::numeric_limits<std::size_t>::max(); }
                ),
                parsi::optional(parsi::extract(
                    parsi::repeat<1>(parsi::expect(charset_digit)),
                    [&](std::string_view str) {
                        auto num = std::stoi(str);  // TODO error handling for overflow
                        quantification.maximum = num
                    }
                )),
                parsi::expect('}')
            )
        );

        auto res = parser_quantifiers(stream);
        if (!res) {
            return { .value = std::nullopt, .stream = stream };
        }

        return { .value = quantification, .stream = res.stream };
    }
};

template <typename AccumulateF, typename StateT>
constexpr auto regex_compile_rec(Stream stream, AccumulateF&& accumulate, StateT state)
{
    if (stream.size() <= 0) {  // end of stream
        return CompiledResult(accumulate(std::move(state), regex_eos_tag{}), stream);
    }

    if (auto res = RegexExactMatcher::parse(stream); res.parser) {
        auto new_state = accumulate(std::move(state), std::move(*res.parser));
        if (!new_state) {
            return CompiledResult(std::move(new_state), stream);
        }
        return regex_compile_rec(res.stream, std::move(accumulate), std::move(new_state));
    }

    if (auto res = RegexEscapedMatcher::parse(stream); res.parser) {
        auto new_state = accumulate(std::move(state), std::move(*res.parser));
        if (!new_state) {
            return CompiledResult(std::move(new_state), stream);
        }
        return regex_compile_rec(res.stream, std::move(accumulate), std::move(new_state));
    }

    if (auto res = RegexCharacterClass::parse(stream); res.parser) {
        auto new_state = accumulate(std::move(state), std::move(*res.parser));
        if (!new_state) {
            return CompiledResult(std::move(new_state), stream);
        }
        return regex_compile_rec(res.stream, std::move(accumulate), std::move(new_state));
    }

    if (auto res = RegexBracketCharset::parse(stream); res.parser) {
        auto new_state = accumulate(std::move(state), std::move(*res.parser));
        if (!new_state) {
            return CompiledResult(std::move(new_state), stream);
        }
        return regex_compile_rec(res.stream, std::move(accumulate), std::move(new_state));
    }

    if (auto res = RegexQuantifier::parse(stream); res.parser) {
        auto new_state = accumulate(std::move(state), std::move(*res.parser));
        if (!new_state) {
            return CompiledResult(std::move(new_state), stream);
        }
        return regex_compile_rec(res.stream, std::move(accumulate), std::move(new_state));
    }

    if (stream.starts_with('.')) {
        auto new_state = accumulate(std::move(state), regex_any_tag{});
        if (!new_state) {
            return CompiledResult(std::move(new_state), stream);
        }
        stream.advance(1);
        return regex_compile_rec(stream, std::move(accumulate), std::move(new_state));
    }

    if (stream.starts_with('|')) {
        auto new_state = accumulate(std::move(state), regex_or_tag{});
        if (!new_state) {
            return CompiledResult(std::move(new_state), stream);
        }
        stream.advance(1);
        return regex_compile_rec(stream, std::move(accumulate), std::move(new_state));
    }

    if (stream.starts_with('(')) {  // subgroup start
        auto new_state = accumulate(std::move(state), regex_group_start_tag{});
        if (!new_state) {
            return CompiledResult(std::move(new_state), stream);
        }
        stream.advance(1);
        return regex_compile_rec(stream, std::move(accumulate), std::move(new_state));
    }

    if (stream.starts_with(')')) {  // subgroup end
        auto new_state = accumulate(std::move(state), regex_group_end_tag{});
        if (!new_state) {
            return CompiledResult(std::move(new_state), stream);
        }
        stream.advance(1);
        return regex_compile_rec(stream, std::move(accumulate), std::move(new_state));
    }

    if (auto res = RegexBackreferenceParser::parse(stream); res) {  // back reference
        auto new_state = accumulate(std::move(state), regex_backreference_tag{*res.parser});
        if (!new_state) {
            return CompiledResult(std::move(new_state), stream);
        }
        return regex_compile_rec(res.stream, std::move(accumulate), std::move(new_state));
    }

    return accumulate(std::move(state), regex_unknown_sequence_tag{});
}

}  // namespace internal

// template <internal::FixedString RegexStr>
// constexpr auto regex_compile() noexcept
// {
//     //
// }

template <internal::FixedString RegexStr>
constexpr auto regex_match(std::string_view input) noexcept -> bool
{
    // constexpr auto regex = regex_compile<RegexStr>();
    // return regex.is_match(input);
    return;
}

template <internal::FixedString RegexStr>
constexpr auto regex_extract(std::string_view input) noexcept
{
    constexpr auto regex = regex_compile<RegexStr>();
    return regex.extract(input);
}

}  // namespace parsi

#endif  // PARSI_REGEX_HPP
