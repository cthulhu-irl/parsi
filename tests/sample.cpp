#include <catch2/catch_all.hpp>

#include "parsi/parsi.hpp"

namespace pr = parsi;

TEST_CASE("base") {
    // valid cases
    CHECK(pr::expect("")(""));
    CHECK(pr::expect("exactly")("exactly"));
    CHECK(pr::expect("starting")("starting and going"));

    CHECK(pr::optional(pr::expect("not empty"))(""));

    CHECK(pr::sequence(pr::expect("Hello"), pr::expect("World"))("HelloWorld"));
    CHECK(pr::sequence(pr::expect("Hello"), pr::optional(pr::expect("World")))("HelloWord"));

    // invalid cases
    CHECK(not pr::expect("fury")("ffury"));
    CHECK(not pr::expect("wrong")("not empty"));
    CHECK(not pr::expect("not empty")(""));

    CHECK(not pr::sequence(pr::expect("Hello"), pr::expect("World"))("HelloWord"));
}
