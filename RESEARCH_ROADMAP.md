# CallMeMaybe Research Roadmap

This fork is being used to push C++26 runtime reflection beyond library maintenance into reproducible systems research.

GitHub Issues are currently disabled on this fork, so draft pull requests are the executable task cards. Each card carries its own checked acceptance list and stays draft until the track has evidence worth merging or upstreaming.

## Active tracks

### PR #2 — Reproducible runtime-reflection benchmark suite

https://github.com/godofecht/CallMeMaybe/pull/2

- [x] direct / function-pointer / virtual / variant / table dispatch baselines
- [x] typed and raw CallMeMaybe invocation
- [x] correctness checks and latency distributions
- [x] machine-readable JSON and Markdown summarization
- [x] 10 / 100 / 1,000 / 10,000 entity scaling runner
- [x] compile wall time, peak compiler RSS, binary size and stripped size collection
- [x] green CI smoke validation
- [x] RTTR adapter
- [x] metapp adapter
- [ ] publishable result set on a controlled machine

### PR #3 — Differential C++26 reflection fuzzer

https://github.com/godofecht/CallMeMaybe/pull/3

- [x] deterministic corpus generation
- [x] class/member identity and layout invariants
- [x] parameter identity and parent metadata
- [x] enum/high-bit metadata
- [x] GCC / Clang-P2996 fingerprint comparison harness
- [x] retained manifests and smallest-prefix minimization
- [x] virtual-diamond inheritance and derived-to-virtual-base invocation corpus
- [x] cv/ref-qualified member-function family
- [x] overload-set family and ambiguous bare-lookup rejection
- [x] anonymous / nested / template entity family
- [x] resumable classified multi-program campaign runner
- [x] deterministic non-overlapping campaign sharding with retained shard metadata
- [x] exact shard-coverage verification and deterministic merged campaign manifest
- [x] resume rejects incompatible compiler/campaign metadata and duplicate/out-of-window records
- [x] campaign disagreement extraction and batch minimization from retained manifest
- [x] content-address generated sources, compiler fingerprints, diagnostics and per-program manifests
- [x] resume rejects mutated retained manifests and shard merge requires manifest digests
- [x] batch minimizer re-verifies source disagreement manifests and content-addresses minimized manifests
- [x] green GCC CI smoke coverage across both corpus families
- [ ] 100k cross-compiler campaign and retained disagreement corpus

### PR #4 — CallMeMaybe ↔ Flow runtime reflection bridge

https://github.com/godofecht/CallMeMaybe/pull/4

- [x] stable tagged C ABI
- [x] reflected lookup, signature inspection and checked invocation
- [x] scalar integer / bool / f32 / f64 profile
- [x] borrowed `const char*` strings as native Flow `string`
- [x] borrowed byte-span parameters (`std::span<const uint8_t>` ↔ Flow `span<u8>`)
- [x] borrowed byte-span returns (`std::span<const uint8_t>` ↔ Flow `span<u8>`)
- [x] typed `int*`, `int&`, and `const int&` borrowing profile
- [x] scalar pointer / lvalue-reference borrowing across bool, signed/unsigned integer widths, f32 and f64
- [x] pointer cv-qualification, rvalue-reference and nested pointer/reference borrowing rules
- [x] enum parameters and returns via reflected underlying scalar ABI
- [x] native Flow `extern` declarations
- [x] generated ergonomic typed Flow wrappers with explicit error-bearing results
- [x] cross-boundary regression tests and sanitizer coverage
- [x] pinned Flow compiler transpilation and C11 compile validation
- [x] end-to-end linked Flow executable invoking reflected C++ functions
- [x] aggregates
- [x] methods / constructors
- [x] ownership-transfer profile

### PR #5 — Consteval registry port and measurement

https://github.com/godofecht/CallMeMaybe/pull/5

- [x] pin and inventory upstream consteval-registry experiment
- [x] capture the stabilized semantic contract it must preserve
- [x] define startup / lookup / invocation / compile-time / memory / size measurements
- [x] constexpr-capable core metadata (`Entity`, `Type`, `Parameter`, `Enumerator`)
- [x] constexpr-capable leaf metadata (`DataMember`, `Variable`, `Base`)
- [x] span-backed constexpr Function parameter metadata with runtime-builder fallback
- [x] span-backed constexpr Enum entry metadata preserving raw value/sign information
- [x] compile-time static-assert regression across core, leaf, function and enum metadata
- [x] constexpr-capable Class aggregate storage
- [x] generated static class member/base/name spans
- [x] remove transitional runtime-owned Function/Enum fallback containers
- [x] dependency-stub / full-entity split
- [x] consteval entity/name indexes and non-templated runtime `RegistryView`
- [x] reject duplicate entity IDs during static registry construction
- [ ] eliminate startup registration without semantic regression
- [x] GCC 16.2 release + ASan/UBSan validation on the implemented static backend
- [ ] Clang/P2996 validation on the new backend
- [ ] benchmark against the stabilized runtime backend

### PR #6 — Evidence-backed research report

https://github.com/godofecht/CallMeMaybe/pull/6

- [x] evidence policy and experimental structure
- [x] generator for benchmark and fuzzer result tables
- [x] machine metadata schema
- [x] machine/compiler/commit metadata collector
- [ ] committed controlled-machine benchmark results
- [ ] retained minimized reflection disagreements
- [x] consteval-registry comparison
- [x] Flow bridge binding-size / overhead result
- [ ] final findings section
- [x] focused upstream candidate list
- [x] concise Laurie-facing summary backed by reproducers and patches

The consteval-registry comparison is retained in PR #6 as an architectural/semantic comparison with machine-readable status. It explicitly leaves controlled-machine performance, full static-backend stabilization equivalence, and Clang/P2996 validation unresolved rather than inferring them from implementation progress.

Flow bridge footprint evidence is retained from PR #4 CI #465 in PR #6 at `research/evidence/flow_bridge_ci465.json`; it records deterministic generated-wrapper and ABI-storage sizes only, not shared-CI latency.

## Exit criterion

The point of these tracks is not to accumulate a large fork. Each result must reduce to one of three useful things: a reproducible measurement, a minimized compiler/library pathology, or a focused upstreamable patch. Negative results are retained when they answer the research question.
