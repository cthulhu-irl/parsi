#include <catch2/catch_all.hpp>

#include "parsi/parsi.hpp"

TEST_CASE("RTParser")
{
    parsi::RTParser parser = parsi::sequence(
        parsi::expect("Hello"),
        parsi::optional(parsi::anyof(parsi::expect(" World"), parsi::expect(" Dear"))),
        parsi::eos()
    );

    CHECK(parser("Hello"));
    CHECK(parser("Hello World"));
    CHECK(parser("Hello Dear"));

    CHECK(not parser("Hello "));
    CHECK(not parser("Hello Parser"));
}
