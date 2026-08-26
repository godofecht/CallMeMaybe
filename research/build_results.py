#!/usr/bin/env python3

import argparse
import hashlib
import json
from collections import Counter
from pathlib import Path


CAMPAIGN_STATUSES = (
    "ok",
    "disagreement",
    "compile_failure",
    "invariant_failure",
    "runner_failure",
)


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


def is_campaign_manifest(data) -> bool:
    return all(
        field in data
        for field in (
            "requested_seed_count",
            "families",
            "generated_programs",
            "counts",
            "programs",
            "compiler_fingerprints",
        )
    )


def validate_campaign_manifest(data, path: Path):
    require_fields(
        data,
        [
            "requested_seed_count",
            "seed_start",
            "families",
            "cases_per_program",
            "generated_programs",
            "compilers",
            "compiler_fingerprints",
            "counts",
            "programs",
        ],
        path,
    )

    families = data["families"]
    programs = data["programs"]
    requested_seed_count = data["requested_seed_count"]
    if not isinstance(families, list) or not families:
        raise SystemExit(f"{path}: families must be a non-empty list")
    if not isinstance(requested_seed_count, int) or requested_seed_count <= 0:
        raise SystemExit(f"{path}: requested_seed_count must be positive")
    expected_programs = requested_seed_count * len(families)
    if data["generated_programs"] != len(programs):
        raise SystemExit(f"{path}: generated_programs does not match program records")
    if len(programs) != expected_programs:
        raise SystemExit(
            f"{path}: incomplete campaign coverage: got {len(programs)} programs, "
            f"expected {expected_programs}"
        )

    seen = set()
    observed_counts = Counter()
    seed_start = data["seed_start"]
    valid_seeds = range(seed_start, seed_start + requested_seed_count)
    for record in programs:
        require_fields(record, ["family", "seed", "status", "manifest_sha256"], path)
        key = (record["family"], record["seed"])
        if key in seen:
            raise SystemExit(f"{path}: duplicate campaign program {key!r}")
        if record["family"] not in families or record["seed"] not in valid_seeds:
            raise SystemExit(f"{path}: out-of-window campaign program {key!r}")
        if record["status"] not in CAMPAIGN_STATUSES:
            raise SystemExit(f"{path}: unknown campaign status {record['status']!r}")
        digest = record["manifest_sha256"]
        if not isinstance(digest, str) or len(digest) != 64:
            raise SystemExit(f"{path}: invalid manifest_sha256 for campaign program {key!r}")
        try:
            int(digest, 16)
        except ValueError as error:
            raise SystemExit(
                f"{path}: invalid manifest_sha256 for campaign program {key!r}"
            ) from error
        seen.add(key)
        observed_counts[record["status"]] += 1

    for status in CAMPAIGN_STATUSES:
        expected = data["counts"].get(status, 0)
        actual = observed_counts.get(status, 0)
        if expected != actual:
            raise SystemExit(
                f"{path}: status count mismatch for {status}: manifest={expected}, records={actual}"
            )

    fingerprints = data["compiler_fingerprints"]
    if not isinstance(fingerprints, list) or not fingerprints:
        raise SystemExit(f"{path}: compiler_fingerprints must be a non-empty list")
    for fingerprint in fingerprints:
        for field in ("label", "command", "executable", "executable_sha256", "version"):
            if field not in fingerprint:
                raise SystemExit(f"{path}: compiler fingerprint missing {field}")

    return observed_counts


def campaign_fuzz_section(path: Path, data):
    counts = validate_campaign_manifest(data, path)
    compiler_summary = "; ".join(
        f"{item['label']}: {item['version']}" for item in data["compiler_fingerprints"]
    )
    failures = sum(
        counts.get(status, 0)
        for status in ("compile_failure", "invariant_failure", "runner_failure")
    )
    return [
        f"| `{path}` | {data['generated_programs']} | "
        f"{', '.join(data['families'])} | {compiler_summary} | "
        f"{counts.get('ok', 0)} | {counts.get('disagreement', 0)} | {failures} |"
    ]


def single_fuzz_section(path: Path, data):
    require_fields(data, ["seed", "cases"], path)
    disagreements = ", ".join(data.get("disagreements", [])) or "none"
    return [
        f"| `{path}` | {data['seed']} | {data['cases']} | "
        f"{data.get('reference', 'n/a')} | {disagreements} |"
    ]


def fuzz_section(paths):
    lines = ["## Differential reflection results", ""]
    if not paths:
        return lines + ["_No differential-fuzz manifests supplied._", ""]

    campaigns = []
    singles = []
    for path in paths:
        data = load_json(path)
        if is_campaign_manifest(data):
            campaigns.extend(campaign_fuzz_section(path, data))
        else:
            singles.extend(single_fuzz_section(path, data))

    if campaigns:
        lines += [
            "### Campaign manifests",
            "",
            "| Manifest | Programs | Families | Compilers | OK | Disagreements | Failures |",
            "|---|---:|---|---|---:|---:|---:|",
            *campaigns,
            "",
        ]

    if singles:
        lines += [
            "### Single-run manifests",
            "",
            "| Manifest | Seed | Cases | Reference | Disagreements |",
            "|---|---:|---:|---|---|",
            *singles,
            "",
        ]

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
