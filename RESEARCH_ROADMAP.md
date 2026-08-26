# CallMeMaybe Research Roadmap

This fork is being used to push C++26 runtime reflection beyond library maintenance into reproducible systems research.

PRs #2–#6 are merged implementation tracks. Their remaining unchecked work is evidence collection that is intentionally run locally rather than through GitHub Actions. Exact local commands are retained in `research/LOCAL_EVIDENCE.md`.

## Merged implementation tracks

### PR #2 — Reproducible runtime-reflection benchmark suite

https://github.com/godofecht/CallMeMaybe/pull/2

- [x] direct / function-pointer / virtual / variant / table dispatch baselines
- [x] typed and raw CallMeMaybe invocation
- [x] correctness checks and latency distributions
- [x] machine-readable JSON and Markdown summarization
- [x] 10 / 100 / 1,000 / 10,000 entity scaling runner
- [x] compile wall time, peak compiler RSS, binary size and stripped size collection
- [x] RTTR adapter
- [x] metapp adapter
- [x] correctness smoke validation

### PR #3 — Differential C++26 reflection fuzzer

https://github.com/godofecht/CallMeMaybe/pull/3

- [x] deterministic corpus generation
- [x] class/member identity, layout, parameter, enum and parent invariants
- [x] GCC / Clang-P2996 fingerprint comparison harness
- [x] retained content-addressed manifests and smallest-prefix minimization
- [x] explicit immediately-smaller minimality witness retention
- [x] inheritance / diamond / virtual-base corpus and derived-instance adjustment checks
- [x] cv/ref-qualified member-function and overload-set families
- [x] anonymous / nested / template entity family
- [x] resumable classified campaign runner
- [x] deterministic non-overlapping sharding
- [x] exact shard-coverage verification and deterministic merge
- [x] resume integrity checks for campaign metadata and retained manifests
- [x] disagreement extraction and batch minimization
- [x] GCC smoke coverage across both corpus families

### PR #4 — CallMeMaybe ↔ Flow runtime reflection bridge

https://github.com/godofecht/CallMeMaybe/pull/4

- [x] stable tagged C ABI independent of C++ object layout
- [x] reflected lookup, signature inspection and checked invocation
- [x] bool / integer / f32 / f64 values
- [x] borrowed strings and byte spans
- [x] typed scalar pointer and lvalue-reference borrowing
- [x] pointer qualifier / rvalue-reference / nested-borrow rejection rules
- [x] enums through reflected underlying scalar ABI
- [x] native Flow extern declarations and generated typed wrappers
- [x] explicit error-bearing wrapper results
- [x] cross-boundary C++ regression tests and sanitizer coverage
- [x] pinned Flow transpilation, C11 compile and final linked executable
- [x] aggregates
- [x] methods / instances / constructors
- [x] ownership-transfer profile

### PR #5 — Consteval registry architecture

https://github.com/godofecht/CallMeMaybe/pull/5

- [x] pin and inventory the upstream consteval-registry experiment
- [x] capture the stabilized semantic contract
- [x] constexpr-capable core and leaf metadata
- [x] span-backed constexpr Function, Enum and Class aggregate metadata
- [x] remove transitional runtime-owned Function/Enum fallback containers
- [x] dependency-stub / full-entity split
- [x] consteval entity/name indexes and non-templated `RegistryView`
- [x] reject duplicate entity IDs during static registry construction
- [x] public runtime/static API parity audit including generated type predicates
- [x] mechanical no-`register_rrefl` audit for the static regression corpus
- [x] compile-time registry declaration replacing explicit startup registration
- [x] semantic parity coverage for the applicable stabilization invariants
- [x] static data-member read/ref/write semantics
- [x] public/protected/private and ambiguous-base semantics
- [x] declaration-order and base-edge identity semantics
- [x] cv/ref invocation, overload ambiguity, null handling and lifecycle semantics
- [x] move-only invocation and `DynamicObject` ownership semantics
- [x] GCC 16.2 release + ASan/UBSan validation on the implemented static backend

The runtime registry's late-mutation/freeze behavior is intentionally not a static-backend parity requirement: the compile-time registry is immutable by construction and has no corresponding post-query mutation operation.

### PR #6 — Evidence-backed research report

https://github.com/godofecht/CallMeMaybe/pull/6

- [x] evidence policy and experimental structure
- [x] generator for benchmark and fuzzer result tables
- [x] machine metadata schema and collector
- [x] benchmark compiler executable path + SHA-256 provenance
- [x] consteval-registry architectural comparison
- [x] Flow bridge binding-footprint / ABI-storage evidence
- [x] final evidence-bounded findings section
- [x] focused upstream candidate list
- [x] concise Laurie-facing summary backed by reproducible artifacts

## Local evidence backlog

These are intentionally not merge blockers and must remain local rather than consuming GitHub Actions minutes.

- [ ] controlled-machine publishable runtime benchmark result set for PR #2 / PR #6
- [ ] matched controlled-machine runtime-vs-consteval registry comparison for PR #5 / PR #6
- [ ] Clang/P2996 validation of the static backend for PR #5
- [ ] 100,000-program GCC/Clang-P2996 differential campaign for PR #3
- [ ] retained minimized disagreement corpus if the 100k campaign finds disagreements
- [ ] commit the resulting raw evidence + machine metadata and regenerate the quantitative report sections

A zero-disagreement campaign is itself a valid negative result. Likewise, a consteval backend that loses on compile time, memory or binary size should be reported rather than hidden.

See `research/LOCAL_EVIDENCE.md` for the exact local commands.

## Exit criterion

Each research track must reduce to at least one useful artifact: a reproducible measurement, a minimized compiler/library pathology, or a focused upstreamable patch. Implementation completion and evidence collection are tracked separately so merged code does not falsely appear incomplete merely because controlled experiments still need to be run.
