#include "parsi/parsi.hpp"

#include <iostream>
#include <optional>
#include <array>

namespace json {

namespace internal {

constexpr auto expect_whitespace = parsi::expect(parsi::Charset("\t\n "));

template <typename G, typename F, typename ...Fs>
constexpr auto comma_separated_sequence_rec(G&& init, F&& first, Fs&& ...rest)
{
    if constexpr (sizeof...(Fs) == 0) {
        return parsi::sequence(
            std::forward<G>(init),
            parsi::repeat(internal::expect_whitespace),
            std::forward<F>(first)
        );
    } else {
        return comma_separated_sequence(
            parsi::sequence(
                std::forward<G>(init),
                parsi::repeat(internal::expect_whitespace),
                std::forward<F>(first),
                parsi::repeat(internal::expect_whitespace),
                parsi::expect(',')
            ),
            std::forward<Fs>(rest)...
        );
    }
}

template <typename ...Fs>
constexpr auto comma_separated_sequence(Fs&& ...parsers)
{
    return comma_separated_sequence_rec(
        [](parsi::Stream stream) {  // works as an identity, does nothing.
            return parsi::Result{stream, true};
        },
        std::forward<Fs>(parsers)...
    );
}

}  // namespace internal

template <typename F>
constexpr auto expect_null(F&& visitor)
{
    return parsi::extract(
        //parsi::expect("null"),
        parsi::sequence(
            parsi::expect('n'),
            parsi::expect('u'),
            parsi::expect('l'),
            parsi::expect('l')
        ),
        [visitor=std::forward<F>(visitor)](auto&&) { visitor(); }
    );
}

template <typename F>
constexpr auto expect_integer(F&& visitor)
{
    return [visitor=std::forward<F>(visitor)](parsi::Stream stream) -> parsi::Result {
        long long sign = +1;
        long long num = 0;

        if (stream.cursor >= stream.buffer.size()) {
            return parsi::Result{stream, false};
        }

        // sign character
        const auto opt_sign_char = stream.buffer[stream.cursor];
        if (opt_sign_char == '+' || opt_sign_char == '-') [[unlikely]] {
            sign = (opt_sign_char == '+') * +1
                 + (opt_sign_char == '-') * -1;
            ++stream.cursor;
            if (stream.cursor >= stream.buffer.size()) {
                return parsi::Result{stream, false};
            }
        }

        // first digit
        const auto first_digit_char = stream.buffer[stream.cursor];
        ++stream.cursor;
        if (!parsi::common::charset_digit.contains(first_digit_char)) [[unlikely]] {
            return parsi::Result{stream, false};
        }
        num = first_digit_char - '0';

        // rest of digits
        while (stream.cursor < stream.buffer.size()) {
            const auto digit_char = stream.buffer[stream.cursor];
            if (!parsi::common::charset_digit.contains(digit_char)) {
                break;
            }

            ++stream.cursor;

            num *= 10;
            num += digit_char - '0';
        }

        visitor(sign * num);

        return parsi::Result{stream, true};
    };
}

template <typename F>
constexpr auto expect_decimal(F&& visitor)
{
    return [visitor=std::forward<F>(visitor)](parsi::Stream stream) -> parsi::Result {
        long long integer_part = 0;
        long long decimal_part = 0;

        auto res = expect_integer([&integer_part](long long num) { integer_part = num; })(stream);
        if (!res) {
            return res;
        }
        stream = res.stream;

        if (stream.cursor >= stream.buffer.size()
                || stream.buffer[stream.cursor] != '.') [[unlikely]] {
            return parsi::Result{stream, false};
        }
        ++stream.cursor;

        long long divisor_power = 1;
        while (stream.cursor < stream.buffer.size()) {
            const auto digit_char = stream.buffer[stream.cursor];
            if (!parsi::common::charset_digit.map.test(digit_char)) {
                break;
            }

            divisor_power *= 10;
            ++stream.cursor;

            decimal_part *= 10;
            decimal_part += digit_char - '0';
        }

        // there must be at least one 1 digit after period
        if (divisor_power <= 1) {
            return parsi::Result{stream, false};
        }

        // TODO this doesn't work. use std::strtod or something.

        const double sign = (integer_part < 0) ? -1 : 1;
        const double decimal = static_cast<double>(decimal_part) / static_cast<double>(divisor_power);
        const double number = static_cast<double>(integer_part) + sign * decimal;

        visitor(number);

        return parsi::Result{stream, true};
    };
}

template <typename F>
constexpr auto expect_string_raw(F&& visitor)
{
    const auto escaped_charset = parsi::Charset("\\\"'nra");

    const auto normal_charset = []() {
        auto normal_charset = parsi::Charset("");
        for (char character = 0x20; character < 0x7f; ++character) {
            normal_charset.map.set(character, true);
        }
        normal_charset.map.set('"', false);
        normal_charset.map.set('\\', false);
        return normal_charset;
    }();

    return parsi::extract(
        parsi::sequence(
            parsi::expect('"'),
            parsi::repeat(
                parsi::anyof(
                    parsi::expect(normal_charset),
                    parsi::sequence(
                        parsi::expect('\\'),
                        parsi::expect(escaped_charset)
                    )
                )
            ),
            parsi::expect('"')
        ),
        std::forward<F>(visitor)
    );
}

template <typename F>
constexpr auto expect_string(F&& visitor)
{
    constexpr auto escape_map = []() {
        std::array<char, 256> map = {0};
        map[static_cast<unsigned char>('\\')] = '\\';
        map[static_cast<unsigned char>('"')] = '"';
        map[static_cast<unsigned char>('\'')] = '\'';
        map[static_cast<unsigned char>('n')] = '\n';
        map[static_cast<unsigned char>('r')] = '\r';
        map[static_cast<unsigned char>('a')] = '\a';
        return map;
    }();

    return expect_string_raw(
        [escape_map, visitor=std::forward<F>(visitor)](std::string_view str_raw) {
            std::string str;
            str.reserve(str_raw.size() - 2);  // 2 for quotations

            char last = '\0';
            for (char ch : str_raw.substr(1, str_raw.size()-2)) {
                if (last == '\\') [[unlikely]] {
                    str += escape_map[static_cast<unsigned char>(ch)];
                } else if (ch == '\\') [[unlikely]] {
                    last = ch;
                    continue;
                } else {
                    str += ch;
                }

                last = ch;
            }

            visitor(std::move(str));
        }
    );
}

template <typename ...Fs>
constexpr auto expect_tuple(Fs&& ...parsers)
{
    if constexpr (sizeof...(Fs) == 0) {
        return parsi::sequence(
            parsi::repeat(internal::expect_whitespace),
            parsi::expect('['),
            parsi::repeat(internal::expect_whitespace),
            parsi::expect(']'),
            parsi::repeat(internal::expect_whitespace)
        );
    } else {
        auto joint = parsi::sequence(
            parsi::repeat(internal::expect_whitespace),
            parsi::expect(','),
            parsi::repeat(internal::expect_whitespace)
        );
        return parsi::sequence(
            parsi::repeat(internal::expect_whitespace),
            parsi::expect('['),
            parsi::repeat(internal::expect_whitespace),
            comma_separated_sequence(std::forward<Fs>(parsers)...),
            parsi::repeat(internal::expect_whitespace),
            parsi::expect(']'),
            parsi::repeat(internal::expect_whitespace)
        );
    }
}

template <typename F>
constexpr auto expect_object(F&& visitor);
template <typename F>
constexpr auto expect_array(F&& visitor);

// what would be a good api design here?
// - visit a field optionally
// - require a field to exist
// - custom match on fields
//
template <typename F, typename G>
struct ExpectObject {
    std::remove_cvref_t<F> visitor;
    std::remove_cvref_t<G> concluder;

    constexpr auto operator()(parsi::Stream stream) const noexcept -> parsi::Result
    {
        auto parser = parsi::sequence(
            parsi::repeat(internal::expect_whitespace),
            parsi::expect('{'),
            parsi::repeat(internal::expect_whitespace),
            parsi::anyof(
                parsi::expect('}'),
                parsi::sequence(
                    [=](parsi::Stream stream) {
                        return items_sequence_parser(stream);
                    },
                    parsi::expect('}')
                )
            ),
            parsi::repeat(internal::expect_whitespace)
        );

        return parser(stream);
    }

private:
    constexpr auto key_value_parser(parsi::Stream stream) const noexcept -> parsi::Result
    {
        constexpr auto colon_parser = parsi::sequence(
            parsi::repeat(internal::expect_whitespace),
            parsi::expect(':'),
            parsi::repeat(internal::expect_whitespace)
        );

        parsi::Result res;

        std::string key;

        res = expect_string([&key](std::string str) { key = std::move(str); })(stream);
        if (!res) {
            return res;
        }

        res = colon_parser(stream);
        if (!res) {
            return res;
        }

        return visitor(key, res.stream);
    }

    constexpr auto items_sequence_parser(parsi::Stream stream) const noexcept -> parsi::Result
    {
        while (true) {
            auto res = parsi::sequence(
                parsi::repeat(internal::expect_whitespace),
                [=](parsi::Stream stream) {
                    return key_value_parser(stream);
                },
                parsi::repeat(internal::expect_whitespace)
            )(stream);
            if (!res) {
                return res;
            }
            stream = res.stream;

            res = parsi::expect(',')(stream);
            if (!res) {
                return parsi::Result{stream, true};
            }
            stream = res.stream;
        }
    }
};

template <typename F>
constexpr auto expect_object(F&& visitor)
{
    constexpr auto concluder = [](std::size_t) { /* nothing */ };
    return ExpectObject<F, decltype(concluder)>{
        std::forward<F>(visitor),
        std::move(concluder)
    };
}

template <typename F, typename G>
constexpr auto expect_array(F&& item_parser, G&& concluder)
{
    const auto items_sequence_parser = [=](parsi::Stream stream) -> parsi::Result {
        std::size_t index = 0;
        while (true) {
            auto res = parsi::sequence(
                    parsi::repeat(internal::expect_whitespace),
                    [=](parsi::Stream stream) -> parsi::Result {
                        return item_parser(index, stream);
                    },
                    parsi::repeat(internal::expect_whitespace)
                )(stream);
            if (!res) {
                return res;
            }

            stream = res.stream;

            res = parsi::expect(',')(stream);
            if (!res) {
                return parsi::Result{stream, true};
            }

            stream = res.stream;

            ++index;
        }

        concluder(index);
    };
    return parsi::sequence(
        parsi::repeat(internal::expect_whitespace),
        parsi::expect('['),
        parsi::repeat(internal::expect_whitespace),
        parsi::anyof(
            parsi::expect(']'),
            parsi::sequence(
                items_sequence_parser,
                parsi::expect(']')
            )
        ),
        parsi::repeat(internal::expect_whitespace)
    );
}

template <typename F>
constexpr auto expect_array(F&& item_parser)
{
    return expect_array<F>(
        std::forward<F>(item_parser),
        [](auto&&) { /* nothing */ }
    );
}

template <typename F>
constexpr auto on_field(std::string field, F&& parser)
{
    return [parser, field](std::string_view key, parsi::Stream stream) -> parsi::Result {
        if (key != field) {
            return parsi::Result{stream, false};
        }

        return parser(stream);
    };
}

struct Skip {
    constexpr auto operator()(parsi::Stream stream) const noexcept -> parsi::Result
    {
        constexpr auto item_parser = parsi::anyof(
            expect_null([](auto&& ...) {}),
            expect_string_raw([](auto&& ...) {}),
            expect_decimal([](auto&& ...) {}),
            expect_integer([](auto&& ...) {}),
            expect_array([](auto&&, parsi::Stream stream) {
                return Skip()(stream);
            }),
            expect_object([](auto&&, parsi::Stream stream) {
                return Skip()(stream);
            })
        );
        return item_parser(stream);
    };
};

constexpr auto skip()
{
    return Skip{};
}

template <typename F, typename ...Fs>
inline auto fields_impl(std::string_view key, parsi::Stream stream, F&& first, Fs&& ...rest)
{
    auto res = std::forward<F>(first)(key, stream);
    if (res) {
        return res;
    }

    if constexpr (sizeof...(Fs) == 0) {
        return parsi::Result{stream, false};
    } else {
        return fields_impl(key, stream, std::forward<Fs>(rest)...);
    }
}

template <typename ...Fs>
inline auto fields(Fs&& ...parsers)
{
    return [parsers...](std::string_view key, parsi::Stream stream) -> parsi::Result {
        return fields_impl(key, stream, std::move(parsers)...);
    };
}


struct Walk {
    std::string prefix = "";

    parsi::Result operator()(parsi::Stream stream) const noexcept
    {
        auto parser = parsi::anyof(
            json::expect_null([this]() {
                std::cout << prefix + " null" << std::endl;
            }),
            json::expect_string([this](std::string_view str) {
                std::cout << prefix + " str: " << str << std::endl;
            }),
            json::expect_decimal([this](double decimal) {
                std::cout << prefix + " decimal: " << decimal << std::endl;
            }),
            json::expect_integer([this](long long integer) {
                std::cout << prefix + " integer: " << integer << std::endl;
            }),
            expect_array([this](std::size_t index, parsi::Stream stream) {
                auto parser = Walk{ prefix + "[index:" + std::to_string(index) + "]" };
                return parser(stream);
            }),
            expect_object([this](auto&& key, parsi::Stream stream) {
                auto parser = Walk{ prefix + "[key:" + key + "]" };
                return parser(stream);
            })
        );
        return parser(stream);
    }
};

inline auto walk()
{
    return Walk();
}

constexpr auto validate()
{
    constexpr auto skip_parser = Skip{};
    return [skip_parser](parsi::Stream stream) -> parsi::Result {
        auto res = skip_parser(stream);
        if (!res) {
            return res;
        }
        return parsi::Result{stream, true};
    };
}

}  // namespace json

#include <iostream>

/**
 * TODOs
 *  + add expect_string_raw
 *  x add expect_tuple
 *  - fix decimal parser
 *  + add json validator (only syntax check)
 *    - make sure to avoid recursion
 *  - add better object fields api
 *  - somehow notify user on empty array/object?
 *  - add expect_field that ensures a key exists
 */
int main()
{
    constexpr auto json_str = R"(
        {
            "key": "value",
            "foo": [
                {
                    "bees": "fly"
                },
                [
                    0,1,2,3
                ],
                null,
                12345,
                -42,
                +7,
                3.14,
                +2.71,
                -0.00,
                "Hello World",
                "Escaped \"chars\" and \n lines"
            ]
        }
    )";

    auto parser = json::expect_object(json::fields(
        json::on_field("key", json::expect_string([](std::string_view str) {
            std::cout << "key: " << str << std::endl;
        })),
        json::on_field("foo", json::walk())
    ));

    auto res = parser(json_str);
    if (!res) {
        std::cout << "failed at: " << res.stream.buffer.substr(res.stream.cursor) << std::endl;
        return 1;
    }

    return 0;
}
