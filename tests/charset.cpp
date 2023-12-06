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

TEST_CASE("Charset-Combination")
{
    constexpr auto numeric = parsi::Charset("0123456789");
    constexpr auto lowercase = parsi::Charset("abcdefghijklmnopqrstuvwxyz");
    constexpr auto uppercase = parsi::Charset("ABCDEFGHIJKLMNOPQRSTUVWXYZ");

    constexpr auto alphabetic = parsi::Charset("abcdefghijklmnopqrstuvwxyz" "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
    constexpr auto alphanumeric = parsi::Charset("0123456789" "abcdefghijklmnopqrstuvwxyz" "ABCDEFGHIJKLMNOPQRSTUVWXYZ");

    SECTION("joined monoid identity")
    {
        CHECK(parsi::Charset().joined(parsi::Charset()) == parsi::Charset());
        CHECK(parsi::Charset().joined(numeric) == numeric);
    }

    SECTION("joined overlap")
    {
        CHECK(numeric.joined(numeric) == numeric);
        CHECK(lowercase.joined(uppercase) == alphabetic);
        CHECK(numeric.joined(lowercase).joined(uppercase) == alphanumeric);
    }
}

TEST_CASE("Charset-Opposite")
{
    // constexpr auto non_numeric = parsi::Charset("0123456789").opposite();
    
    auto numeric = parsi::Charset("0123456789");
    auto non_numeric = numeric.opposite();

    CHECK(numeric != non_numeric);

    CHECK(!non_numeric.contains('0'));
    CHECK(!non_numeric.contains('1'));
    CHECK(!non_numeric.contains('2'));
    CHECK(!non_numeric.contains('3'));
    CHECK(!non_numeric.contains('4'));
    CHECK(!non_numeric.contains('5'));
    CHECK(!non_numeric.contains('6'));
    CHECK(!non_numeric.contains('7'));
    CHECK(!non_numeric.contains('8'));
    CHECK(!non_numeric.contains('9'));

    CHECK(non_numeric.contains('\0'));
    CHECK(non_numeric.contains('A'));
    CHECK(non_numeric.contains('@'));
    CHECK(non_numeric.contains('\n'));
    CHECK(non_numeric.contains('\r'));
    CHECK(non_numeric.contains('\255'));
}
