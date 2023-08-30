# parsi

[![Build:Linux](https://github.com/cthulhu-irl/parsi/actions/workflows/linux.yml/badge.svg)](https://github.com/cthulhu-irl/parsi/actions?query=workflow%3ALinux)
[![Build:Windows](https://github.com/cthulhu-irl/parsi/actions/workflows/windows.yml/badge.svg)](https://github.com/cthulhu-irl/parsi/actions?query=workflow%3AWindows)
[![codecov](https://codecov.io/gh/cthulhu-irl/parsi/branch/build/coverage/graph/badge.svg?token=U2QVK5MRNW)](https://codecov.io/gh/cthulhu-irl/parsi)

NOTE: this is WIP/prototype and not ready.

parsi is a [parser combinator](https://en.wikipedia.org/wiki/Parser_combinator) library that provides basic parser blocks and a common way to define and combine parsers into more complex parsers.

It can be used as an alternative to complex regex, especially when data needs to be extracted.

The minimum required standard is currently `C++20`.

A simple hex color parser would look like this:
```cpp
struct Color {
    std::uint8_t red = 0;
    std::uint8_t green = 0;
    std::uint8_t blue = 0;
};

constexpr auto color_from_string(std::string_view str) -> std::optional<Color>
{
    constexpr auto convert_hex_digit = [](char digit) -> std::uint8_t {
        if ('0' <= digit && digit <= '9') {
            return digit - '0';
        }

        if ('a' <= digit && digit <= 'f') {
            return 10 + (digit - 'a');
        }

        if ('A' <= digit && digit <= 'F') {
            return 10 + (digit - 'A');
        }

        // unreachable as parser has verified it
        return 0;
    };

    Color color;

    constexpr auto hex_charset = parsi::Charset("0123456789abcdefABCDEF");

    auto parser = parsi::sequence(
        parsi::expect('#'),
        // a color code with 6 hex digits like `#C3A3BB` is equivalent to
        // Color{ .red = 0xC3, .green = 0xA3, .blue = 0xBB }
        parsi::extract(
            parsi::repeat<6, 6>(parsi::expect(hex_charset)),  // min=6 and max=6
            [&](std::string_view str) {
                // str's length is guaranteed to be 6,
                // and characters are guaranteed to be in hex_charset.
                color.red = convert_hex_digit(str[0]) * 16
                          + convert_hex_digit(str[1]);

                color.green = convert_hex_digit(str[2]) * 16
                            + convert_hex_digit(str[3]);

                color.blue = convert_hex_digit(str[4]) * 16
                           + convert_hex_digit(str[5]);
            }
        ),
        parsi::eos()  // end of stream
    );

    if (!parser(str)) {
        return std::nullopt;
    }

    return color;
}
```

### Building Examples

Example executables will be built with `example_` prefix into the `bin/` directory:
```
cmake -S . -B build -DPARSI_EXAMPLES=ON
cmake --build build
```
