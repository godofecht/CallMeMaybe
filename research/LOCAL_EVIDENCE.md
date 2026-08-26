# Local evidence backlog

All implementation research tracks in PRs #2–#6 are merged. The remaining work is deliberately local evidence collection; it must not be moved into GitHub Actions.

## Controlled runtime benchmarks

Use a quiet machine and retain machine metadata beside every result set.

```sh
GXX=${GXX:-g++-16}
mkdir -p research/local-evidence
python3 research/collect_machine.py \
  --compiler "$GXX" \
  --build-type Release \
  --notes "controlled local benchmark run" \
  --output research/local-evidence/machine-gcc.json

cmake -S . -B build-local-bench \
  -DCMAKE_CXX_COMPILER="$GXX" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMM_BUILD_BENCHMARKS=ON \
  -DCMM_BUILD_TESTS=OFF \
  -DCMM_BUILD_EXAMPLES=OFF \
  -DCMM_BUILD_FLOW_BRIDGE=OFF
cmake --build build-local-bench -j

./build-local-bench/benchmarks/cmm_runtime_bench \
  --iterations 1000000 \
  --batch 4096 \
  --json research/local-evidence/runtime.json

if [[ -x build-local-bench/benchmarks/cmm_rttr_bench ]]; then
  ./build-local-bench/benchmarks/cmm_rttr_bench \
    --iterations 1000000 \
    --json research/local-evidence/rttr.json
fi

if [[ -x build-local-bench/benchmarks/cmm_metapp_bench ]]; then
  ./build-local-bench/benchmarks/cmm_metapp_bench \
    --iterations 1000000 \
    --json research/local-evidence/metapp.json
fi

python3 benchmarks/run_scale.py \
  --compiler "$GXX -std=c++26 -freflection" \
  --counts 10 100 1000 10000 \
  --output research/local-evidence/runtime-scale.json
```

The RTTR and metapp result files are only expected when those dependencies are installed and their benchmark targets were generated. Do not substitute shared-run timings for missing local measurements.

## Runtime versus consteval registry comparison

```sh
GXX=${GXX:-g++-16}
python3 experiments/consteval_registry/run_scale_pair.py \
  --compiler "$GXX" \
  --counts 10 100 1000 10000 \
  --output-dir research/local-evidence/consteval-scale \
  --json research/local-evidence/consteval-scale.json
```

This records matched compile wall time, explicit runtime registration cost, lookup cost, executable size, and stripped size. A zero `main_registration_ns` value for the consteval backend is not a zero process-startup claim.

## Clang/P2996 static-backend validation

Set `CLANG_P2996` to the reflection-capable Clang executable from the P2996 toolchain. The root CMake configuration supplies the repository's required `-freflection-latest`, matching libc++, runtime path, and reflection capability probe.

```sh
: "${CLANG_P2996:?set CLANG_P2996 to the P2996 clang++ executable}"
cmake -S . -B build-local-clang \
  -DCMAKE_CXX_COMPILER="$CLANG_P2996" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMM_BUILD_TESTS=ON \
  -DCMM_BUILD_EXAMPLES=OFF \
  -DCMM_BUILD_BENCHMARKS=OFF \
  -DCMM_BUILD_FLOW_BRIDGE=OFF
cmake --build build-local-clang -j
ctest --test-dir build-local-clang --output-on-failure
```

Passing this closes the remaining cross-compiler validation gate for the static backend.

## 100k GCC/Clang differential-reflection campaign

The campaign is 50,000 seeds across both corpus families, yielding 100,000 generated programs. It is sharded eight ways and resumable.

```sh
GXX=${GXX:-g++-16}
: "${CLANG_P2996:?set CLANG_P2996 to the P2996 clang++ executable}"
mkdir -p research/local-evidence/reflection-100k

for i in {0..7}; do
  python3 tools/reflection_fuzz/campaign.py \
    --programs 50000 \
    --family both \
    --shard-count 8 \
    --shard-index "$i" \
    --compiler "gcc=$GXX -std=c++26 -freflection" \
    --compiler "clang=$CLANG_P2996 -std=c++26 -freflection-latest -stdlib=libc++" \
    --output-dir "research/local-evidence/reflection-100k/shard-$i" \
    --resume
done

python3 tools/reflection_fuzz/merge_campaign.py \
  research/local-evidence/reflection-100k/shard-{0,1,2,3,4,5,6,7}/campaign.json \
  --output research/local-evidence/reflection-100k/campaign.json

python3 tools/reflection_fuzz/minimize_campaign.py \
  research/local-evidence/reflection-100k/campaign.json \
  --output-dir research/local-evidence/reflection-100k/minimized
```

A campaign with no semantic disagreements is a valid negative result. If disagreements exist, retain the content-addressed minimized artifacts and their immediately smaller clean-prefix witnesses.

## Promote evidence into the report

Only after the local artifacts exist should numerical or disagreement results be committed and rendered into the research report.

```sh
python3 research/build_results.py \
  --benchmark research/local-evidence/runtime.json \
  --output research/local-evidence/GENERATED_RESULTS.md
```

Additional `--benchmark` and `--fuzz-manifest` arguments may be supplied for every retained evidence artifact that matches the generator's input schema. Raw JSON and machine metadata remain the source of truth.
