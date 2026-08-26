# CallMeMaybe Runtime Reflection Benchmarks

This directory contains reproducible microbenchmarks for the runtime cost of invoking reflected callables.

The baseline suite intentionally has no third-party benchmark dependency. It compares the same integer operation through direct calls, function pointers, virtual dispatch, `std::variant`, a dispatch table, typed CallMeMaybe invocation, and raw `reflect_invoke`.

## Build

```sh
cmake -S . -B build-bench -DCMAKE_BUILD_TYPE=Release -DCMM_BUILD_BENCHMARKS=ON -DCMM_BUILD_TESTS=OFF -DCMM_BUILD_EXAMPLES=OFF
cmake --build build-bench --target cmm_runtime_bench -j
```

## Run

```sh
./build-bench/benchmarks/cmm_runtime_bench --iterations 1000000 --batch 4096 --json benchmark.json
python3 benchmarks/summarize.py benchmark.json
```

The JSON file records the compiler, iteration count, batching, checksums, mean nanoseconds per operation, and p50/p95/p99 batch latency. All dispatch paths must produce an identical checksum or the benchmark fails.

## Methodology

Use a quiet machine, Release mode, fixed CPU power settings where possible, and multiple independent runs. CI should be used only as a correctness smoke test; timing thresholds do not belong in shared runners.

For publishable comparisons, record exact compiler revision, standard library revision, CPU, OS, build flags, commit SHA, and whether the binary was stripped. Third-party adapters for RTTR and metapp should be reported separately from the dependency-free core so changes in their build systems do not invalidate the baseline.
