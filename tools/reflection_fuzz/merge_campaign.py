#!/usr/bin/env python3

import argparse
import json
import string
from collections import Counter
from pathlib import Path


STATUSES = ("ok", "disagreement", "compile_failure", "invariant_failure", "runner_failure")


def load(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def campaign_key(data: dict) -> tuple:
    return (
        data["requested_seed_count"],
        data["seed_start"],
        tuple(data["families"]),
        data["cases_per_program"],
        tuple(data["compilers"]),
        data["shard"]["count"],
    )


def expected_keys(seed_start: int, requested_seed_count: int, families: list[str]) -> set[tuple[str, int]]:
    return {
        (family, seed)
        for seed in range(seed_start, seed_start + requested_seed_count)
        for family in families
    }


def valid_sha256(value) -> bool:
    return (
        isinstance(value, str)
        and len(value) == 64
        and all(character in string.hexdigits for character in value)
    )


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Verify and merge deterministic differential-reflection campaign shards"
    )
    parser.add_argument("shards", nargs="+", type=Path, help="campaign.json files, one per shard")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    shards = [load(path) for path in args.shards]
    reference = campaign_key(shards[0])

    for path, data in zip(args.shards, shards):
        if campaign_key(data) != reference:
            raise SystemExit(f"incompatible campaign metadata: {path}")

    shard_count = shards[0]["shard"]["count"]
    shard_indices = [data["shard"]["index"] for data in shards]
    if len(shards) != shard_count:
        raise SystemExit(f"expected {shard_count} shard manifests, received {len(shards)}")
    if sorted(shard_indices) != list(range(shard_count)):
        raise SystemExit("shard indexes must cover every index exactly once")

    requested_seed_count = shards[0]["requested_seed_count"]
    seed_start = shards[0]["seed_start"]
    families = shards[0]["families"]
    cases_per_program = shards[0]["cases_per_program"]
    compilers = shards[0]["compilers"]

    records = []
    seen = set()
    for path, data in zip(args.shards, shards):
        expected_selected = sum(
            1
            for offset in range(requested_seed_count)
            if offset % shard_count == data["shard"]["index"]
        )
        if data["selected_seed_count"] != expected_selected:
            raise SystemExit(f"incorrect selected_seed_count in {path}")
        if data["generated_programs"] != len(data["programs"]):
            raise SystemExit(f"generated_programs does not match records in {path}")

        local_counts = Counter(record["status"] for record in data["programs"])
        for status in local_counts:
            if status not in STATUSES:
                raise SystemExit(f"unknown status {status!r} in {path}")
        for status in STATUSES:
            if data["counts"].get(status, 0) != local_counts.get(status, 0):
                raise SystemExit(f"status count mismatch for {status} in {path}")

        for record in data["programs"]:
            key = (record["family"], record["seed"])
            if key in seen:
                raise SystemExit(f"duplicate campaign program {key}")
            if not valid_sha256(record.get("manifest_sha256")):
                raise SystemExit(f"program {key} in {path} is missing a valid manifest_sha256")
            seen.add(key)
            records.append(record)

    expected = expected_keys(seed_start, requested_seed_count, families)
    missing = expected - seen
    unexpected = seen - expected
    if missing or unexpected:
        raise SystemExit(
            f"campaign coverage mismatch: missing={len(missing)} unexpected={len(unexpected)}"
        )

    records.sort(key=lambda record: (record["seed"], families.index(record["family"])))
    counts = Counter(record["status"] for record in records)
    merged = {
        "requested_seed_count": requested_seed_count,
        "selected_seed_count": requested_seed_count,
        "seed_start": seed_start,
        "shard": {"index": "merged", "count": shard_count},
        "families": families,
        "cases_per_program": cases_per_program,
        "generated_programs": len(records),
        "compilers": compilers,
        "counts": {status: counts.get(status, 0) for status in STATUSES},
        "programs": records,
        "source_shards": [str(path) for path in args.shards],
    }

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(merged, indent=2) + "\n", encoding="utf-8")

    print(json.dumps({
        "generated_programs": len(records),
        "counts": merged["counts"],
        "output": str(args.output),
    }, indent=2))

    if counts["disagreement"]:
        raise SystemExit(3)
    if counts["compile_failure"] or counts["invariant_failure"] or counts["runner_failure"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
