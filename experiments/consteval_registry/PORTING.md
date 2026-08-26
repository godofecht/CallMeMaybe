# Consteval Registry Port and Measurement Plan

## Source experiment

Upstream CallMeMaybe PR #1 (`LaurieWired/CallMeMaybe`, head `10c85912ed28433c31698670f42c89ae2c198979`) replaces the runtime singleton registry with a consteval-built registry and a non-templated runtime view. It changes 14 files, replacing dynamic registry containers and incremental member construction with constexpr-capable entities, static arrays and spans.

This fork has since accumulated a large correctness stabilization pass. The consteval experiment must therefore be ported forward rather than copied wholesale.

## Semantic contract that must survive the port

The compile-time backend is not acceptable unless all of the following remain true:

- declaring-parent identity participates in member IDs, so same-named members in different classes cannot collide
- repeated function parameter types remain distinct by parameter position and parent function
- distinct entities that hash to the same ID are detected instead of silently aliasing
- dependency stubs can be promoted to fully registered entities regardless of registration order
- enum parent metadata is correct and unsigned/high-bit enumerators preserve their representation
- complete type predicates remain available through the runtime API
- class member metadata preserves declaration order
- overload lookup reports ambiguity rather than picking an arbitrary candidate
- erased references preserve constness, cv/ref categories and aliasing
- member invocation can adjust derived instances to accessible base subobjects and rejects ambiguous base paths
- null instance handling remains explicit
- destructor invocation remains supported
- constructor lookup uses canonical reflected type identity and supports zero arguments without non-standard arrays
- GCC 16.2 release tests and ASan/UBSan remain green
- the installed CMake package remains usable

## Performance questions

The experiment is only interesting if it answers measurable questions. Compare the stabilized runtime registry and the consteval backend on the same entity corpus for:

1. process startup before first user call
2. explicit registration time removed from `main`
3. first runtime lookup latency
4. steady-state lookup latency
5. dynamic invocation latency
6. compile time
7. peak compiler memory
8. unstripped and stripped binary size
9. runtime heap allocation before first lookup
10. scaling at 10, 100, 1,000 and 10,000 reflected entities

No performance claim should be made from a single shared-CI timing run. CI is for compilation and semantic equivalence; publishable timing data belongs in the benchmark result set with machine metadata.

## Port sequence

- [x] Inventory the upstream experiment and pin its exact head commit.
- [x] Record the stabilized semantic contract that cannot regress.
- [x] Define the benchmark dimensions that decide whether the architecture is worthwhile.
- [x] Introduce constexpr-capable entity storage without changing the public runtime API.
- [x] Replace incremental vector-backed member lists with stable spans over generated arrays.
- [x] Split dependency stubs from explicitly requested full entity materialization.
- [x] Build entity/name indexes during constant evaluation.
- [x] Expose a non-templated runtime `RegistryView` over constant-initialized storage.
- [x] Replace startup `register_rrefl` calls with an explicit compile-time build declaration for the currently materialized static metadata slice.
- [ ] Port every stabilization regression test to the new backend.
- [x] Run release and sanitizer CI on GCC 16.2.
- [ ] Validate the current Clang/P2996 path or document a compiler-specific blocker with a minimal reproducer.
- [ ] Feed both backends into the benchmark suite and commit the comparison results.

`StaticRegistryData` owns fixed sorted entity/name arrays, and its consteval builder derives the name index directly from entity storage, sorts both indexes, and marks duplicate names ambiguous with `cmm::invalid_info`. `RegistryView` exposes binary-search lookup through non-templated spans. The public static metadata slice is declared at namespace scope and queried without `register_rrefl`; CI #481 proves that declaration/query path, while CI #484 additionally runs the static-backend parent-aware member-identity regression under GCC 16.2 Release and ASan/UBSan. The remaining semantic gate is the full stabilization regression corpus, followed by Clang/P2996 validation and controlled benchmark comparison.

## Matched scaling harness

`generate_scale_pair.py` now emits two sources from the same synthetic type corpus. The runtime source explicitly registers every reflected type through `register_rrefl`, while the consteval source feeds the same reflected type pack through `make_type_static_registry` and `CMM_USE_STATIC_REGISTRY`. Both execute the same name-lookup checksum, so a comparison run is rejected if either backend cannot resolve the complete corpus.

`run_scale_pair.py` compiles both sources with the same compiler command and optimization flags, records compiler wall time, unstripped and stripped binary size, executes both binaries, and retains each backend's JSON metrics. The in-program registration field is deliberately named `main_registration_ns`: it measures the explicit runtime registration block and is exactly zero for the consteval source. It is not a process-startup measurement. Process startup, peak compiler RSS, heap allocation and publishable timing still require the controlled-machine measurement pass and remain part of the unchecked comparison gate.

The harness defaults to the planned 10 / 100 / 1,000 / 10,000 entity scale points. No generated timing or size result is committed by this implementation step.

## Upstream strategy

If the port wins materially, the upstreamable result should be small enough to review as architecture rather than as a fork dump: semantic regressions get isolated tests, storage changes get a focused patch, and performance evidence is linked independently. If the consteval backend loses badly on compile time or binary size, that result is still worth publishing; the experiment should not be forced into a predetermined conclusion.
