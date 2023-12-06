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

    SECTION("equality with same size")
    {
        parsi::internal::Bitset<64> bitset_a;
        parsi::internal::Bitset<64> bitset_b;

        CHECK(bitset_a == bitset_b);

        bitset_a.set(0, true);
        bitset_a.set(1, true);
        bitset_a.set(42, true);

        bitset_b.set(0, true);
        bitset_b.set(1, true);
        bitset_b.set(42, true);

        CHECK(bitset_a == bitset_b);

        bitset_a.set(2, true);
        CHECK(bitset_a != bitset_b);
    }

    SECTION("equality with different size")
    {
        CHECK(parsi::internal::Bitset<64>{} != parsi::internal::Bitset<128>{});
    }

    SECTION("joined returns max size")
    {
        const auto bitset_64 = parsi::internal::Bitset<64>{};
        const auto bitset_42 = parsi::internal::Bitset<42>{};

        CHECK(bitset_42.joined(bitset_42) == bitset_42);
        CHECK(bitset_42.joined(bitset_64) == bitset_64);
        CHECK(bitset_64.joined(bitset_42) == bitset_64);
        CHECK(bitset_64.joined(bitset_64) == bitset_64);
    }

    SECTION("joined overlaps")
    {
        auto bitset_a = parsi::internal::Bitset<64>{};
        auto bitset_b = parsi::internal::Bitset<64>{};

        bitset_a.set(0, true);
        bitset_a.set(42, true);

        bitset_b.set(1, true);
        bitset_b.set(42, true);

        const auto joined_bitset = bitset_a.joined(bitset_b);

        CHECK(joined_bitset.test(0));
        CHECK(joined_bitset.test(1));
        CHECK(joined_bitset.test(42));

        CHECK(joined_bitset == bitset_b.joined(bitset_a));
    }

    SECTION("negated bits")
    {
        auto bitset = parsi::internal::Bitset<64>{};
        bitset.set(0, true);
        bitset.set(1, true);
        bitset.set(42, true);

        auto bitset_negated = bitset.negated();

        CHECK(!bitset_negated.test(0));
        CHECK(!bitset_negated.test(1));
        CHECK(!bitset_negated.test(42));

        CHECK(bitset_negated.test(2));
        CHECK(bitset_negated.test(3));
        CHECK(bitset_negated.test(5));
        CHECK(bitset_negated.test(10));
        CHECK(bitset_negated.test(43));
        CHECK(bitset_negated.test(53));
        CHECK(bitset_negated.test(63));
    }

    SECTION("negate in-place modifier")
    {
        auto bitset = parsi::internal::Bitset<64>{};
        bitset.set(0, true);
        bitset.set(1, true);
        bitset.set(42, true);

        auto bitset_copy_negate = bitset;
        bitset_copy_negate.negate();

        CHECK(bitset.negated() == bitset_copy_negate);
    }
}
