#!/usr/bin/env python3

import argparse
import json
from pathlib import Path


def load_json(path: Path):
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def benchmark_section(paths):
    lines = ["## Runtime dispatch results", ""]
    if not paths:
        return lines + ["_No benchmark result files supplied._", ""]

    for path in paths:
        data = load_json(path)
        lines += [f"### `{path}`", "", f"Compiler: `{data['compiler']}`", ""]
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
        disagreements = ", ".join(data.get("disagreements", [])) or "none"
        lines.append(
            f"| `{path}` | {data['seed']} | {data['cases']} | "
            f"{data.get('reference', 'n/a')} | {disagreements} |"
        )
    lines.append("")
    return lines


def main():
    parser = argparse.ArgumentParser(description="Build evidence-backed CallMeMaybe result tables")
    parser.add_argument("--benchmark", action="append", type=Path, default=[])
    parser.add_argument("--fuzz-manifest", action="append", type=Path, default=[])
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    lines = [
        "# Generated CallMeMaybe Results",
        "",
        "This file is generated from committed machine-readable evidence. Do not edit numerical results by hand.",
        "",
    ]
    lines += benchmark_section(args.benchmark)
    lines += fuzz_section(args.fuzz_manifest)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text("\n".join(lines) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
