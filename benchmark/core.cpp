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
    constexpr auto hex_charset = parsi::Charset("0123456789abcdefABCDEF");

    if (str.size() != 7 || str[0] != '#') {
        return std::nullopt;
    }

    bool valid = true;
    for (std::size_t i = 1; i < 7; ++i) {
        valid = valid && hex_charset.contains(str[i]);
    }

    if (!valid) [[unlikely]] {
        return std::nullopt;
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

    constexpr auto hex_charset = parsi::Charset("0123456789abcdefABCDEF");
    constexpr auto color_parser = parsi::sequence(
        parsi::expect('#'),
        // a color code with 6 hex digits like `#C3A3BB` is equivalent to
        // Color{ .red = 0xC3, .green = 0xA3, .blue = 0xBB }
        parsi::repeat<6, 6>(parsi::expect(hex_charset)),  // min=6 and max=6
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

    auto rstr = match.get<1>().to_view();
    auto gstr = match.get<2>().to_view();
    auto bstr = match.get<3>().to_view();

    Color color;

    color.red = convert_hex_digit(rstr[0]) * 16 + convert_hex_digit(rstr[1]);
    color.green = convert_hex_digit(bstr[0]) * 16 + convert_hex_digit(bstr[1]);
    color.blue = convert_hex_digit(gstr[0]) * 16 + convert_hex_digit(gstr[1]);

    return color;
}

static void bench_color_hex(benchmark::State& state, auto&& parser)
{
    std::srand(std::time(nullptr));

    auto str = std::format("#{:02x}{:02x}{:02x}", std::rand()%256, std::rand()%256, std::rand()%256);
    const std::string_view strview = str;

    for (auto _ : state) {
        auto res = parser(strview);
        benchmark::DoNotOptimize(res);
    }
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

        state.PauseTiming();
        bytes_count += str.size();
        state.ResumeTiming();
    }

    state.SetBytesProcessed(bytes_count);
}
BENCHMARK_CAPTURE(bench_digits, raw, [digits_charset=parsi::Charset("0123456789")](parsi::Stream stream) {
    std::size_t index = 0;

    while (index < stream.size() && digits_charset.contains(stream.at(index))) {
        ++index;
    }

    return parsi::Result{stream.advanced(index), true};
});
BENCHMARK_CAPTURE(bench_digits, parsi, parsi::repeat(parsi::expect(parsi::Charset("0123456789"))));
BENCHMARK_CAPTURE(bench_digits, ctre, ctre::match<R"(^[0-9]*)">);


constexpr auto optional_whitespaces = parsi::repeat(parsi::expect(parsi::Charset(" \n\t")));
constexpr auto expect_digits = parsi::repeat<1>(parsi::expect(parsi::Charset("0123456789")));
constexpr auto expect_identifer = []() {
    constexpr auto first_charset = parsi::Charset("_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
    constexpr auto rest_charset = parsi::Charset("_abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789");
    return parsi::sequence(
        parsi::expect('"'),
        parsi::expect(first_charset),
        parsi::repeat(parsi::expect(rest_charset)),
        parsi::expect('"')
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


static void bench_empty_string(benchmark::State& state, auto&& parser)
{
    for (auto _ : state) {
        auto res = parser("");
        benchmark::DoNotOptimize(res);
    }
}
BENCHMARK_CAPTURE(bench_empty_string, parsi, parsi_parser);
BENCHMARK_CAPTURE(bench_empty_string, ctre, ctre_parser);


static void bench_empty_list(benchmark::State& state, auto&& parser)
{
    for (auto _ : state) {
        auto res = parser("[]");
        benchmark::DoNotOptimize(res);
    }
}
BENCHMARK_CAPTURE(bench_empty_list, parsi, parsi_parser);
BENCHMARK_CAPTURE(bench_empty_list, ctre, ctre_parser);


static void bench_early_failure(benchmark::State& state, auto&& parser)
{
    for (auto _ : state) {
        auto res = parser("['test',2]");
        benchmark::DoNotOptimize(res);
    }
}
BENCHMARK_CAPTURE(bench_early_failure, parsi, parsi_parser);
BENCHMARK_CAPTURE(bench_early_failure, ctre, ctre_parser);


static void bench_late_failure(benchmark::State& state, auto&& parser)
{
    for (auto _ : state) {
        auto res = parser("[2,3,4,5,test,'rest']");
        benchmark::DoNotOptimize(res);
    }
}
BENCHMARK_CAPTURE(bench_late_failure, parsi, parsi_parser);
BENCHMARK_CAPTURE(bench_late_failure, ctre, ctre_parser);

static void bench_many_items(benchmark::State& state, auto&& parser)
{
    auto str = [&state]() {
        std::string ret;
        ret.reserve(state.range(0) * 20);
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

        state.PauseTiming();
        bytes_count += str.size();
        state.ResumeTiming();
    }

    state.SetBytesProcessed(bytes_count);
}
BENCHMARK_CAPTURE(bench_many_items, parsi, parsi_parser)->RangeMultiplier(10)->Range(1, 100'000'000);
BENCHMARK_CAPTURE(bench_many_items, ctre, ctre_parser)->RangeMultiplier(10)->Range(1, 100'000'000);

BENCHMARK_MAIN();
