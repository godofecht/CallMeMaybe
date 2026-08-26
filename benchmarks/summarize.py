#!/usr/bin/env python3

import argparse
import json
from pathlib import Path


def load(path: Path):
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def main():
    parser = argparse.ArgumentParser(description="Summarize CallMeMaybe benchmark JSON")
    parser.add_argument("files", nargs="+", type=Path)
    args = parser.parse_args()

    for path in args.files:
        data = load(path)
        results = data["results"]
        direct = next(result for result in results if result["name"] == "direct")
        baseline = direct["ns_per_op"]

        print(f"## {path}")
        print()
        print(f"Compiler: `{data['compiler']}`  ")
        print(f"Iterations: `{data['iterations']}`  ")
        print(f"Batch: `{data['batch']}`")
        print()
        print("| Dispatch | ns/op | p50 | p95 | p99 | vs direct |")
        print("|---|---:|---:|---:|---:|---:|")

        for result in results:
            ratio = result["ns_per_op"] / baseline if baseline else 0.0
            print(
                f"| {result['name']} | {result['ns_per_op']:.3f} | "
                f"{result['p50_ns']:.3f} | {result['p95_ns']:.3f} | "
                f"{result['p99_ns']:.3f} | {ratio:.2f}x |"
            )

        print()


if __name__ == "__main__":
    main()
