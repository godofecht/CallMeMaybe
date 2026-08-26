#!/usr/bin/env python3

import tempfile
from pathlib import Path

from campaign import campaign_identity, sha256_file, validate_record_artifact, validate_resume_summary


def expect_rejected(summary: dict, identity: dict,
                    expected_records: set[tuple[str, int]]) -> None:
    try:
        validate_resume_summary(summary, identity, expected_records, verify_artifacts=False)
    except SystemExit:
        return
    raise AssertionError("incompatible resume manifest was accepted")


def expect_artifact_rejected(record: dict) -> None:
    try:
        validate_record_artifact(record)
    except SystemExit:
        return
    raise AssertionError("mutated or incomplete retained artifact was accepted")


def main() -> None:
    identity = campaign_identity(
        4,
        1,
        0,
        2,
        ["core", "shapes"],
        1,
        ["gcc=g++-16 -std=c++26 -freflection", "clang=clang++ -std=c++26 -freflection-latest"],
    )
    expected_records = {
        (family, seed)
        for seed in (1, 3)
        for family in ("core", "shapes")
    }

    compatible = {
        **identity,
        "programs": [
            {"family": "core", "seed": 1},
            {"family": "shapes", "seed": 1},
        ],
    }
    validate_resume_summary(compatible, identity, expected_records, verify_artifacts=False)

    expect_rejected(
        {**compatible, "compilers": ["gcc=g++-16 -std=c++26 -freflection"]},
        identity,
        expected_records,
    )
    expect_rejected(
        {**compatible, "seed_start": 2},
        identity,
        expected_records,
    )
    expect_rejected(
        {
            **identity,
            "programs": [
                {"family": "core", "seed": 1},
                {"family": "core", "seed": 1},
            ],
        },
        identity,
        expected_records,
    )
    expect_rejected(
        {**identity, "programs": [{"family": "core", "seed": 2}]},
        identity,
        expected_records,
    )

    with tempfile.TemporaryDirectory(prefix="cmm-campaign-resume-test-") as temp_dir:
        output_dir = Path(temp_dir) / "core" / "seed-00000001"
        output_dir.mkdir(parents=True)
        manifest = output_dir / "manifest.json"
        manifest.write_text('{"seed": 1}\n', encoding="utf-8")
        record = {
            "family": "core",
            "seed": 1,
            "output_dir": str(output_dir),
            "manifest_sha256": sha256_file(manifest),
        }
        validate_record_artifact(record)

        manifest.write_text('{"seed": 1, "mutated": true}\n', encoding="utf-8")
        expect_artifact_rejected(record)
        expect_artifact_rejected({"family": "core", "seed": 1, "output_dir": str(output_dir)})


if __name__ == "__main__":
    main()
