#!/usr/bin/env python3

import argparse
import hashlib
import json
from pathlib import Path


def load_json(path: Path):
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def require_fields(data, fields, path: Path):
    missing = [field for field in fields if field not in data]
    if missing:
        raise SystemExit(f"{path}: missing required fields: {', '.join(missing)}")


def load_machine_metadata(path: Path):
    data = load_json(path)
    require_fields(
        data,
        [
            "timestamp_utc",
            "platform",
            "machine",
            "cpu_model",
            "compiler_command",
            "compiler_version",
            "commit_sha",
            "build_type",
        ],
        path,
    )
    if data["compiler_version"] == "unknown":
        raise SystemExit(f"{path}: compiler_version is unknown")
    if data["commit_sha"] == "unknown":
        raise SystemExit(f"{path}: commit_sha is unknown")
    return data


def benchmark_section(pairs):
    lines = ["## Runtime dispatch results", ""]
    if not pairs:
        return lines + ["_No benchmark result files supplied._", ""]

    for benchmark_path, machine_path in pairs:
        data = load_json(benchmark_path)
        machine = load_machine_metadata(machine_path)
        require_fields(data, ["compiler", "results"], benchmark_path)
        lines += [
            f"### `{benchmark_path}`",
            "",
            f"Compiler: `{data['compiler']}`",
            "",
            f"Machine metadata: `{machine_path}`",
            "",
            f"Machine: `{machine['cpu_model']}` / `{machine['platform']}`",
            "",
            f"Measured commit: `{machine['commit_sha']}`",
            "",
        ]
        lines += ["| Dispatch | ns/op | p50 | p95 | p99 |", "|---|---:|---:|---:|---:|"]
        for result in data["results"]:
            lines.append(
                f"| {result['name']} | {result['ns_per_op']:.3f} | "
                f"{result['p50_ns']:.3f} | {result['p95_ns']:.3f} | {result['p99_ns']:.3f} |"
            )
        lines.append("")
    return lines


def fuzz_section(paths):
    lines = ["## Differential reflection results", ""]
    if not paths:
        return lines + ["_No differential-fuzz manifests supplied._", ""]

    lines += ["| Manifest | Seed | Cases | Reference | Disagreements |", "|---|---:|---:|---|---|"]
    for path in paths:
        data = load_json(path)
        require_fields(data, ["seed", "cases"], path)
        disagreements = ", ".join(data.get("disagreements", [])) or "none"
        lines.append(
            f"| `{path}` | {data['seed']} | {data['cases']} | "
            f"{data.get('reference', 'n/a')} | {disagreements} |"
        )
    lines.append("")
    return lines


def provenance_section(benchmark_pairs, fuzz_paths):
    artifacts = []
    for benchmark_path, machine_path in benchmark_pairs:
        artifacts.append(("benchmark", benchmark_path))
        artifacts.append(("machine metadata", machine_path))
    artifacts.extend(("fuzz manifest", path) for path in fuzz_paths)

    lines = ["## Evidence provenance", ""]
    if not artifacts:
        return lines + ["_No evidence artifacts supplied._", ""]

    lines += ["| Kind | Artifact | SHA-256 |", "|---|---|---|"]
    for kind, path in artifacts:
        lines.append(f"| {kind} | `{path}` | `{sha256_file(path)}` |")
    lines.append("")
    return lines


def main():
    parser = argparse.ArgumentParser(description="Build evidence-backed CallMeMaybe result tables")
    parser.add_argument("--benchmark", action="append", type=Path, default=[])
    parser.add_argument(
        "--machine-metadata",
        action="append",
        type=Path,
        default=[],
        help="machine metadata paired by position with each --benchmark",
    )
    parser.add_argument("--fuzz-manifest", action="append", type=Path, default=[])
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    if len(args.benchmark) != len(args.machine_metadata):
        raise SystemExit(
            "each --benchmark requires exactly one paired --machine-metadata "
            f"(got {len(args.benchmark)} benchmark(s), {len(args.machine_metadata)} metadata file(s))"
        )

    benchmark_pairs = list(zip(args.benchmark, args.machine_metadata))
    lines = [
        "# Generated CallMeMaybe Results",
        "",
        "This file is generated from committed machine-readable evidence. Do not edit numerical results by hand.",
        "",
    ]
    lines += benchmark_section(benchmark_pairs)
    lines += fuzz_section(args.fuzz_manifest)
    lines += provenance_section(benchmark_pairs, args.fuzz_manifest)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
