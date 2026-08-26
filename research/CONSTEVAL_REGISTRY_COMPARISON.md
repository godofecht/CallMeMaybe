# Consteval registry comparison

This comparison is architectural and semantic evidence only. It does not rank runtime performance. The machine-readable companion is `research/evidence/consteval_registry_comparison.json`.

The stabilized runtime backend performs explicit `cmm::register_rrefl` registration in the matched scaling harness. The consteval branch replaces that registration block for its implemented static slices with namespace-scope static registry declarations backed by consteval-built entity and name indexes and a non-templated `RegistryView`. Static construction rejects duplicate entity IDs before index construction.

The static backend has implemented source-level parity slices for parent-aware member identity, parameter-position identity, unsigned/high-bit enum metadata, cv/ref-preserving invocation, overload ambiguity, declaration order, derived-to-base adjustment, null-instance rejection, reflected free/member invocation, constructor/destructor lifecycle handling, and move-only dynamic-object ownership. These establish that the experiment is no longer merely a storage prototype.

The comparison is not a claim of full backend equivalence. The complete stabilization regression corpus has not been validated on the static backend, Clang/P2996 validation remains open, and no controlled-machine result set has been committed. Therefore process startup, lookup/invocation latency, compile time, peak compiler RSS, runtime heap allocation, and binary-size rankings remain unknown.

A matched 10 / 100 / 1,000 / 10,000 entity harness exists on `research/consteval-registry`. It generates runtime and consteval sources from the same synthetic corpus and rejects incomplete lookup through a common checksum. Its runtime-side `main_registration_ns` field measures only the explicit registration block; zero on the consteval side must not be described as zero process-startup cost.

The evidence-backed conclusion at this stage is narrow: compile-time registry construction can replace explicit startup registration for the implemented static slices while retaining a substantial portion of the stabilized semantic model. Whether that trade is worthwhile remains an empirical question until the controlled-machine benchmark and Clang/P2996 gates are complete.
