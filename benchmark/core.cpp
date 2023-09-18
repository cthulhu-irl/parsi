#include <benchmark/benchmark.h>

#include <parsi/parsi.hpp>
#include <ctre.hpp>

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
    std::string str;
    str.reserve(state.range(0) * 15);

    str += '[';
    for (std::size_t i=0; i < state.range(0); ++i) {
        str += "1234567890, best   ,";
    }
    str += "1234565";
    str += ']';

    for (auto _ : state) {
        auto res = parser(str.c_str());
        benchmark::DoNotOptimize(&res);
    }
}
BENCHMARK_CAPTURE(bench_many_items, parsi, parsi_parser)->RangeMultiplier(10)->Range(1, 10'000);
BENCHMARK_CAPTURE(bench_many_items, ctre, ctre_parser)->RangeMultiplier(10)->Range(1, 10'000);

BENCHMARK_MAIN();
