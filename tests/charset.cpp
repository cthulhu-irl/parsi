#include "parsi/charset.hpp"

#include <catch2/catch_all.hpp>

TEST_CASE("Charset-ASCII")
{
    const auto digits = parsi::Charset("0123456789");

    CHECK(digits.contains('0'));
    CHECK(digits.contains('5'));
    CHECK(digits.contains('9'));

    CHECK(not digits.contains('A'));
    CHECK(not digits.contains('\0'));
}

TEST_CASE("Charset-Binary")
{
    const auto byteset = parsi::Charset({0, 1, 2, 3, 4, 5, 255});

    CHECK(byteset.contains(0));
    CHECK(byteset.contains(2));
    CHECK(byteset.contains(255));

    CHECK(not byteset.contains(124));
    CHECK(not byteset.contains(254));
}

TEST_CASE("Charset-CharArray")
{
    SECTION("signed char")
    {
        const auto char_array = std::array<char, 3>{'A', '\0', 'B'};
        const auto charset = parsi::Charset(char_array.data(), char_array.size());

        CHECK(charset.contains('A'));
        CHECK(charset.contains('\0'));
        CHECK(charset.contains('B'));
    }

    SECTION("unsigned char")
    {
        const auto char_array = std::array<char, 3>{'A', '\0', 'B'};
        const auto charset = parsi::Charset(char_array.data(), char_array.size());

        CHECK(charset.contains('A'));
        CHECK(charset.contains('\0'));
        CHECK(charset.contains('B'));
    }
}
