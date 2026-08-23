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
