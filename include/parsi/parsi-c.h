#ifndef PARSI_PARSI_C_H
#define PARSI_PARSI_C_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    parsi_parser_type_none = 0,
    parsi_parser_type_custom,
    parsi_parser_type_eos,
    parsi_parser_type_char,
    parsi_parser_type_charset,
    parsi_parser_type_string,
    parsi_parser_type_static_string,
    parsi_parser_type_extract,
    parsi_parser_type_sequence,
    parsi_parser_type_anyof,
    parsi_parser_type_repeat,
    parsi_parser_type_optional
} parsi_parser_type_enum;

typedef struct {
    // 4 <= 256 / (8 * sizeof(size_t)), considering 64bit size_t
    size_t bitset[4];
} parsi_charset_t;

typedef struct {
    const char* cursor;
    size_t size;
} parsi_stream_t;

typedef struct {
    bool is_valid;
    parsi_stream_t stream;
} parsi_result_t;

struct parsi_parser;
typedef struct parsi_compiled_parser parsi_compiled_parser_t;

typedef parsi_result_t(*parsi_parser_fn_t)(void* context, parsi_stream_t stream);

typedef void(*parsi_free_fn_t)(void* ptr);
typedef void(*parsi_string_free_fn_t)(char* str, size_t size);
typedef void(*parsi_parser_free_fn_t)(struct parsi_parser* parser);
typedef void(*parsi_parser_list_free_fn_t)(struct parsi_parser* parsers, size_t size);

typedef bool(*parsi_extract_visitor_fn_t)(void* context, const char* str, size_t size);

typedef struct parsi_parser {
    parsi_parser_type_enum type;
    union {
        struct {
            parsi_parser_fn_t func;
            void* context;
            parsi_free_fn_t free_context_fn;
        } custom;

        struct {
            char expected;
        } expect_char;

        struct {
            parsi_charset_t expected;
        } expect_charset;

        struct {
            char* string;
            size_t size;
            parsi_string_free_fn_t free_string_fn;
        } expect_string;

        struct {
            const char* string;
            size_t size;
        } expect_static_string;

        struct {
            struct parsi_parser* parser;
            parsi_extract_visitor_fn_t func;
            void* context;
            parsi_free_fn_t free_context_fn;
            parsi_parser_free_fn_t free_parser_fn;
        } extract;

        struct {
            struct parsi_parser* parsers;
            size_t size;
            parsi_parser_list_free_fn_t free_list_fn;
        } sequence;

        struct {
            struct parsi_parser* parsers;
            size_t size;
            parsi_parser_list_free_fn_t free_list_fn;
        } anyof;

        struct {
            struct parsi_parser* parser;
            size_t min;
            size_t max;
            parsi_parser_free_fn_t free_parser_fn;
        } repeat;

        struct {
            struct parsi_parser* parser;
            parsi_parser_free_fn_t free_parser_fn;
        } optional;
    };
} parsi_parser_t;

parsi_charset_t parsi_charset(const char* str);
parsi_charset_t parsi_charset_n(const char* str, size_t size);

parsi_parser_t* parsi_alloc_parser(parsi_parser_t parser);
void parsi_free_parser(parsi_parser_t* parser);

parsi_compiled_parser_t* parsi_compile(parsi_parser_t* parser);
void parsi_free_compiled_parser(parsi_compiled_parser_t*);

parsi_result_t parsi_parse(parsi_compiled_parser_t* parser, parsi_stream_t stream);

//-- helpers

/** create a null/none parser object, useful for none-terminated lists */
parsi_parser_t parsi_none();
parsi_parser_t parsi_custom_parser(parsi_parser_fn_t func, void* context, parsi_free_fn_t free_context_fn);
parsi_parser_t parsi_expect_eos();
parsi_parser_t parsi_expect_char(char chr);
parsi_parser_t parsi_expect_charset(parsi_charset_t charset);
parsi_parser_t parsi_expect_charset_str(const char* charset_str);
parsi_parser_t parsi_expect_string(char* str, size_t size, parsi_string_free_fn_t free_string_fn);
parsi_parser_t parsi_expect_static_string(const char* str);
parsi_parser_t parsi_combine_extract(parsi_parser_t* parser, parsi_extract_visitor_fn_t func, void* context, parsi_free_fn_t free_context_fn, parsi_parser_free_fn_t free_parser_fn);
parsi_parser_t parsi_combine_sequence(parsi_parser_t* parsers, parsi_parser_list_free_fn_t free_list_fn);
parsi_parser_t parsi_combine_sequence_n(parsi_parser_t* parsers, size_t size, parsi_parser_list_free_fn_t free_list_fn);
parsi_parser_t parsi_combine_anyof(parsi_parser_t* parsers, parsi_parser_list_free_fn_t free_list_fn);
parsi_parser_t parsi_combine_anyof_n(parsi_parser_t* parsers, size_t size, parsi_parser_list_free_fn_t free_list_fn);
parsi_parser_t parsi_combine_repeat(parsi_parser_t* parser, size_t min, size_t max, parsi_parser_free_fn_t free_parser_fn);
parsi_parser_t parsi_combine_optional(parsi_parser_t* parser, parsi_parser_free_fn_t free_parser_fn);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // PARSI_PARSI_C_H
