#include <optional>
#include <tuple>

#include "parsi/regex.hpp"

#include <catch2/catch_all.hpp>

TEST_CASE("regex_match")
{
    SECTION("exact")
    {
        using first_regex = parsi::regex_match<"test">;
        CHECK(first_regex("test"));
        CHECK(first_regex("atest"));
        CHECK(first_regex("testa"));
        CHECK(first_regex("atesta"));

        CHECK(not first_regex("rest"));
    }

    SECTION("start-end")
    {
        using begin_regex = parsi::regex_match<"^test">;
        CHECK(begin_regex("test"));
        CHECK(begin_regex("testrest"));
        CHECK(not begin_regex("ttest"));
        
        using end_regex = parsi::regex_match<"test$">;
        CHECK(end_regex("test"));
        CHECK(end_regex("atest"));
        CHECK(not end_regex("tested"));

        using begin_end_regex = parsi::regex_match<"^test$">;
        CHECK(begin_end_regex("test"));
        CHECK(not begin_end_regex("ttest"));
        CHECK(not begin_end_regex("testt"));
        CHECK(not begin_end_regex("ttestt"));
    }

    SECTION("dot-wildcard")
    {
        using first_regex = parsi::regex_match<".es.">;
        CHECK(first_regex("test"));
        CHECK(first_regex("best"));
        CHECK(first_regex("resp"));
        CHECK(first_regex("chest"));
        CHECK(not first_regex("estt"));
        CHECK(not first_regex("tees"));
        CHECK(not first_regex("check"));
    }

    SECTION("repeat")
    {
        using first_regex = parsi::regex_match<"a+b*">;
        CHECK(first_regex("a"));
        CHECK(first_regex("ab"));
        CHECK(first_regex("aaaaabbbb"));
        CHECK(not first_regex(""));
        CHECK(not first_regex("b"));

        static_assert(not requires { parsi::regex_match<"*"> });
        static_assert(not requires { parsi::regex_match<"+"> });
        static_assert(not requires { parsi::regex_match<"+b*"> });
    }

    SECTION("charset")
    {
        using hex_regex = parsi::regex_match<"[a-fA-F0-9]">;

        CHECK(hex_regex("0"));
        CHECK(hex_regex("1"));
        CHECK(hex_regex("2"));
        CHECK(hex_regex("3"));
        CHECK(hex_regex("4"));
        CHECK(hex_regex("5"));
        CHECK(hex_regex("6"));
        CHECK(hex_regex("7"));
        CHECK(hex_regex("8"));
        CHECK(hex_regex("9"));

        CHECK(hex_regex("a"));
        CHECK(hex_regex("b"));
        CHECK(hex_regex("d"));
        CHECK(hex_regex("f"));
        CHECK(not hex_regex("i"));
        CHECK(not hex_regex("s"));
        CHECK(not hex_regex("u"));
        CHECK(not hex_regex("z"));

        CHECK(hex_regex("A"));
        CHECK(hex_regex("B"));
        CHECK(hex_regex("D"));
        CHECK(hex_regex("F"));
        CHECK(not hex_regex("H"));
        CHECK(not hex_regex("K"));
        CHECK(not hex_regex("X"));
        CHECK(not hex_regex("Y"));
        CHECK(not hex_regex("Z"));

        CHECK(not hex_regex("/<,>!@#%&-_=\\+\\*\\(\\)\\[\\]\\?\\.\\\\"));
    }

    SECTION("subset")
    {
        using first_regex = parsi::regex_match<"">;
    }

    SECTION("complex")
    {
        using first_regex = parsi::regex_match<"">;
    }
}

TEST_CASE("regex_extract")
{
    SECTION("simple")
    {
        using first_regex = parsi::regex_extract<"([a-z]+),([0-9]+)">;
        CHECK(first_regex("test,1234") == std::make_tuple("test", "1234"));
        CHECK(first_regex(",1234") == std::make_tuple(std::nullopt, std::nullopt));
        CHECK(first_regex("test,") == std::make_tuple(std::nullopt, std::nullopt));
    }

    SECTION("nested")
    {
        using first_regex = parsi::regex_extract<"([a-z]+)\\s*=\\s*((0x[a-f0-9])|(0o[0-7]+)|([1-9][0-9]*),\\s*([a-z]+))">;
        CHECK(first_regex("test = (1, a)") == std::make_tuple("test", std::make_tuple("1", "a")));
        CHECK(first_regex("rest = (0xdeadbeef, hex)") == std::make_tuple("rest", std::make_tuple("0xdeadbeef", "hex")));
        CHECK(first_regex("heaven = (0o777, oct)") == std::make_tuple("heaven", std::make_tuple("0o777", "oct")));
        CHECK(first_regex("notso = (0b1101, bin)") == std::make_tuple("notso", std::make_tuple(std::nullopt, "bin")));
        CHECK(first_regex("a_b = (0b1101, yikes)") == std::make_tuple(std::nullopt, std::make_tuple(std::nullopt, "yikes")));
    }

    SECTION("repeated")
    {
        using first_regex = parsi::regex_extract<"((0x[a-f0-9]+)\\s*,\\s*)*(0x[a-f0-9]+)">;
        CHECK(first_regex("0xdeadbeef,0x1337,0x42") == std::make_tuple(std::array{"0xdeadbeef", "0x1337"}, "0x42"));
        CHECK(first_regex("0xdeadroast,0x1337,0x42") == std::make_tuple(std::array{nullptr, "0x1337"}, "0x42"));
        CHECK(first_regex("0xdeadbeef,0xL33T,0x42") == std::make_tuple(std::array{"0xdeadbeef", nullptr}, "0x42"));
        CHECK(first_regex("0x42") == std::make_tuple("0x42"));
    }
}
