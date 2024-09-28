#include <stdio.h>
#include <string.h>

#include "parsi/parsi-c.h"

typedef struct color {
    unsigned char r;
    unsigned char b;
    unsigned char g;
} color_t;

unsigned char convert_hex_digit(char digit) {
    if ('0' <= digit && digit <= '9') {
        return digit - '0';
    }

    if ('a' <= digit && digit <= 'f') {
        return 10 + (digit - 'a');
    }

    if ('A' <= digit && digit <= 'F') {
        return 10 + (digit - 'A');
    }

    // unreachable as parser has verified it
    return 0;
};

static bool color_extract_visitor(void* context, const char* str, size_t size)
{
    if (size != 6) {
        return false;
    }
    color_t* color = (color_t*)context;
    color->r = convert_hex_digit(str[0]) * 16 + convert_hex_digit(str[1]);
    color->g = convert_hex_digit(str[2]) * 16 + convert_hex_digit(str[3]);
    color->b = convert_hex_digit(str[4]) * 16 + convert_hex_digit(str[5]);
    return true;
}

static bool short_color_extract_visitor(void* context, const char* str, size_t size)
{
    if (size != 3) {
        return false;
    }
    color_t* color = (color_t*)context;
    color->r = convert_hex_digit(str[0]) * 16 + convert_hex_digit(str[0]);
    color->g = convert_hex_digit(str[1]) * 16 + convert_hex_digit(str[1]);
    color->b = convert_hex_digit(str[2]) * 16 + convert_hex_digit(str[2]);
    return true;
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        printf("Usage:\n\t%s <string>\n", argv[0]);
        return 1;
    }

    const char* str = argv[1];

    color_t color;

    parsi_charset_t hex_charset = parsi_charset("0123456789abcdefABCDEF");

    parsi_parser_t* parser = parsi_alloc_parser(
        parsi_combine_sequence((parsi_parser_t[]) {
            parsi_expect_char('#'),
            // after the initial '#' character, there can be either 6 (long verion) or 3 (short version) hex characters.
            // we can use anyof which retracts in stream and tries the next parser.
            // note these parsers aren't backtracking, and extract is always called when its inner parser is successful,
            // no matter if after anyof there is no `eos`.
            // thus for example here if we have short version parser first in `anyof`, then when the input is long version,
            // both `short_color_extract_visitor` and `color_extract_visitor` will be called.
            parsi_combine_anyof((parsi_parser_t[]) {
                parsi_combine_extract(
                    // parser_alloc_parser is used only just not to have to define
                    // the inner parsers separately with another variable and get a pointer to.
                    parsi_alloc_parser(parsi_combine_repeat(parsi_alloc_parser(parsi_expect_charset(hex_charset)), 6, 6, parsi_free_parser)),
                    color_extract_visitor,
                    &color,
                    NULL,
                    parsi_free_parser
                ),
                parsi_combine_extract(
                    parsi_alloc_parser(parsi_combine_repeat(parsi_alloc_parser(parsi_expect_charset(hex_charset)), 3, 3, parsi_free_parser)),
                    short_color_extract_visitor,
                    &color,
                    NULL,
                    parsi_free_parser
                ),
                parsi_none()
            }, NULL),
        parsi_expect_eos(),
        parsi_none()
        }, NULL)
    );

    parsi_compiled_parser_t* compiled_parser = parsi_compile(parser);

    parsi_result_t result = parsi_parse(compiled_parser, (parsi_stream_t){ .cursor = str, .size = strlen(str) });
    if (result.is_valid) {
        printf("color(%d, %d, %d)\n", (int)color.r, (int)color.g, (int)color.b);
    } else {
        printf("failed at (remaining size: %zu): %s\n", result.stream.size, result.stream.cursor);
    }

    parsi_free_compiled_parser(compiled_parser);
    parsi_free_parser(parser);

    return 0;
}
