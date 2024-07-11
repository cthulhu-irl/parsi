#include "parsi/fixed_string.hpp"

#include <catch2/catch_all.hpp>

TEST_CASE("FixedString")
{
    SECTION("constexpr")
    {
        constexpr auto string = parsi::FixedString("Hello");;
        static_assert(string.size() == sizeof("Hello")-1);
        static_assert(string == "Hello");
        static_assert(string == "Hello\0");
        static_assert(string != "Hell");
        static_assert(string.as_string_view() == std::string_view("Hello"));
    }
}
