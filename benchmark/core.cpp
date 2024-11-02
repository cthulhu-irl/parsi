#include <ctime>
#include <random>
#include <format>
#include <optional>

#include <benchmark/benchmark.h>

#include <parsi/parsi.hpp>
#include <ctre.hpp>

struct Color {
    std::uint8_t red = 0;
    std::uint8_t green = 0;
    std::uint8_t blue = 0;
};

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

    // unreachables as parser has verified it
    return 0;
};

constexpr auto raw_color_from_string(std::string_view str) -> std::optional<Color>
{
    if (str.size() != 7 || str[0] != '#') {
        return std::nullopt;
    }

    bool valid = true;
    for (std::size_t i = 1; i < 7; ++i) {
        const bool is_hex = ('0' <= str[i] && str[i] <= '9') || ('a' <= str[i] && str[i] <= 'f') || ('A' <= str[i] && str[i] <= 'F');
        if (!is_hex) [[unlikely]] {
            return std::nullopt;
        }
    }

    Color color;

    color.red = convert_hex_digit(str[1]) * 16 + convert_hex_digit(str[2]);
    color.green = convert_hex_digit(str[3]) * 16 + convert_hex_digit(str[4]);
    color.blue = convert_hex_digit(str[5]) * 16 + convert_hex_digit(str[6]);

    return color;
}

constexpr auto parsi_color_from_string(std::string_view str) -> std::optional<Color>
{
    Color color;

    constexpr auto color_parser = parsi::sequence(
        parsi::expect('#'),
        // a color code with 6 hex digits like `#C3A3BB` is equivalent to
        // Color{ .red = 0xC3, .green = 0xA3, .blue = 0xBB }
        parsi::repeat<6, 6>(parsi::expect(parsi::CharRange{'0', '9'}, parsi::CharRange{'a', 'f'}, parsi::CharRange{'A', 'F'})),  // min=6 and max=6
        parsi::eos()  // end of stream
    );

    const auto parser = parsi::extract(
        color_parser,
        [&](std::string_view str) {
            // str's length is guaranteed to be 7, with first character being '#',
            // and the rest of them are guaranteed to be in hex_charset.
            color.red = convert_hex_digit(str[1]) * 16 + convert_hex_digit(str[2]);
            color.green = convert_hex_digit(str[3]) * 16 + convert_hex_digit(str[4]);
            color.blue = convert_hex_digit(str[5]) * 16 + convert_hex_digit(str[6]);
        }
    );

    if (!parser(str)) {
        return std::nullopt;
    }

    return color;
}

constexpr auto ctre_color_from_string(std::string_view str) -> std::optional<Color>
{
    constexpr auto matcher = ctre::match<R"(^#([0-9a-fA-F]{2})([0-9a-fA-F]{2})([0-9a-fA-F]{2})$)">;

    auto match = matcher(str);
    if (!match) {
        return std::nullopt;
    }

    auto r_str = match.get<1>().to_view();
    auto g_str = match.get<2>().to_view();
    auto b_str = match.get<3>().to_view();

    Color color;

    color.red = convert_hex_digit(r_str[0]) * 16 + convert_hex_digit(r_str[1]);
    color.green = convert_hex_digit(b_str[0]) * 16 + convert_hex_digit(b_str[1]);
    color.blue = convert_hex_digit(g_str[0]) * 16 + convert_hex_digit(g_str[1]);

    return color;
}

static void bench_color_hex(benchmark::State& state, auto&& parser)
{
    std::srand(std::time(nullptr));

    std::vector<std::string> colors;
    colors.reserve(1'000);
    for (std::size_t count = 0; count < colors.capacity(); ++count)
    {
        colors.push_back(std::format("#{:02x}{:02x}{:02x}", std::rand()%256, std::rand()%256, std::rand()%256));
    }

    std::size_t byte_count = 0;
    for (auto _ : state) {
        for (auto& color_str : colors) {
            auto res = parser(std::string_view(color_str));
            benchmark::DoNotOptimize(res);
        }
        byte_count += colors.size() * 7;
    }

    state.SetBytesProcessed(byte_count);
}
BENCHMARK_CAPTURE(bench_color_hex, raw, raw_color_from_string);
BENCHMARK_CAPTURE(bench_color_hex, parsi, parsi_color_from_string);
BENCHMARK_CAPTURE(bench_color_hex, ctre, ctre_color_from_string);


static void bench_digits(benchmark::State& state, auto&& parser)
{
    std::string str;
    str.reserve(1'000'000);
    for (std::size_t i = 0; i < str.capacity(); ++i) {
        str += '9' - (i % 10);
    }

    std::size_t bytes_count = 0;

    for (auto _ : state) {
        auto res = parser(str.c_str());
        benchmark::DoNotOptimize(res);
        bytes_count += str.size();
    }

    state.SetBytesProcessed(bytes_count);
}
BENCHMARK_CAPTURE(bench_digits, raw, [](parsi::Stream stream) {
    while (stream.size() >= 0 && '0' <= stream.front() && stream.front() <= '9') {
        stream.advance(1);
    }
    return parsi::Result{stream, true};
});
BENCHMARK_CAPTURE(bench_digits, parsi, parsi::repeat(parsi::expect(parsi::CharRange{'0', '9'})));
BENCHMARK_CAPTURE(bench_digits, ctre, ctre::match<R"(^[0-9]*)">);


constexpr auto optional_whitespaces = parsi::repeat(parsi::expect(parsi::Charset(" \n\t")));
constexpr auto expect_digits = parsi::repeat<1>(parsi::expect(parsi::CharRange{'0', '9'}));
constexpr auto expect_identifer = []() {
    return parsi::sequence(
        parsi::expect(parsi::CharRange{'a', 'z'}, parsi::CharRange{'A', 'Z'}, parsi::CharRange{'_', '_'}),
        parsi::repeat(parsi::expect(parsi::CharRange{'a', 'z'}, parsi::CharRange{'A', 'Z'}, parsi::CharRange{'0', '9'}, parsi::CharRange{'_', '_'}))
    );
}();
constexpr auto expect_item = parsi::anyof(
    parsi::extract(expect_digits, [](std::string_view token) { benchmark::DoNotOptimize(token); }),
    parsi::extract(expect_identifer, [](std::string_view token) { benchmark::DoNotOptimize(token); })
);
constexpr auto parsi_parser = parsi::sequence(
    parsi::expect('['),
    optional_whitespaces,
    parsi::optional(parsi::sequence(
        expect_item,
        parsi::repeat(parsi::sequence(
            optional_whitespaces,
            parsi::expect(','),
            optional_whitespaces,
            expect_item,
            optional_whitespaces
        ))
    )),
    parsi::expect(']'),
    parsi::eos()
);

constexpr auto ctre_parser = ctre::match<R"(^\[\s*(([0-9]+|[A-Za-z_]+[A-Za-z0-9_]*)\s*(,\s*([0-9]+|[A-Za-z_]+[A-Za-z0-9_]*)\s*)*)?\]$)">;

static void bench_many_items(benchmark::State& state, auto&& parser)
{
    auto str = [&state]() {
        std::string ret;
        ret.reserve(state.range(0));
        ret += '[';
        for (std::size_t i=0; i < state.range(0); ++i) {
            ret += "1234567890,   test,";
        }
        ret += "1";
        ret += ']';
        return ret;
    }();

    std::size_t bytes_count = 0;

    for (auto _ : state) {
        auto res = parser(str.c_str());
        benchmark::DoNotOptimize(&res);
        bytes_count += str.size();
    }

    state.SetBytesProcessed(bytes_count);
}
BENCHMARK_CAPTURE(bench_many_items, parsi, parsi_parser)->RangeMultiplier(10)->Range(100, 10'000'000);
BENCHMARK_CAPTURE(bench_many_items, ctre, ctre_parser)->RangeMultiplier(10)->Range(100, 10'000'000);

BENCHMARK_MAIN();
