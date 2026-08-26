# Reflection Differential Fuzzer

This harness generates deterministic, legal C++26 programs that stress runtime-reflection identity and metadata invariants, compiles the same corpus with one or more reflection-capable compilers, runs each binary, and compares normalized semantic fingerprints.

The first corpus family targets bugs that are disproportionately likely in a runtime registry built from compile-time reflection:

- same-named members in distinct declaring classes
- different physical layouts for semantically similar classes
- parameter-position identity for repeated parameter types
- parent metadata for members / parameters / enumerators
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

## Large resumable campaigns

`campaign.py` can split one global seed window into deterministic non-overlapping shards. Seed assignment is based on the seed's zero-based offset modulo `--shard-count`, so changing execution order or resuming a shard cannot change which programs belong to it. Each shard writes its index, shard count, seed start and selected-seed count into `campaign.json`.

`--programs` is the number of seeds in the global window, not the total number of generated programs. With `--family both`, every selected seed generates one `core` program and one `shapes` program. For example, 50,000 seeds across both families produce a 100,000-program campaign.

An eight-shard 100,000-program campaign can be run as eight independent local commands by varying only `--shard-index` and the output directory:

```sh
python3 tools/reflection_fuzz/campaign.py \
  --programs 50000 \
  --family both \
  --shard-count 8 \
  --shard-index 0 \
  --compiler 'gcc=g++-16 -std=c++26 -freflection' \
  --compiler 'clang=/path/to/clang++ -std=c++26 -freflection-latest -stdlib=libc++' \
  --output-dir reflection-fuzz-campaign/shard-0 \
  --resume
```

The remaining shard indexes are `1` through `7`. Keep compiler commands, seed start, program count, family selection and shard count identical across shards so the retained manifests describe one reproducible campaign.

Before promoting a sharded campaign as evidence, merge it through `merge_campaign.py`. The merger rejects missing or duplicate shard indexes, incompatible campaign metadata, incorrect per-shard seed counts, duplicate family/seed records, incorrect status totals, and any gap or extra program in the exact global seed/family Cartesian product. Only a fully covered campaign produces the merged manifest.

```sh
python3 tools/reflection_fuzz/merge_campaign.py \
  reflection-fuzz-campaign/shard-{0,1,2,3,4,5,6,7}/campaign.json \
  --output reflection-fuzz-campaign/campaign.json
```

The merged manifest preserves the compiler commands and campaign parameters, recomputes status totals from the records, sorts records deterministically by seed then family, and retains the source shard paths. Its exit classification matches `campaign.py`: semantic disagreements exit 3, compile/invariant/runner failures exit 1, and a complete all-green campaign exits 0.
