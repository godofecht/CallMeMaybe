# Reflection Differential Fuzzer

This harness generates deterministic, legal C++26 programs that stress runtime-reflection identity and metadata invariants, compiles the same corpus with one or more reflection-capable compilers, runs each binary, and compares normalized semantic fingerprints.

The first corpus family targets bugs that are disproportionately likely in a runtime registry built from compile-time reflection:

- same-named members in distinct declaring classes
- different physical layouts for semantically similar classes
- parameter-position identity for repeated parameter types
- parent metadata for members, parameters and enumerators
- namespace-qualified type identity
- enum metadata including high-bit unsigned values

Each generated program also checks invariants internally against `offsetof` and the expected declaring entity before cross-compiler comparison.

## GCC smoke run

```sh
python3 tools/reflection_fuzz/run.py \
  --cases 32 \
  --seed 1 \
  --compiler 'gcc=g++-16 -std=c++26 -freflection'
```

## GCC vs Clang/P2996

```sh
python3 tools/reflection_fuzz/run.py \
  --cases 100 \
  --seed 1 \
  --compiler 'gcc=g++-16 -std=c++26 -freflection' \
  --compiler 'clang=/path/to/clang++ -std=c++26 -freflection-latest -stdlib=libc++'
```

Results are written to `reflection-fuzz-results/`: generated source, compiler stdout/stderr, one fingerprint per compiler, and a manifest recording the exact commands and disagreement set. A disagreement exits with status 3 so a corpus can be minimized and retained as a regression test rather than disappearing into a fuzz log.

## Minimize a disagreement

`minimize.py` binary-searches the generated prefix and retains the smallest prefix that still produces a semantic fingerprint disagreement. Pass the same corpus family that produced the disagreement so minimization cannot silently switch from `shapes` back to the default `core` generator:

```sh
python3 tools/reflection_fuzz/minimize.py \
  --family shapes \
  --max-cases 1000 \
  --seed 7 \
  --compiler 'gcc=g++-16 -std=c++26 -freflection' \
  --compiler 'clang=/path/to/clang++ -std=c++26 -freflection-latest -stdlib=libc++' \
  --output-dir reflection-fuzz-minimized
```

`--family` accepts `core` or `shapes` and defaults to `core` for compatibility with earlier invocations. A compiler crash or ordinary compile failure is not treated as a semantic mismatch; minimization stops and preserves the diagnostic distinction rather than mislabelling it.
