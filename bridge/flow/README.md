# CallMeMaybe ↔ Flow Bridge

This bridge exposes CallMeMaybe's C++26 runtime reflection through a stable C ABI that Flow can call through its native `extern` mechanism.

C++ remains responsible for registering the entities that are intended to cross the boundary. Once registered, Flow can resolve a function by runtime name, inspect its arity and scalar ABI kinds, and invoke it without a hand-written wrapper for each function.

The first ABI profile intentionally supports values whose representation is portable and unambiguous across the language boundary: booleans, signed/unsigned integers up to 64 bits, `f32`, `f64`, and returned pointers. `void*` input is supported; arbitrary typed pointer input, references, aggregates, strings, ownership transfer, methods and constructors remain explicit follow-up work rather than being silently treated as safe.

## Why the ABI is tagged

`cmm_flow_value` carries a kind plus 64 bits of payload. The bridge checks that the supplied kind matches the reflected C++ parameter type before constructing `cmm::Value`. This preserves CallMeMaybe's runtime type checking instead of reducing the boundary to unchecked `void*` calls.

## Build

```sh
cmake -S . -B build-flow \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMM_BUILD_FLOW_BRIDGE=ON
cmake --build build-flow
ctest --test-dir build-flow --output-on-failure
```

The Flow side is `call_me_maybe.flow`; it uses Flow's current `extern { function ... }` syntax and the current `u32`, `u64`, `string`, and `ptr<T>` ABI types.

## Intended generated-binding path

The generic ABI is the foundation for generated ergonomic wrappers. A generator can inspect a registered function's parameter and return kinds and emit a Flow function that packs native Flow arguments into `CmmFlowValue`, invokes the runtime entity ID, checks the error code, and unpacks the result. That generation step is deliberately separate from the ABI so wrapper syntax can evolve without breaking binary compatibility.
