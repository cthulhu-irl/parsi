#include <benchmark/benchmark.h>

#include <parsi/parsi.hpp>

constexpr auto expect_whitespaces = parsi::repeat(parsi::expect(parsi::Charset(" \n\t")));
constexpr auto expect_digits = parsi::repeat<1>(parsi::expect(parsi::Charset("0123456789")));
constexpr auto parser = parsi::sequence(
    parsi::expect('['),
    expect_whitespaces,
    parsi::optional(parsi::sequence(
        expect_digits,
        parsi::repeat(parsi::sequence(
            expect_whitespaces,
            parsi::expect(','),
            expect_whitespaces,
            expect_digits,
            expect_whitespaces
        ))
    )),
    expect_whitespaces,
    parsi::expect(']')
);


static void empty(benchmark::State& state)
{
    for (auto _ : state) {
        auto res = parser("");
        benchmark::DoNotOptimize(res);
    }
}
BENCHMARK(empty);

static void empty_list(benchmark::State& state)
{
    for (auto _ : state) {
        auto res = parser("[]");
        benchmark::DoNotOptimize(res);
    }
}
BENCHMARK(empty_list);

static void single_item(benchmark::State& state)
{
    for (auto _ : state) {
        auto res = parser("[123456]");
        benchmark::DoNotOptimize(res);
    }
}
BENCHMARK(single_item);

static void multiple_items(benchmark::State& state)
{
    for (auto _ : state) {
        auto res = parser("[123456,243,667767,81,0]");
        benchmark::DoNotOptimize(res);
    }
}
BENCHMARK(multiple_items);

static void early_failure(benchmark::State& state)
{
    for (auto _ : state) {
        auto res = parser("[test,2]");
        benchmark::DoNotOptimize(res);
    }
}
BENCHMARK(early_failure);

static void late_failure(benchmark::State& state)
{
    for (auto _ : state) {
        auto res = parser("[2,3,4,5,test]");
        benchmark::DoNotOptimize(res);
    }
}
BENCHMARK(late_failure);

static void many_items(benchmark::State& state)
{
    std::string str;
    str.reserve(20'000 * 15);

    str += '[';
    for (std::size_t i=0; i < 20'000; ++i) {
        str += "1234567890   ,";
    }
    str += "1234565  ";
    str += ']';

    for (auto _ : state) {
        auto res = parser(str.c_str());
        benchmark::DoNotOptimize(&res);
    }
}
BENCHMARK(many_items);

BENCHMARK_MAIN();
