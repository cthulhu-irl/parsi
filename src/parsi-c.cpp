#include "parsi/parsi-c.h"

#include <cstddef>
#include <cstdlib>
#include <cstring>

#include "parsi/parsi.hpp"

static bool parsi_charset_check(const parsi_charset_t* charset, char chr)
{
    const std::size_t idx = static_cast<std::size_t>(chr);
    const std::size_t bit = static_cast<std::size_t>(1) << (idx & 63ull);  // idx % 64
    return charset->bitset[idx >> 6ull] & bit;  // idx / 64
}

namespace parsi::details {

template <bool IsValidV>
constexpr auto always_parser = [](Stream stream) { return Result{stream, IsValidV}; };

template <typename F>
static auto parser_list_parameter_pack_visit(const std::size_t count, parsi_parser_t* parsers, F&& callback)
{
    switch (count)
    {
        case 0: return callback();
        case 1: return callback(parsers[0]);
        case 2: return callback(parsers[0], parsers[1]);
        case 3: return callback(parsers[0], parsers[1], parsers[2]);
        case 4: return callback(parsers[0], parsers[1], parsers[2], parsers[3]);
        case 5: return callback(parsers[0], parsers[1], parsers[2], parsers[3], parsers[4]);
        case 6: return callback(parsers[0], parsers[1], parsers[2], parsers[3], parsers[4], parsers[5]);
        case 7: return callback(parsers[0], parsers[1], parsers[2], parsers[3], parsers[4], parsers[5], parsers[6]);
        case 8: return callback(parsers[0], parsers[1], parsers[2], parsers[3], parsers[4], parsers[5], parsers[6], parsers[7]);
        case 9: return callback(parsers[0], parsers[1], parsers[2], parsers[3], parsers[4], parsers[5], parsers[6], parsers[7], parsers[8]);
        case 10: return callback(parsers[0], parsers[1], parsers[2], parsers[3], parsers[4], parsers[5], parsers[6], parsers[7], parsers[8], parsers[9]);
        default: break;
    }

    // TODO
    return callback(parsers[0], parsers[1], parsers[2], parsers[3], parsers[4], parsers[5], parsers[6], parsers[7], parsers[8], parsers[9], parsers[10]);
}

template <typename CallbackF>
static auto compile_impl(parsi_parser_t* parser, CallbackF&& wrapper_cb)
{
    if (!parser) [[unlikely]] {
        return wrapper_cb(always_parser<false>);
    }

    constexpr auto expect_custom_charset = [](parsi_charset_t charset) {
        return [charset=charset](Stream stream) -> Result {
            if (stream.size() >= 1 && parsi_charset_check(&charset, stream.front())) [[likely]] {
                stream.advance(1);
                return Result{stream, true};
            }
            return Result{stream, false};
        };
    };

    switch (parser->type)
    {
        case parsi_parser_type_none:
            return wrapper_cb(always_parser<false>);
        case parsi_parser_type_custom:
            return wrapper_cb([func=parser->custom.func, context=parser->custom.context](Stream stream) {
                auto result = func(context, parsi_stream_t{.cursor = stream.data(), .size = stream.size()});
                return Result{Stream(result.stream.cursor, result.stream.size), result.is_valid};
            });
        case parsi_parser_type_eos:
            return wrapper_cb(fn::Eos{});
        case parsi_parser_type_char:
            return wrapper_cb(fn::ExpectChar{parser->expect_char.expected});
        case parsi_parser_type_charset:
            return wrapper_cb(expect_custom_charset(parser->expect_charset.expected));
        case parsi_parser_type_string:
            return wrapper_cb(fn::ExpectStringView{std::string_view(parser->expect_string.string, parser->expect_string.size)});
        case parsi_parser_type_static_string:
            return wrapper_cb(fn::ExpectStringView{std::string_view(parser->expect_static_string.string, parser->expect_static_string.size)});
        case parsi_parser_type_extract:
            return wrapper_cb(extract(
                compile_impl(parser->extract.parser, wrapper_cb),
                [func=parser->extract.func, context=parser->extract.context](std::string_view str) {
                    return func(context, str.data(), str.size());
                }
            ));
        case parsi_parser_type_sequence:
            return parser_list_parameter_pack_visit(parser->sequence.size, parser->sequence.parsers, [&]<typename ...Ts>(Ts&& ...parsers) {
                return wrapper_cb(sequence(compile_impl(&parsers, wrapper_cb)...));
            });
        case parsi_parser_type_anyof:
            return parser_list_parameter_pack_visit(parser->anyof.size, parser->anyof.parsers, [&]<typename ...Ts>(Ts&& ...parsers) {
                return wrapper_cb(anyof(compile_impl(&parsers, wrapper_cb)...));
            });
        case parsi_parser_type_repeat:
            if (parser->repeat.min == 0 && parser->repeat.max == std::numeric_limits<std::size_t>::max()) {
                if (parser->repeat.parser->type == parsi_parser_type_char) {
                    return wrapper_cb(repeat(expect(parser->repeat.parser->expect_char.expected)));
                }
                if (parser->repeat.parser->type == parsi_parser_type_charset) {
                    return wrapper_cb(repeat(expect_custom_charset(parser->repeat.parser->expect_charset.expected)));
                }
                return wrapper_cb(repeat(compile_impl(parser->repeat.parser, wrapper_cb)));
            }
            return wrapper_cb(repeat(compile_impl(parser->repeat.parser, wrapper_cb), parser->repeat.min, parser->repeat.max));
        case parsi_parser_type_optional:
            return wrapper_cb(optional(compile_impl(parser->optional.parser, wrapper_cb)));
    }

    // this should be unreachable.
    return wrapper_cb(always_parser<false>);
}

static RTParser compile_to_rtparser(parsi_parser_t* parser)
{
    return compile_impl(parser, []<typename ParserT>(ParserT&& p) { return RTParser(std::forward<ParserT>(p)); });
}

} // namespace parsi::details

struct parsi_compiled_parser {
    parsi::RTParser parser;
};

parsi_charset_t parsi_charset(const char* str)
{
    return parsi_charset_n(str, std::strlen(str));
}

parsi_charset_t parsi_charset_n(const char* str, size_t size)
{
    std::size_t bytes[4] = {0};
    for (std::size_t index = 0; index < size; ++index) {
        const unsigned char chr = static_cast<unsigned char>(str[index]);
        const std::size_t bit = static_cast<std::size_t>(1) << (chr & 63);  // chr % 64
        bytes[chr >> 6] |= bit;  // chr / 64
    }
    return parsi_charset_t{ .bitset = { bytes[0], bytes[1], bytes[2], bytes[3] } };
}

parsi_parser_t* parsi_alloc_parser(parsi_parser_t parser)
{
    parsi_parser_t* parser_ptr = (parsi_parser_t*)std::malloc(sizeof(parser));
    if (!parser_ptr)
    {
        return NULL;
    }
    std::memcpy(parser_ptr, &parser, sizeof(parser));
    return parser_ptr;
}

static void parsi_free_impl(parsi_parser_t* parser)
{
    switch (parser->type) {
        case parsi_parser_type_string:
            parser->expect_string.free_string_fn(parser->expect_string.string, parser->expect_string.size);
            break;

        case parsi_parser_type_sequence:
            for (std::size_t index = 0; index < parser->sequence.size; ++index) {
                parsi_free_impl(&(parser->sequence.parsers[index]));
            }
            if (parser->sequence.free_list_fn) {
                parser->sequence.free_list_fn(parser->sequence.parsers, parser->sequence.size);
            }
            break;

        case parsi_parser_type_anyof:
            for (std::size_t index = 0; index < parser->anyof.size; ++index) {
                parsi_free_impl(&(parser->anyof.parsers[index]));
            }
            if (parser->anyof.free_list_fn) {
                parser->anyof.free_list_fn(parser->anyof.parsers, parser->anyof.size);
            }
            break;

        case parsi_parser_type_repeat:
            if (parser->repeat.free_parser_fn)
            {
                parser->repeat.free_parser_fn(parser->repeat.parser);
            }
            break;

        case parsi_parser_type_optional:
            if (parser->optional.free_parser_fn)
            {
                parser->optional.free_parser_fn(parser->optional.parser);
            }
            break;

        case parsi_parser_type_extract:
            if (parser->extract.free_context_fn)
            {
                parser->extract.free_context_fn(parser->extract.context);
            }
            if (parser->extract.free_parser_fn)
            {
                parser->extract.free_parser_fn(parser->extract.parser);
            }
            break;

        case parsi_parser_type_custom:
            if (parser->custom.free_context_fn)
            {
                parser->custom.free_context_fn(parser->custom.context);
            }
            break;

        case parsi_parser_type_eos:
        case parsi_parser_type_char:
        case parsi_parser_type_charset:
        case parsi_parser_type_static_string:
        default:
            break;
    }
}

void parsi_free_parser(parsi_parser_t* parser)
{
    parsi_free_impl(parser);
    std::free(parser);
}

parsi_compiled_parser_t* parsi_compile(parsi_parser_t* parser)
{
    return new parsi_compiled_parser_t{parsi::details::compile_to_rtparser(parser)};
}

void parsi_free_compiled_parser(parsi_compiled_parser_t* compiled_parser)
{
    delete compiled_parser;
}

parsi_result_t parsi_parse(parsi_compiled_parser_t* compiled_parser, parsi_stream_t stream)
{
    auto result = compiled_parser->parser(parsi::Stream(stream.cursor, stream.size));
    return parsi_result_t{.is_valid = result.is_valid(), .stream = parsi_stream_t{.cursor = result.stream().data(), .size = result.stream().size()}};
}

//-- helpers

parsi_parser_t parsi_none()
{
    return parsi_parser_t{ .type = parsi_parser_type_none };
}

parsi_parser_t parsi_custom_parser(parsi_parser_fn_t func, void* context, parsi_free_fn_t free_context_fn)
{
    return parsi_parser_t{
        .type = parsi_parser_type_custom,
        .custom = {
            .func = func,
            .context = context,
            .free_context_fn = free_context_fn
        }
    };
}

parsi_parser_t parsi_expect_eos()
{
    return parsi_parser_t{ .type = parsi_parser_type_eos };
}

parsi_parser_t parsi_expect_char(char chr)
{
    return parsi_parser_t{
        .type = parsi_parser_type_char,
        .expect_char = { .expected = chr }
    };
}

parsi_parser_t parsi_expect_charset(parsi_charset_t charset)
{
    return parsi_parser_t{
        .type = parsi_parser_type_charset,
        .expect_charset = { .expected = charset }
    };
}

parsi_parser_t parsi_expect_charset_str(const char* charset_str)
{
    return parsi_parser_t{
        .type = parsi_parser_type_charset,
        .expect_charset = { .expected = parsi_charset(charset_str) }
    };
}

parsi_parser_t parsi_expect_string(char* str, size_t size, parsi_string_free_fn_t free_string_fn)
{
    return parsi_parser_t{
        .type = parsi_parser_type_string,
        .expect_string = {
            .string = str,
            .size = size,
            .free_string_fn = free_string_fn
        }
    };
}

parsi_parser_t parsi_expect_static_string(const char* str)
{
    return parsi_parser_t{
        .type = parsi_parser_type_static_string,
        .expect_static_string = { .string = str, .size = strlen(str) }
    };
}

parsi_parser_t parsi_combine_extract(parsi_parser_t* parser, parsi_extract_visitor_fn_t func, void* context, parsi_free_fn_t free_context_fn, parsi_parser_free_fn_t free_parser_fn)
{
    return parsi_parser_t{
        .type = parsi_parser_type_extract,
        .extract = {
            .parser = parser,
            .func = func,
            .context = context,
            .free_context_fn = free_context_fn,
            .free_parser_fn = free_parser_fn
        }
    };
}

parsi_parser_t parsi_combine_sequence(parsi_parser_t* parsers, parsi_parser_list_free_fn_t free_list_fn)
{
    if (!parsers) {
        return parsi_parser_t{ .type = parsi_parser_type_none };
    }

    size_t size = 0;
    parsi_parser_t* head = parsers;
    while (head->type != parsi_parser_type_none) {
        head = &parsers[++size];
    }

    return parsi_parser_t{
        .type = parsi_parser_type_sequence,
        .sequence = { .parsers = parsers, .size = size, .free_list_fn = free_list_fn }
    };
}

parsi_parser_t parsi_combine_sequence_n(parsi_parser_t* parsers, size_t size, parsi_parser_list_free_fn_t free_list_fn)
{
    return parsi_parser_t{
        .type = parsi_parser_type_sequence,
        .sequence = { .parsers = parsers, .size = size, .free_list_fn = free_list_fn }
    };
}

parsi_parser_t parsi_combine_anyof(parsi_parser_t* parsers, parsi_parser_list_free_fn_t free_list_fn)
{
    if (!parsers) {
        return parsi_parser_t{ .type = parsi_parser_type_none };
    }

    size_t size = 0;
    parsi_parser_t* head = parsers;
    while (head->type != parsi_parser_type_none) {
        head = &parsers[++size];
    }

    return parsi_parser_t{
        .type = parsi_parser_type_anyof,
        .anyof = { .parsers = parsers, .size = size, .free_list_fn = free_list_fn }
    };
}

parsi_parser_t parsi_combine_anyof_n(parsi_parser_t* parsers, size_t size, parsi_parser_list_free_fn_t free_list_fn)
{
    return parsi_parser_t{
        .type = parsi_parser_type_anyof,
        .anyof = { .parsers = parsers, .size = size, .free_list_fn = free_list_fn }
    };
}

parsi_parser_t parsi_combine_repeat(parsi_parser_t* parser, size_t min, size_t max, parsi_parser_free_fn_t free_parser_fn)
{
    return parsi_parser_t{
        .type = parsi_parser_type_repeat,
        .repeat = { .parser = parser, .min = min, .max = max, .free_parser_fn = free_parser_fn }
    };
}

parsi_parser_t parsi_combine_optional(parsi_parser_t* parser, parsi_parser_free_fn_t free_parser_fn)
{
    return parsi_parser_t{
        .type = parsi_parser_type_optional,
        .optional = { .parser = parser, .free_parser_fn = free_parser_fn }
    };
}
