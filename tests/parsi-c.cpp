#include <catch2/catch_all.hpp>

#include "parsi/parsi-c.h"


TEST_CASE("c expect char")
{
    SECTION("type and value")
    {
        auto parser = parsi_expect_char('a');
        REQUIRE(parser.type == parsi_parser_type_char);
        CHECK(parser.expect_char.expected == 'a');
    }

    SECTION("whitespace")
    {
        auto parser = parsi_expect_char(' ');
        REQUIRE(parser.type == parsi_parser_type_char);
        CHECK(parser.expect_char.expected == ' ');
    }

    SECTION("NUL character")
    {
        auto parser = parsi_expect_char('\0');
        REQUIRE(parser.type == parsi_parser_type_char);
        CHECK(parser.expect_char.expected == '\0');
    }

    SECTION("parse")
    {
        auto parser = parsi_expect_char('X');
        auto compiled_parser = parsi_compile(&parser);
        CHECK(parsi_parse(compiled_parser, parsi_stream_t{ .cursor = "X", .size = 1 }).is_valid);
        CHECK(!parsi_parse(compiled_parser, parsi_stream_t{ .cursor = "x", .size = 1 }).is_valid);
        CHECK(!parsi_parse(compiled_parser, parsi_stream_t{ .cursor = "xX", .size = 2 }).is_valid);
        CHECK(!parsi_parse(compiled_parser, parsi_stream_t{ .cursor = "", .size = 0 }).is_valid);
        parsi_free_compiled_parser(compiled_parser);
    }
}

TEST_CASE("c expect charset")
{
    SECTION("charset and charset_n")
    {
        std::string_view charset_chars = "0123456789ABCDEFabcdef";
        auto first = parsi_charset(charset_chars.data());
        auto second = parsi_charset_n(charset_chars.data(), charset_chars.size());
        for (std::size_t index = 0; index < std::size(first.bitset); ++index)
        {
            CHECK(first.bitset[index] == second.bitset[index]);
        }
    }

    SECTION("NUL-character")
    {
        auto first_parser = parsi_expect_charset(parsi_charset("a\0b"));
        auto second_parser = parsi_expect_charset(parsi_charset_n("a\0b", 3));

        auto first_compiled_parser = parsi_compile(&first_parser);
        auto second_compiled_parser = parsi_compile(&second_parser);

        CHECK(!parsi_parse(first_compiled_parser, parsi_stream_t{ .cursor = "\0", .size = 1 }).is_valid);
        CHECK(parsi_parse(second_compiled_parser, parsi_stream_t{ .cursor = "\0", .size = 1 }).is_valid);

        parsi_free_compiled_parser(first_compiled_parser);
        parsi_free_compiled_parser(second_compiled_parser);
    }

    SECTION("match")
    {
        std::string_view charset_chars = "0123456789ABCDEFabcdef";
        auto parser = parsi_expect_charset(parsi_charset(charset_chars.data()));
        auto compiled_parser = parsi_compile(&parser);

        for (std::size_t index = 0; index < charset_chars.size() - 1; ++index)
        {
            CHECK(parsi_parse(compiled_parser, parsi_stream_t{ .cursor = charset_chars.substr(index).data(), .size = 1 }).is_valid);
        }

        std::string_view some_non_charset_chars = "SGHJKIsghji []'\"\\|.,";
        for (std::size_t index = 0; index < some_non_charset_chars.size(); ++index)
        {
            CHECK(!parsi_parse(compiled_parser, parsi_stream_t{ .cursor = some_non_charset_chars.substr(index).data(), .size = 1 }).is_valid);
        }

        parsi_free_compiled_parser(compiled_parser);
    }
}

TEST_CASE("c expect static string")
{
    SECTION("parse")
    {
        auto parser = parsi_expect_static_string("hello");
        auto compiled_parser = parsi_compile(&parser);
        CHECK(parsi_parse(compiled_parser, parsi_stream_t{ .cursor = "hello", .size = 5 }).is_valid);
        CHECK(parsi_parse(compiled_parser, parsi_stream_t{ .cursor = "hello world", .size = 11 }).is_valid);
        CHECK(!parsi_parse(compiled_parser, parsi_stream_t{ .cursor = "bello world", .size = 11 }).is_valid);
        CHECK(!parsi_parse(compiled_parser, parsi_stream_t{ .cursor = "hell", .size = 4 }).is_valid);
        CHECK(!parsi_parse(compiled_parser, parsi_stream_t{ .cursor = " hello", .size = 4 }).is_valid);
        parsi_free_compiled_parser(compiled_parser);
    }
}

TEST_CASE("c extract")
{
    SECTION("parse")
    {
        struct context_type {
            std::string str = "";
            std::size_t visitor_call_counter = 0;
            std::size_t free_context_call_counter = 0;
        };

        context_type ctx;

        constexpr auto visitor_fn = [](void* context, const char* str, size_t size) -> bool {
            context_type& ctx = *reinterpret_cast<context_type*>(context);
            auto str_view = std::string_view(str, size);
            CHECK(str_view == "X");
            ctx.str += str_view;
            ++ctx.visitor_call_counter;
            return true;
        };

        constexpr auto free_context_fn = [](void* context) {
            context_type& ctx = *reinterpret_cast<context_type*>(context);
            ++ctx.free_context_call_counter;
        };

        constexpr auto free_parser_fn = [](parsi_parser_t* parser) {
            CHECK(parser->type == parsi_parser_type_char);
            CHECK(parser->expect_char.expected == 'X');
        };

        auto sub_parser = parsi_expect_char('X');
        auto parser = parsi_alloc_parser(parsi_combine_extract(&sub_parser, visitor_fn, &ctx, free_context_fn, free_parser_fn));
        auto compiled_parser = parsi_compile(parser);

        CHECK(parsi_parse(compiled_parser, parsi_stream_t{ .cursor = "Xx12", .size = 4 }).is_valid);
        CHECK(parsi_parse(compiled_parser, parsi_stream_t{ .cursor = "X", .size = 1 }).is_valid);
        CHECK(!parsi_parse(compiled_parser, parsi_stream_t{ .cursor = "x", .size = 1 }).is_valid);
        CHECK(!parsi_parse(compiled_parser, parsi_stream_t{ .cursor = "xX", .size = 2 }).is_valid);

        parsi_free_parser(parser);
        parsi_free_compiled_parser(compiled_parser);

        CHECK(ctx.str == "XX");
        CHECK(ctx.visitor_call_counter == 2);
        CHECK(ctx.free_context_call_counter == 1);
    }
}

TEST_CASE("c eos")
{
    auto parser = parsi_expect_eos();
    CHECK(parser.type == parsi_parser_type_eos);

    auto compiled_parser = parsi_compile(&parser);

    CHECK(parsi_parse(compiled_parser, parsi_stream_t{ .cursor = "", .size = 0 }).is_valid);
    CHECK(parsi_parse(compiled_parser, parsi_stream_t{ .cursor = "test", .size = 0 }).is_valid);

    CHECK(!parsi_parse(compiled_parser, parsi_stream_t{ .cursor = "x", .size = 1 }).is_valid);
    CHECK(!parsi_parse(compiled_parser, parsi_stream_t{ .cursor = "test", .size = 4 }).is_valid);
    CHECK(!parsi_parse(compiled_parser, parsi_stream_t{ .cursor = "\0", .size = 1 }).is_valid);

    parsi_free_compiled_parser(compiled_parser);
}

TEST_CASE("c custom")
{
    struct context_t {
        std::string viewed = "";
        std::size_t parser_call_counter = 0;
        std::size_t free_context_call_counter = 0;
    };

    context_t ctx;

    constexpr auto parser_fn = [](void* context, parsi_stream_t stream) -> parsi_result_t {
        context_t& ctx = *reinterpret_cast<context_t*>(context);
        auto strview = std::string_view(stream.cursor, stream.size);
        ctx.viewed += strview;
        ctx.parser_call_counter++;
        bool is_valid = strview == "World";
        if (is_valid) {
            stream.cursor += stream.size;
            stream.size = 0;
        }
        return parsi_result_t{ .is_valid = is_valid, .stream = stream };
    };

    constexpr auto free_context_fn = [](void* context) {
        context_t& ctx = *reinterpret_cast<context_t*>(context);
        ctx.free_context_call_counter++;
    };

    auto parser = parsi_alloc_parser(parsi_custom_parser(parser_fn, &ctx, free_context_fn));
    auto compiled_parser = parsi_compile(parser);

    {
        auto result = parsi_parse(compiled_parser, parsi_stream_t{ .cursor = "Hello", .size = 5 });
        CHECK(!result.is_valid);
        CHECK(std::string_view(result.stream.cursor, result.stream.size) == "Hello");
    }

    {
        auto result = parsi_parse(compiled_parser, parsi_stream_t{ .cursor = "World", .size = 5 });
        CHECK(result.is_valid);
        CHECK(std::string_view(result.stream.cursor, result.stream.size) == "");
    }

    parsi_free_compiled_parser(compiled_parser);
    parsi_free_parser(parser);

    CHECK(ctx.viewed == "HelloWorld");
    CHECK(ctx.parser_call_counter == 2);
    CHECK(ctx.free_context_call_counter == 1);
}

TEST_CASE("c expect string")
{
    char str[] = "hello";

    constexpr auto free_string_fn = [](char* str, size_t size) {
        str[0] = '1';
        str[1] = '2';
        str[2] = '3';
        str[3] = '4';
        str[4] = '5';
    };

    auto parser = parsi_alloc_parser(parsi_expect_string(str, std::strlen(str), free_string_fn));
    auto compiled_parser = parsi_compile(parser);

    {
        auto result = parsi_parse(compiled_parser, parsi_stream_t{ .cursor = "hello", .size = 5 });
        CHECK(result.is_valid);
        CHECK(result.stream.size == 0);
    }
    {
        auto result = parsi_parse(compiled_parser, parsi_stream_t{ .cursor = "Hello", .size = 5 });
        CHECK(!result.is_valid);
        CHECK(std::string_view(result.stream.cursor, result.stream.size) == "Hello");
    }
    {
        auto result = parsi_parse(compiled_parser, parsi_stream_t{ .cursor = "hellO", .size = 5 });
        CHECK(!result.is_valid);
        CHECK(std::string_view(result.stream.cursor, result.stream.size) == "hellO");
    }
    {
        auto result = parsi_parse(compiled_parser, parsi_stream_t{ .cursor = " hello", .size = 6 });
        CHECK(!result.is_valid);
        CHECK(std::string_view(result.stream.cursor, result.stream.size) == " hello");
    }

    parsi_free_compiled_parser(compiled_parser);
    parsi_free_parser(parser);

    CHECK(std::string_view(str, std::strlen(str)) == "12345");
}

TEST_CASE("c repeat")
{
    // TODO
}

TEST_CASE("c sequence")
{
    // TODO
}

TEST_CASE("c anyof")
{
    // TODO
}

TEST_CASE("c optional")
{
    // TODO
}
