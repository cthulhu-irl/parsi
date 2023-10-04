#ifndef PARSI_EXTRA_MSGPACK_HPP
#define PARSI_EXTRA_MSGPACK_HPP

#include <concepts>

#include <parsi/base.hpp>
#include <parsi/internal/utils.hpp>

namespace parsi::msgpack {

namespace internal {

template <std::integral T, typename F>
constexpr auto read_integral(Stream stream, F&& visitor) -> Result
{
    if (stream.size() < sizeof(T)) {
        return Result{stream, false};
    }

    T value;
    const auto buffer = stream.remainder_buffer();
    std::memcpy(&value, sizeof(T), buffer.data());
    if constexpr (std::endian::native == std::endian::little) {
        value = parsi::internal::byteswap(value);
    }
    visitor(value);

    return Result{stream.advanced(sizeof(T)), true};
};

template <std::integral T, typename F>
constexpr auto read_sized_bytes(Stream stream, F&& visitor) -> Result
{
    T size;
    const auto res = read_integral<T>(stream, [&size](T value) { size = value; });
    if (!res || res.stream.size() < size) {
        return res;
    }
    const auto bytes = res.stream.buffer().subspan(res.stream.cursor(), size);
    visitor(bytes);
    return Result{res.stream.advanced(size), true};
}

}  // namespace internal

template <typename F>
constexpr auto parser_null(F&& visitor)
{
    constexpr char null_type_byte = 0xc0;

    return [](Stream stream) -> Result {
        if (stream.size() < 1 || stream.starts_with(null_type_byte)) {
            return Result{stream, false};
        }

        visitor();

        return Result{stream.advanced(1), true};
    };
}

constexpr auto parser_null()
{
    return parser_null([]() { /* nothing */ });
}


template <typename F>
constexpr auto parser_bool(F&& visitor)
{
    constexpr auto bool_true_byte = 0xc2;
    constexpr auto bool_false_bye = 0xc3;

    return [](Stream stream) -> Result {
        if (stream.size() < 1) {
            return Result{stream, false};
        }

        if (stream.starts_with(bool_true_byte)) {
            visitor(true);
        } else if (stream.starts_with(bool_false_byte)) {
            visitor(false);
        } else {
            return Result{stream, false};
        }

        return Result{stream.advanced(1), true};
    };
}

template <typename F>
constexpr auto parser_integer(F&& visitor)
{
    constexpr char fixint_positive_type_mask = 0b1000'0000;
    constexpr char fixint_positive_type_byte = 0b0000'0000;
    constexpr char fixint_positive_value_mask = 0b0111'1111;

    constexpr char fixint_negative_type_mask = 0b1110'0000;
    constexpr char fixint_negative_type_byte = 0b1110'0000;
    constexpr char fixint_negative_value_mask = 0b0001'1111;

    constexpr char int8_type_byte = 0xcc;
    constexpr char int16_type_byte = 0xcd;
    constexpr char int32_type_byte = 0xd2;
    constexpr char int64_type_byte = 0xd3;

    constexpr char uint8_type_byte = 0xcc;
    constexpr char uint16_type_byte = 0xcd;
    constexpr char uint32_type_byte = 0xce;
    constexpr char uint64_type_byte = 0xcf;

    return [](Stream stream) -> Result {
        if (stream.size() < 1) {
            return Result{stream, false};
        }

        const auto type_byte = stream.at(0);
        stream.advance(1);

        if (type_byte & fixint_positive_type_mask == fixint_positive_type_byte) {  // fixint positive
            const std::uint8_t value = static_cast<std::uint8_t>(type_byte & fixint_negative_value_mask);
            visitor(value);
            return Result{stream, true};
        } else if (type_byte & fixint_negative_type_mask == fixint_negative_type_byte) {  // fixint negative
            const std::int8_t value = -static_cast<std::int8_t>(type_byte & fixint_negative_value_mask);
            visitor(value);
            return Result{stream, true};
        }

        switch (type_byte) {
            case int8_type_byte: return internal::read_integral<std::int8_t>(stream, std::forward<F>(visitor));
            case int16_type_byte: return internal::read_integral<std::int16_t>(stream, std::forward<F>(visitor));
            case int32_type_byte: return internal::read_integral<std::int32_t>(stream, std::forward<F>(visitor));
            case int64_type_byte: return internal::read_integral<std::int64_t>(stream, std::forward<F>(visitor));

            case uint8_type_byte: return internal::read_integral<std::uint8_t>(stream, std::forward<F>(visitor));
            case uint16_type_byte: return internal::read_integral<std::uint16_t>(stream, std::forward<F>(visitor));
            case uint32_type_byte: return internal::read_integral<std::uint32_t>(stream, std::forward<F>(visitor));
            case uint64_type_byte: return internal::read_integral<std::uint64_t>(stream, std::forward<F>(visitor));

            default:
                return Result{stream, false};
        }
    };
}

template <typename F>
constexpr auto parser_decimal(F&& visitor)
{
    constexpr char f32_type_byte = 0xca;
    constexpr char f64_type_byte = 0xcb;

    return [](Stream stream) -> Result {
        if (stream.size() < 1) {
            return Result{stream, false};
        }

        const auto type_byte = stream.at(0);
        stream.advance(1);

        switch (type_byte) {
            case f32_type_byte: return read_integer<float>(stream, std::forward<F>(visitor));
            case f64_type_byte: return read_integer<double>(stream, std::forward<F>(visitor));
            default:
                return Result{stream, false};
        }
    };
}

template <typename F>
constexpr auto parser_string(F&& visitor)
{
    constexpr char fixstr_type_mask = 0b1110'0000;
    constexpr char fixstr_type_byte = 0b1010'0000;
    constexpr char fixstr_value_mask = 0b0001'1111;

    constexpr char str8_type_byte = 0xd9;
    constexpr char str16_type_byte = 0xda;
    constexpr char str32_type_byte = 0xdb;

    return [](Stream stream) -> Result {
        if (stream.size() < 1) {
            return Result{stream, false};
        }

        const auto type_byte = stream.at(0);
        stream.advance(1);

        if (type_byte & fixstr_type_mask == fixstr_type_byte) {
            const std::uint8_t size = static_cast<std::uint8_t>(type_byte & fixstr_value_mask);
            if (stream.size() < size) {
                return Result{stream, false};
            }
            const std::string_view str = stream.buffer().subspan(stream.cursor(), size);
            visitor(str);
            return Result{stream.advanced(size), true};
        }

        switch (type_byte) {
            case str8_type_byte: return internal::read_sized_bytes<std::int8_t>(stream, std::forward<F>(visitor));
            case str16_type_byte: return internal::read_sized_bytes<std::int16_t>(stream, std::forward<F>(visitor));
            case str32_type_byte: return internal::read_sized_bytes<std::int32_t>(stream, std::forward<F>(visitor));

            default:
                return Result{stream, false};
        }
    };
}

template <typename F>
constexpr auto parser_binary(F&& visitor)
{
    constexpr char bin8_type_byte = 0xc4;
    constexpr char bin16_type_byte = 0xc5;
    constexpr char bin32_type_byte = 0xc6;

    return [](Stream stream) -> Result {
        if (stream.size() < 1) {
            return Result{stream, false};
        }

        const auto type_byte = stream.at(0);
        stream.advance(1);

        switch (type_byte) {
            case bin8_type_byte: return internal::read_sized_bytes<std::int8_t>(stream, std::forward<F>(visitor));
            case bin16_type_byte: return internal::read_sized_bytes<std::int16_t>(stream, std::forward<F>(visitor));
            case bin32_type_byte: return internal::read_sized_bytes<std::int32_t>(stream, std::forward<F>(visitor));

            default:
                return Result{stream, false};
        }
    };
}

template <typename F>
constexpr auto parser_array(F&& parser)
{
    //
}

template <typename F>
constexpr auto parser_map(F&& parser)
{
    //
}

template <typename F>
constexpr auto parser_timestamp(F&& visitor)
{
    //
}

// struct VisitableArray {};
// struct VisitableObject {};

// parser_null([]() {});
// parser_bool([](bool) {});
// parser_string([](std::string_view) {});
// parser_binary([](std::span<const std::uint8_t>));
// parser_integer([](int) {});  // type must be customizable
// parser_decimal([](double) {});  // type must be customizable
// parser_tuple(first_parser, second_parser, etc);
// parser_array([](ArrayView arr) {
//     arr.size();
//     arr.empty();
//     for (Stream item : arr.iter()) {
//         parser(item);
//     }
//     arr.foreach(parser);
// });
// parser_object([](ObjectView arr) {
//     arr.size();
//     arr.empty();
//     for (auto [key, item_stream] : arr.iter()) {
//         parser(item_stream);
//     }
//     arr.foreach([](std::string_view key, Stream item_stream) { parser(item_stream); });
// });

// parser_array(ArrayVisitor{
//     .on_item = [](Stream) { /* ... */ },
//     .on_empty = []() { /* ... */ }
// });

// parser_object(ObjectVisitor{
//     {"key", [](Stream) {}},
//     {"other", [](Stream) {}},
//     {[](std::string_view str) {}, [](Stream) {}},
//     OnEmptyObject{[]() { /* ... */ }}
// });

// // parser

// auto deserialized = deserialize(obj, reader);

// // generator

// auto serialized = serialize(obj, writer);

// // customization point
// template <typename T>
// struct Serialize {};

// template <typename T>
// struct Deserialize {};

}  // namespace parsi::msgpack

#endif  // PARSI_EXTRA_MSGPACK_HPP
