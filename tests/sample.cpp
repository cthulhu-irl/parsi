#include <catch2/catch_all.hpp>

#include "parsi/parsi.hpp"

namespace pr = parsi;

TEST_CASE("basic usages") {
    // valid cases
    CHECK(pr::expect("")(""));
    CHECK(pr::expect("exactly")("exactly"));
    CHECK(pr::expect("starting")("starting and going"));

    CHECK(pr::expect('a')("abcd"));

    CHECK(pr::expect(pr::Charset("abcd"))("a"));
    CHECK(pr::expect(pr::Charset("abcd"))("b"));
    CHECK(pr::expect(pr::Charset("abcd"))("c"));
    CHECK(pr::expect(pr::Charset("abcd"))("d"));

    CHECK(pr::optional(pr::expect("not empty"))(""));

    CHECK(pr::sequence(pr::expect("Hello"), pr::expect("World"))("HelloWorld"));
    CHECK(pr::sequence(pr::expect("Hello"), pr::optional(pr::expect("World")))("HelloWord"));
    CHECK(pr::sequence(pr::expect("Hello"), pr::optional(pr::expect("World")))("HelloWorld"));

    CHECK(pr::anyof(pr::expect("test"), pr::expect("best"))("best"));
    CHECK(pr::anyof(pr::expect('a'), pr::expect('b'))("best"));

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

    CHECK(pr::extract(pr::expect("test"), [](std::string_view str) {
        CHECK(str == "test");
    })("test"));

    // invalid cases
    CHECK(not pr::expect("fury")("ffury"));
    CHECK(not pr::expect("wrong")("not empty"));
    CHECK(not pr::expect("not empty")(""));

    CHECK(not pr::expect('a')("bcd"));

    CHECK(not pr::expect(pr::Charset("abcd"))("e"));
    CHECK(not pr::expect(pr::Charset("abcd"))("f"));
    CHECK(not pr::expect(pr::Charset("abcd"))("g"));
    CHECK(not pr::expect(pr::Charset("abcd"))("h"));

    CHECK(not pr::sequence(pr::expect("Hello"), pr::expect("World"))("HelloWord"));

    CHECK(not pr::anyof(pr::expect("test"), pr::expect("best"))("rest"));

    CHECK(not pr::repeat<1>(pr::expect("at least once"))("nope"));
    CHECK(not pr::repeat<1, 1>(pr::expect("at least once"))("nope"));

    CHECK(not pr::extract(pr::expect("test"), [](std::string_view str) {
        return str == "not test";
    })("test"));
}

TEST_CASE("complex") {
    auto parser = pr::sequence(
        pr::expect("{"),
        pr::repeat(pr::expect(" ")),
        pr::extract(pr::expect("Hello!"), [](std::string_view str) {
            return str == "Hello!";
        }),
        pr::repeat(pr::expect(" ")),
        pr::expect("}")
    );

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

TEST_CASE("Bitset") {
    SECTION("constexpr") {
        constexpr auto bitset = []() {
            pr::internal::Bitset<125> ret;
            ret.set(0, true);
            ret.set(1, true);
            ret.set(84, true);
            ret.set(42, true);
            ret.set(127, true);
            return ret;
        }();

        CHECK(bitset.test(0));
        CHECK(bitset.test(1));
        CHECK(bitset.test(84));
        CHECK(bitset.test(42));

        CHECK(not bitset.test(64));
        CHECK(not bitset.test(128));
        CHECK(not bitset.test(125));
    }

    SECTION("set false") {
        pr::internal::Bitset<64> bitset;

        bitset.set(42, true);
        CHECK(bitset.test(42));

        bitset.set(42, false);
        CHECK(not bitset.test(42));
    }
}
