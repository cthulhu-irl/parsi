#include <catch2/catch_all.hpp>

#include "parsi/parsi.hpp"

namespace pr = parsi;

TEST_CASE("expect")
{
    CHECK(pr::expect("abcd")("abcd"));
    CHECK(pr::expect(pr::FixedString("abcd"))("abcd"));

    CHECK(pr::expect("")(""));
    CHECK(pr::expect("exactly")("exactly"));
    CHECK(pr::expect("starting")("starting and going"));

    CHECK(not pr::expect("fury")("ffury"));
    CHECK(not pr::expect("wrong")("not empty"));
    CHECK(not pr::expect("not empty")(""));

    CHECK(pr::expect('a')("abcd"));
    CHECK(not pr::expect('a')("bcd"));

    CHECK(pr::expect(pr::Charset("abcd"))("a"));
    CHECK(pr::expect(pr::Charset("abcd"))("b"));
    CHECK(pr::expect(pr::Charset("abcd"))("c"));
    CHECK(pr::expect(pr::Charset("abcd"))("d"));

    CHECK(not pr::expect(pr::Charset("abcd"))("e"));
    CHECK(not pr::expect(pr::Charset("abcd"))("f"));
    CHECK(not pr::expect(pr::Charset("abcd"))("g"));
    CHECK(not pr::expect(pr::Charset("abcd"))("h"));

    CHECK(not pr::expect("test")(""));
    CHECK(not pr::expect('a')(""));
    CHECK(not pr::expect(pr::Charset("abcd"))(""));
}

TEST_CASE("expect dynamic string")
{
    std::string str = "abcd";
    CHECK(pr::expect(str)("abcd"));

    CHECK(pr::expect(std::string("abcd"))("abcd"));

    CHECK(not pr::expect(std::string("abcd"))("abc"));
}

TEST_CASE("expect_not")
{
    CHECK(pr::expect_not('a')("ba"));
    CHECK(not pr::expect_not('a')("ab"));

    CHECK(pr::expect_not(pr::Charset("abcd"))("A"));
    CHECK(pr::expect_not(pr::Charset("abcd"))("@"));
    CHECK(not pr::expect_not(pr::Charset("abcd"))("a"));
    CHECK(not pr::expect_not(pr::Charset("abcd"))("b"));
    CHECK(not pr::expect_not(pr::Charset("abcd"))("c"));
    CHECK(not pr::expect_not(pr::Charset("abcd"))("d"));

    CHECK(not pr::expect_not('a')(""));
}

TEST_CASE("eos")
{
    CHECK(pr::eos()(""));

    CHECK(not pr::eos()("test"));
}

TEST_CASE("optional")
{
    CHECK(pr::optional(pr::expect("not empty"))(""));
}

TEST_CASE("sequence")
{
    CHECK(pr::sequence(pr::expect("Hello"), pr::expect("World"))("HelloWorld"));
    CHECK(pr::sequence(pr::expect("Hello"), pr::optional(pr::expect("World")))("HelloWord"));
    CHECK(pr::sequence(pr::expect("Hello"), pr::optional(pr::expect("World")))("HelloWorld"));

    CHECK(not pr::sequence(pr::expect("Hello"), pr::expect("World"))("HelloWord"));
}

TEST_CASE("anyof")
{
    CHECK(pr::anyof(pr::expect("test"), pr::expect("best"))("best"));
    CHECK(pr::anyof(pr::expect('a'), pr::expect('b'))("best"));

    CHECK(not pr::anyof(pr::expect("test"), pr::expect("best"))("rest"));
}

TEST_CASE("repeat")
{
    CHECK(pr::repeat(pr::expect(" "))("a b"));
    CHECK(pr::repeat(pr::expect("none"))("nope"));
    CHECK(pr::repeat(pr::expect("once"))("once"));
    CHECK(pr::repeat(pr::expect("more "))("more more "));
    CHECK(pr::repeat<1>(pr::expect("at least once"))("at least once at least once"));
    CHECK(pr::repeat<1>(pr::expect("more "))("more more "));
    CHECK(pr::repeat<0, 0>(pr::expect("match"))("match"));
    CHECK(pr::repeat<0, 0>(pr::expect("match"))("yep"));
    CHECK(pr::repeat<0, 0>(pr::expect("match"))("match"));
    CHECK(pr::repeat<1, 1>(pr::expect("exactly once"))("exactly once"));

    CHECK(not pr::repeat<1>(pr::expect("at least once"))("nope"));
    CHECK(not pr::repeat<1, 1>(pr::expect("at least once"))("nope"));
}

TEST_CASE("extract")
{
    CHECK(pr::extract(pr::expect("test"),
                      [](std::string_view str) { CHECK(str == "test"); })("test"));

    CHECK(not pr::extract(pr::expect("test"),
                          [](std::string_view str) { return str == "not test"; })("test"));
}

TEST_CASE("complex composition")
{
    auto parser = pr::sequence(
        pr::expect("{"), pr::repeat(pr::expect(" ")),
        pr::extract(pr::expect("Hello!"), [](std::string_view str) { return str == "Hello!"; }),
        pr::repeat(pr::expect(" ")), pr::expect("}"));

    CHECK(parser("{Hello!}"));
    CHECK(parser("{Hello! }"));
    CHECK(parser("{ Hello!}"));
    CHECK(parser("{ Hello! }"));
    CHECK(parser("{   Hello!   }"));

    CHECK(not parser("Hello!"));
    CHECK(not parser("{Hello!"));
    CHECK(not parser("Hello!}"));
    CHECK(not parser("{Hell!}"));
    CHECK(not parser(" { Hello! } "));
}
