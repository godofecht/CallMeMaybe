# CallMeMaybe Runtime Reflection Research Report

Status: evidence collection in progress. Numerical claims belong here only after their raw machine-readable inputs are committed.

## Question

How close can a C++26 reflection-backed runtime reflection system get to hand-written dispatch while retaining runtime discovery, safe erased invocation and enough metadata to support tooling and foreign-language bindings?

## Experimental tracks

### Runtime dispatch

Compare direct calls, function pointers, virtual dispatch, variant dispatch, table dispatch and CallMeMaybe invocation. Extend with RTTR and metapp adapters. Report latency distributions, startup cost, compile cost, memory and binary size across increasing entity counts.

### Reflection differential testing

Generate deterministic legal C++26 programs and compare semantic fingerprints across GCC reflection and Clang/P2996. Every disagreement must retain the generated source, exact compiler commands and normalized outputs so it can become either a compiler bug or a CallMeMaybe regression test.

### Consteval registry

Port the compile-time registry architecture onto the stabilized semantics and compare it against the runtime registry. The result is allowed to be negative: reduced startup cost does not count as a win if compile time, memory or binary size regress disproportionately.

### Flow bridge

Use runtime reflection as an actual language boundary. Measure how much binding code can disappear when Flow resolves and invokes reflected C++ entities through a checked tagged ABI.

## Evidence rules

1. Shared CI establishes correctness, not publishable timing.
2. Every timing table records CPU, OS, compiler revision, standard library revision, build flags and commit SHA.
3. Every cross-compiler disagreement retains a minimal or minimizable source artifact.
4. No benchmark comparison is reported if different dispatch paths compute different checksums.
5. Cold/startup measurements and hot steady-state measurements are reported separately.
6. Binary-size claims distinguish unstripped and stripped artifacts.
7. Failed hypotheses remain in the report.

## Results

_No validated result sets have been promoted into this branch yet._

## Findings

_No findings yet. This section is generated from retained evidence, not filled speculatively._

## Upstream candidates

A result becomes an upstream candidate only when it has a focused reproducer or patch, a regression test, and an explanation of the semantic/performance trade-off. Large research branches are not intended to be submitted upstream wholesale.

The current candidate queue is deliberately narrower than the research queue:

| Candidate | Evidence now | Upstream form | Status |
| --- | --- | --- | --- |
| Parent-aware entity identity and parameter-position identity | Differential corpus in PR #3 exercises same-named members in distinct declaring classes, repeated parameter types, parent metadata and normalized fingerprints; the stabilized fork already carries the corresponding identity semantics | Focused hash/identity patch plus the smallest identity regression cases | Candidate; retain as separate identity patch rather than bundling with the fuzzer |
| Ambiguous bare member lookup | PR #3 contains an overload-set family that requires bare lookup to reject ambiguity while signature-selected invocation succeeds; GCC CI smoke coverage is retained on the track | Focused lookup patch plus overload regression | Candidate; semantic behavior is independently testable |
| Derived-to-base instance adjustment, including virtual diamonds | PR #3 contains inheritance/diamond/virtual-base generation and derived-to-virtual-base invocation validation; this is isolated from the benchmark and Flow work | Focused invocation-adjustment patch plus inheritance regression | Candidate; preserve explicit ambiguous-path rejection |
| cv/ref-preserving erased member invocation | PR #3 exercises cv/ref-qualified member-function families, mutable-vs-const invocation and `ConstViolation` behavior | Focused erased-value/invocation patch plus cv/ref regression | Candidate; keep separate from overload lookup |
| Unsigned/high-bit enum metadata preservation | PR #3 explicitly stresses high-bit unsigned enumerators and parent metadata; PR #5's constexpr `Enum::Entry` storage preserves raw value bits plus signedness | Focused enum metadata patch plus high-bit regression | Candidate; no performance claim required |
| Constexpr-capable entity metadata and static class metadata spans | PR #5 has compile-time `static_assert` coverage for core/leaf/function/enum metadata and CI #249 validates constexpr `Class` aggregate metadata populated from generated static member/base/name spans under GCC 16.2 release and ASan/UBSan | Stageable storage-only patch before any registry-backend replacement | Candidate; do not upstream the unfinished consteval registry architecture wholesale |
| Borrowed Flow ABI profiles | PR #4 has an end-to-end linked Flow executable and sanitizer coverage for scalar, string and borrowed byte-span parameter calls; byte-span return support remains blocked and explicitly unchecked | Optional bridge module, not a core runtime patch | Research artifact only until API scope and ownership rules stabilize |
| Consteval registry replacement | PR #5 still has runtime-owned fallback containers, runtime registry construction, no `RegistryView`, no cross-compiler validation and no controlled benchmark comparison | None yet | Not an upstream candidate |
| RTTR/metapp performance comparison | PR #2 has the benchmark harness but adapters and controlled-machine results are still missing | None yet | Not an upstream candidate |
| Cross-compiler reflection disagreement | PR #3 has generation, classification, persistence and minimization infrastructure, but no retained 100k-campaign disagreement has been promoted | Compiler reproducer or library regression depending on classification | Not a candidate until an actual disagreement artifact exists |

This list is intentionally evidence-gated: it identifies patch-sized directions already supported by regression machinery while explicitly excluding unfinished architectural work and uncollected performance results.
