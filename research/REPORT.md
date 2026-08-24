# CallMeMaybe Runtime Reflection Research Report

Status: evidence collection in progress. Numerical claims belong here only after their raw machine-readable inputs are committed.

## Question

How close can a C++26 reflection-backed runtime reflection system get to hand-written dispatch while retaining runtime discovery, safe erased invocation and enough metadata to support tooling and foreign-language bindings?

## Experimental tracks

### Runtime dispatch

Compare direct calls, function pointers, virtual dispatch, variant dispatch, table dispatch and CallMeMaybe invocation. RTTR and metapp adapters now execute the same deterministic workload and reject checksum disagreement. Publishable latency, startup, compile-cost, memory and binary-size comparisons still require a controlled machine.

### Reflection differential testing

Generate deterministic legal C++26 programs and compare semantic fingerprints across GCC reflection and Clang/P2996. Every disagreement must retain the generated source, exact compiler commands and normalized outputs so it can become either a compiler bug or a CallMeMaybe regression test.

### Consteval registry

Port the compile-time registry architecture onto the stabilized semantics and compare it against the runtime registry. The result is allowed to be negative: reduced startup cost does not count as a win if compile time, memory or binary size regress disproportionately.

### Flow bridge

Use runtime reflection as an actual language boundary. The current end-to-end path covers scalar values, borrowed strings and borrowed byte-span parameters through generated typed Flow wrappers. Ownership transfer, typed pointer/reference borrowing, aggregates/enums and methods remain separate unfinished profiles.

## Evidence rules

1. Shared CI establishes correctness, not publishable timing.
2. Every timing table records CPU, OS, compiler revision, standard library revision, build flags and commit SHA.
3. Every cross-compiler disagreement retains a minimal or minimizable source artifact.
4. No benchmark comparison is reported if different dispatch paths compute different checksums.
5. Cold/startup measurements and hot steady-state measurements are reported separately.
6. Binary-size claims distinguish unstripped and stripped artifacts.
7. Failed hypotheses remain in the report.

## Results

_No controlled-machine benchmark result set or retained cross-compiler disagreement has been promoted into this branch yet._

Correctness evidence is already retained in the research tracks. PR #2 CI #290 builds and executes CallMeMaybe, RTTR and metapp benchmark adapters and passes the sanitizer stage. PR #4 CI #245 exercises the linked Flow → C ABI → CallMeMaybe → C++ path for integers, floating point, booleans, borrowed strings and borrowed byte-span parameters. PR #5 CI #267 validates the consteval entity/name index builder under GCC 16.2 release and ASan/UBSan. These runs are correctness evidence only and are not used as publishable performance measurements.

## Findings

The work so far supports architectural and semantic conclusions, but not performance rankings. Runtime-reflection comparison infrastructure is now broad enough to produce controlled results without changing workloads between CallMeMaybe, RTTR and metapp. The differential harness is broad enough to retain and minimize a compiler/library disagreement when one is found. The Flow bridge demonstrates that reflected lookup and checked erased invocation can drive generated foreign-language wrappers without a hand-authored C++ wrapper per function. The consteval port has proved constexpr-capable metadata, static class/function/enum spans, explicit dependency materialization state, sorted consteval indexes and a non-templated `RegistryView`; replacing runtime startup registration remains unfinished.

## Upstream candidates

A result becomes an upstream candidate only when it has a focused reproducer or patch, a regression test, and an explanation of the semantic/performance trade-off. Large research branches are not intended to be submitted upstream wholesale.

| Candidate | Evidence now | Upstream form | Status |
| --- | --- | --- | --- |
| Parent-aware entity identity and parameter-position identity | PR #3 exercises same-named members in distinct declaring classes, repeated parameter types, parent metadata and normalized fingerprints | Focused hash/identity patch plus the smallest identity regression cases | Candidate |
| Ambiguous bare member lookup | PR #3 requires bare overload lookup to reject ambiguity while signature-selected invocation succeeds | Focused lookup patch plus overload regression | Candidate |
| Derived-to-base instance adjustment, including virtual diamonds | PR #3 contains inheritance/diamond/virtual-base generation and derived-to-virtual-base invocation validation | Focused invocation-adjustment patch plus inheritance regression | Candidate |
| cv/ref-preserving erased member invocation | PR #3 exercises cv/ref-qualified member-function families, mutable-vs-const invocation and `ConstViolation` behavior | Focused erased-value/invocation patch plus cv/ref regression | Candidate |
| Unsigned/high-bit enum metadata preservation | PR #3 stresses high-bit unsigned enumerators and PR #5 preserves raw enum value bits plus signedness in constexpr storage | Focused enum metadata patch plus high-bit regression | Candidate |
| Constexpr-capable metadata and static aggregate spans | PR #5 has compile-time coverage for core, leaf, function, enum and class metadata; CI #249 validates generated static class member/base/name spans | Stageable storage-only patch before registry-backend replacement | Candidate |
| Dependency-stub/full-entity materialization semantics | PR #5 makes dependency state explicit and CI #259 preserves registration-order behavior under release and sanitizers | Focused registry-state patch plus registration-order regression | Candidate |
| Consteval entity/name indexes and `RegistryView` | PR #5 CI #267 validates constexpr-sorted entity/name indexes, duplicate-name ambiguity and a non-templated span-backed runtime view | Stageable index/view patch independent of startup-registry replacement | Candidate |
| Borrowed Flow ABI profiles | PR #4 CI #245 covers scalar, string and borrowed byte-span parameter calls end to end | Optional bridge module, not a core runtime patch | Research artifact until ownership/API scope stabilizes |
| RTTR/metapp adapters | PR #2 CI #290 builds and executes both adapters against the same checksum-validated workload | Benchmark infrastructure only | No performance claim until controlled-machine data exists |
| Consteval registry replacement | Static storage/index/view prerequisites exist in PR #5, but startup registration has not yet been eliminated and cross-compiler/benchmark validation is incomplete | None yet | Not an upstream candidate |
| Cross-compiler reflection disagreement | PR #3 has generation, classification, persistence and minimization infrastructure but no retained 100k-campaign disagreement has been promoted | Compiler reproducer or library regression depending on classification | Not a candidate until an actual disagreement exists |

## Laurie-facing summary

The fork now contains several patch-sized results worth discussing independently, without asking upstream to absorb the research branches wholesale.

For core semantics, PR #3 (`https://github.com/godofecht/CallMeMaybe/pull/3`) carries deterministic regressions for parent-aware identity, parameter-position identity, ambiguous overload lookup, cv/ref-preserving erased invocation, derived-to-base adjustment including virtual diamonds, and unsigned/high-bit enum metadata. These are suitable for small upstream patches because each behavior is independently testable and does not depend on benchmark claims.

For constexpr architecture, PR #5 (`https://github.com/godofecht/CallMeMaybe/pull/5`) has moved metadata storage, class/function/enum aggregate spans, dependency materialization state, entity/name indexes and a non-templated `RegistryView` into constexpr-capable forms while preserving the stabilized semantics. CI #249, #257, #259, #264 and #267 provide GCC 16.2 release/sanitizer evidence for those stages. The important boundary is explicit: startup registration has not yet been eliminated, so the full consteval backend should not be proposed upstream yet.

For reproducible measurement, PR #2 (`https://github.com/godofecht/CallMeMaybe/pull/2`) now contains checksum-validated CallMeMaybe, RTTR and metapp adapters plus scaling instrumentation through 10,000 entities. CI #290 proves the three adapters build and execute together, but its timings are intentionally not presented as performance results. A controlled-machine artifact is still required before making any speed, startup, memory or size claim.

For language-boundary work, PR #4 (`https://github.com/godofecht/CallMeMaybe/pull/4`) demonstrates generated Flow wrappers calling reflected C++ through a checked tagged C ABI, with CI #245 covering scalar values, borrowed strings and borrowed byte-span parameters end to end. This is useful as evidence that CallMeMaybe can support foreign-language bindings, but it should remain a separate bridge experiment until pointer/reference borrowing, ownership transfer, aggregate/enum representation and methods are settled.

The differential campaign in PR #3 is ready to retain minimized GCC/Clang-P2996 disagreements, but none has yet been promoted as evidence. Likewise, no controlled benchmark result is committed yet. Those absences are deliberate: they prevent the report from turning shared-CI noise or hypothetical compiler disagreements into upstream claims.

The recommended upstream sequence is therefore: discuss the small semantic regressions first; stage constexpr metadata/storage and index/view changes separately if desired; keep the full consteval registry replacement, Flow ABI expansion and performance comparisons in the fork until their remaining evidence gates are satisfied.
