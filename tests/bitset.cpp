#include "parsi/internal/bitset.hpp"

#include <catch2/catch_all.hpp>

TEST_CASE("Bitset")
{
    SECTION("constexpr")
    {
        constexpr auto bitset = []() {
            parsi::internal::Bitset<125> ret;
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

    SECTION("set false")
    {
        parsi::internal::Bitset<64> bitset;

        bitset.set(42, true);
        CHECK(bitset.test(42));

        bitset.set(42, false);
        CHECK(not bitset.test(42));
    }
}
