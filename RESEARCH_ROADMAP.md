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
- [ ] RTTR and metapp adapters
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
- [x] green GCC CI corpus
- [ ] cv/ref-qualified member-function family
- [ ] overload-set family
- [ ] anonymous / nested / template family
- [ ] large cross-compiler campaign and retained disagreements

### PR #4 — CallMeMaybe ↔ Flow runtime reflection bridge

https://github.com/godofecht/CallMeMaybe/pull/4

- [x] stable tagged C ABI
- [x] reflected lookup, signature inspection and checked invocation
- [x] scalar integer / bool / f32 / f64 profile
- [x] native Flow `extern` declarations
- [x] cross-boundary regression tests and sanitizer coverage
- [x] green CI validation
- [ ] generated ergonomic Flow wrappers
- [ ] strings / spans / aggregates / enums
- [ ] typed borrowing and ownership rules
- [ ] methods / constructors
- [ ] end-to-end Flow compiler link test

### PR #5 — Consteval registry port and measurement

https://github.com/godofecht/CallMeMaybe/pull/5

- [x] pin and inventory upstream consteval-registry experiment
- [x] capture the stabilized semantic contract it must preserve
- [x] define startup / lookup / invocation / compile-time / memory / size measurements
- [ ] port constexpr-capable entity storage
- [ ] build consteval entity/name indexes and runtime `RegistryView`
- [ ] eliminate startup registration without semantic regression
- [ ] GCC sanitizer and Clang/P2996 validation
- [ ] benchmark against the stabilized runtime backend

### PR #6 — Evidence-backed research report

https://github.com/godofecht/CallMeMaybe/pull/6

- [x] evidence policy and experimental structure
- [x] generator for benchmark and fuzzer result tables
- [ ] machine metadata schema
- [ ] committed controlled-machine benchmark results
- [ ] retained minimized reflection disagreements
- [ ] consteval comparison and Flow bridge measurements
- [ ] focused upstream candidate list
- [ ] concise Laurie-facing summary backed by reproducers and patches

## Exit criterion

The point of these tracks is not to accumulate a large fork. Each result must reduce to one of three useful things: a reproducible measurement, a minimized compiler/library pathology, or a focused upstreamable patch. Negative results are retained when they answer the research question.
