#include <benchmark/benchmark.h>

static void sample(benchmark::State& state)
{
  for (auto _ : state) {
    // nothing
  }
}
BENCHMARK(sample);

BENCHMARK_MAIN();

