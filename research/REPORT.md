# CallMeMaybe Runtime Reflection Research Report

Status: implementation findings complete; controlled-machine performance and cross-compiler evidence collection remain open. Numerical claims belong here only after their raw machine-readable inputs are committed.

## Question

How close can a C++26 reflection-backed runtime reflection system get to hand-written dispatch while retaining runtime discovery, safe erased invocation and enough metadata to support tooling and foreign-language bindings?

## Experimental tracks

### Runtime dispatch

Compare direct calls, function pointers, virtual dispatch, variant dispatch, table dispatch and CallMeMaybe invocation. RTTR and metapp adapters now execute the same deterministic workload and reject checksum disagreement. Publishable latency, startup, compile-cost, memory and binary-size comparisons still require a controlled machine.

### Reflection differential testing

Generate deterministic legal C++26 programs and compare semantic fingerprints across GCC reflection and Clang/P2996. Every disagreement must retain the generated source, exact compiler commands and normalized outputs so it can become either a compiler bug or a CallMeMaybe regression test.

### Consteval registry

Port the compile-time registry architecture onto the stabilized semantics and compare it against the runtime registry. The implemented static slices now replace explicit startup registration with namespace-scope static registry declarations, but the result is allowed to be negative: removing that registration step does not count as a win if compile time, memory or binary size regress disproportionately.

### Flow bridge

Use runtime reflection as an actual language boundary. The end-to-end path now covers scalar values, borrowed strings and byte spans, scalar pointer/lvalue-reference borrowing with explicit qualifier rules, enum values, aggregate handles, constructors/methods, and explicit object ownership transfer through generated typed Flow wrappers.

## Evidence rules

1. Shared CI establishes correctness, not publishable timing.
2. Every timing table records CPU, OS, compiler revision, standard library revision, build flags and commit SHA.
3. Every cross-compiler disagreement retains a minimal or minimizable source artifact.
4. No benchmark comparison is reported if different dispatch paths compute different checksums.
5. Cold/startup measurements and hot steady-state measurements are reported separately.
6. Binary-size claims distinguish unstripped and stripped artifacts.
7. Failed hypotheses remain in the report.

## Results

_No controlled-machine runtime benchmark result set or retained cross-compiler disagreement has been promoted into this branch yet._

### Flow bridge footprint

PR #4 CI #465 records a deterministic footprint probe from the same reflected fixture used by the linked Flow end-to-end executable. The raw evidence is committed as `research/evidence/flow_bridge_ci465.json` and is scoped to generated source footprint and ABI storage only; shared-CI timing is not used as a latency claim.

| Metric | Result |
| --- | ---: |
| Reflected/generated fixture bindings | 21 |
| Generated typed Flow wrapper source | 16,493 bytes |
| `cmm_flow_value` tagged carrier | 24 bytes |
| `cmm_flow_span_u8` carrier | 16 bytes |
| `cmm_flow_info` entity ID | 8 bytes |

The 24-byte tagged value contains an 8-byte primary payload plus kind/reserved metadata and an 8-byte auxiliary field, so the scalar carrier uses 16 bytes more storage than its 64-bit payload. This is an ABI-storage result, not a runtime-overhead timing claim. CI #465 also transpiles the generated Flow module, compiles the generated C as C11, links it against the bridge and reflected C++ fixture, executes it successfully, and passes ASan/UBSan.

Correctness evidence is retained in the research tracks. PR #2 CI #290 builds and executes CallMeMaybe, RTTR and metapp benchmark adapters and passes the sanitizer stage. PR #4 CI #465 exercises the complete linked Flow → C ABI → CallMeMaybe → C++ path across all currently scoped ABI families and retains the footprint metrics above. The consteval research track validates constexpr-capable metadata, static registry construction and semantic slices under GCC 16.2 release and sanitizers. These correctness runs are not used as publishable runtime-performance measurements.

## Findings

The completed implementation work supports architectural and semantic conclusions, but not runtime-performance rankings. Runtime-reflection comparison infrastructure is broad enough to produce controlled results without changing workloads between CallMeMaybe, RTTR and metapp. The differential harness can deterministically shard, retain, integrity-check and minimize compiler/library disagreements when they are found. The Flow bridge demonstrates a complete scoped foreign-language binding surface with generated wrappers for scalar, borrowed, enum, aggregate, object and ownership profiles; its retained evidence quantifies generated binding footprint and ABI carrier sizes without converting shared-run timing into a speed claim.

The consteval track has progressed beyond a storage prototype: constexpr-capable metadata, static class/function/enum spans, explicit dependency materialization state, duplicate-ID rejection, sorted consteval entity/name indexes and a non-templated `RegistryView` are implemented, and the implemented static slices use compile-time registry declarations instead of explicit runtime `register_rrefl` startup calls. That is a real architectural result. It is not evidence of full backend equivalence or a performance win: the complete stabilization corpus, Clang/P2996 validation and controlled-machine benchmark comparison remain open.

Accordingly, the strongest present conclusion is that CallMeMaybe can support three materially different surfaces from one reflected semantic core: stabilized runtime reflection, a compile-time-built registry for implemented static slices, and a generated Flow foreign-language boundary. The remaining open work is evidence collection rather than a prerequisite for landing these research implementations: controlled performance data determines whether the consteval trade is worthwhile, while the 100k cross-compiler campaign determines whether any compiler/CMM disagreement can be promoted as a reproducible result.

## Upstream candidates

A result becomes an upstream candidate only when it has a focused reproducer or patch, a regression test, and an explanation of the semantic/performance trade-off. Large research branches are not intended to be submitted upstream wholesale.

| Candidate | Evidence now | Upstream form | Status |
| --- | --- | --- | --- |
| Parent-aware entity identity and parameter-position identity | PR #3 exercises same-named members in distinct declaring classes, repeated parameter types, parent metadata and normalized fingerprints | Focused hash/identity patch plus the smallest identity regression cases | Candidate |
| Ambiguous bare member lookup | PR #3 requires bare overload lookup to reject ambiguity while signature-selected invocation succeeds | Focused lookup patch plus overload regression | Candidate |
| Derived-to-base instance adjustment, including virtual diamonds | PR #3 contains inheritance/diamond/virtual-base generation and derived-to-virtual-base invocation validation | Focused invocation-adjustment patch plus inheritance regression | Candidate |
| cv/ref-preserving erased member invocation | PR #3 exercises cv/ref-qualified member-function families, mutable-vs-const invocation and `ConstViolation` behavior | Focused erased-value/invocation patch plus cv/ref regression | Candidate |
| Unsigned/high-bit enum metadata preservation | PR #3 stresses high-bit unsigned enumerators and PR #5 preserves raw enum value bits plus signedness in constexpr storage | Focused enum metadata patch plus high-bit regression | Candidate |
| Constexpr-capable metadata and static aggregate spans | PR #5 has compile-time coverage for core, leaf, function, enum and class metadata | Stageable storage-only patch before registry-backend replacement | Candidate |
| Dependency-stub/full-entity materialization semantics | PR #5 makes dependency state explicit and preserves registration-order behavior | Focused registry-state patch plus registration-order regression | Candidate |
| Consteval entity/name indexes and `RegistryView` | PR #5 validates constexpr-sorted entity/name indexes, duplicate-name ambiguity and a non-templated span-backed runtime view | Stageable index/view patch | Candidate |
| Flow bridge ABI and generated bindings | PR #4 covers all currently scoped ABI families end to end and retains exact generated-wrapper/ABI-storage footprint evidence | Optional bridge module, not a core runtime patch | Research artifact |
| RTTR/metapp adapters | PR #2 builds and executes both adapters against the same checksum-validated workload | Benchmark infrastructure only | No performance claim until controlled-machine data exists |
| Consteval registry replacement | PR #5 replaces explicit startup registration for implemented static slices, but full-corpus, Clang/P2996 and controlled benchmark validation remain incomplete | Focused backend patch only after the remaining evidence gates | Research artifact; not yet an upstream performance claim |
| Cross-compiler reflection disagreement | PR #3 has generation, classification, persistence and minimization infrastructure but no retained 100k-campaign disagreement has been promoted | Compiler reproducer or library regression depending on classification | Not a candidate until an actual disagreement exists |

## Laurie-facing summary

The fork contains several patch-sized results worth discussing independently, without asking upstream to absorb the research branches wholesale.

For core semantics, PR #3 (`https://github.com/godofecht/CallMeMaybe/pull/3`) carries deterministic regressions for parent-aware identity, parameter-position identity, ambiguous overload lookup, cv/ref-preserving erased invocation, derived-to-base adjustment including virtual diamonds, and unsigned/high-bit enum metadata. These are suitable for small upstream patches because each behavior is independently testable and does not depend on benchmark claims.

For constexpr architecture, PR #5 (`https://github.com/godofecht/CallMeMaybe/pull/5`) has moved metadata storage, class/function/enum aggregate spans, dependency materialization state, entity/name indexes and a non-templated `RegistryView` into constexpr-capable forms while preserving substantial stabilized semantics. The implemented static slices now use compile-time registry declarations instead of explicit startup `register_rrefl` calls. The important boundary is still explicit: full stabilization-corpus equivalence, Clang/P2996 validation and controlled performance evidence remain open, so the full consteval backend should not yet be presented upstream as a proven performance replacement.

For reproducible measurement, PR #2 (`https://github.com/godofecht/CallMeMaybe/pull/2`) contains checksum-validated CallMeMaybe, RTTR and metapp adapters plus scaling instrumentation through 10,000 entities. Its shared-CI timings are intentionally not presented as performance results. A controlled-machine artifact is still required before making speed, startup, memory or binary-size claims.

For language-boundary work, PR #4 (`https://github.com/godofecht/CallMeMaybe/pull/4`) covers the scoped Flow ABI end to end: scalar values, borrowed strings/byte spans, pointer/reference borrowing rules, enums, aggregates, constructors/methods and ownership transfer. Retained evidence records an exact 21-binding generated-wrapper footprint of 16,493 bytes and ABI carrier sizes of 24 bytes (`cmm_flow_value`), 16 bytes (byte span) and 8 bytes (entity ID). These are footprint/storage claims only, not shared-CI latency claims.

The differential campaign in PR #3 is ready to retain minimized GCC/Clang-P2996 disagreements, but none has yet been promoted as evidence. Likewise, no controlled benchmark result is committed yet. Those absences are deliberate: they prevent the report from turning shared-CI noise or hypothetical compiler disagreements into upstream claims.

The recommended upstream sequence is therefore: discuss the small semantic regressions first; stage constexpr metadata/storage and index/view changes separately if desired; keep performance claims and the full consteval-backend recommendation in the fork until the remaining evidence gates are satisfied; treat the Flow bridge as an independent research artifact with reproducible footprint evidence.
