#!/usr/bin/env python3

import argparse
import hashlib
import json
import shlex
import shutil
import subprocess
import sys
from pathlib import Path


def selected_seeds(seed_start: int, programs: int, shard_index: int, shard_count: int) -> list[int]:
    return [
        seed_start + offset
        for offset in range(programs)
        if offset % shard_count == shard_index
    ]


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def compiler_fingerprints(compilers: list[str]) -> list[dict]:
    fingerprints = []
    for compiler in compilers:
        label, command = compiler.split("=", 1)
        argv = shlex.split(command)
        executable = shutil.which(argv[0])
        if executable is None:
            raise SystemExit(f"compiler executable not found for {label}: {argv[0]}")

        completed = subprocess.run(
            [executable, "--version"],
            text=True,
            capture_output=True,
            timeout=10,
        )
        if completed.returncode != 0:
            diagnostic = (completed.stderr or completed.stdout).strip()
            raise SystemExit(
                f"failed to fingerprint compiler {label}: {diagnostic or completed.returncode}"
            )

        version_lines = (completed.stdout or completed.stderr).strip().splitlines()
        if not version_lines:
            raise SystemExit(f"compiler {label} produced no --version output")

        resolved = str(Path(executable).resolve())
        fingerprints.append({
            "label": label,
            "command": command,
            "executable": resolved,
            "executable_sha256": sha256_file(Path(resolved)),
            "version": version_lines[0],
        })
    return fingerprints


def campaign_identity(requested_seed_count: int, seed_start: int, shard_index: int,
                      shard_count: int, families: list[str], cases_per_program: int,
                      compilers: list[str], compiler_identity: list[dict]) -> dict:
    return {
        "requested_seed_count": requested_seed_count,
        "seed_start": seed_start,
        "shard": {
            "index": shard_index,
            "count": shard_count,
        },
        "families": families,
        "cases_per_program": cases_per_program,
        "compilers": compilers,
        "compiler_fingerprints": compiler_identity,
    }


def validate_record_artifact(record: dict) -> None:
    output_dir = record.get("output_dir")
    expected_digest = record.get("manifest_sha256")
    if not isinstance(output_dir, str) or not output_dir:
        raise SystemExit("cannot resume campaign with record missing output_dir")
    if not isinstance(expected_digest, str) or len(expected_digest) != 64:
        raise SystemExit("cannot resume campaign with record missing manifest_sha256")

    manifest_path = Path(output_dir) / "manifest.json"
    if not manifest_path.is_file():
        raise SystemExit(f"cannot resume campaign with missing retained manifest: {manifest_path}")
    actual_digest = sha256_file(manifest_path)
    if actual_digest != expected_digest:
        raise SystemExit(
            f"cannot resume campaign with mutated retained manifest: {manifest_path}"
        )


def validate_resume_summary(summary: dict, expected_identity: dict,
                            expected_records: set[tuple[str, int]],
                            verify_artifacts: bool = True) -> None:
    for key, expected in expected_identity.items():
        actual = summary.get(key)
        if actual != expected:
            raise SystemExit(
                f"cannot resume incompatible campaign: {key} is {actual!r}, expected {expected!r}"
            )

    seen: set[tuple[str, int]] = set()
    for record in summary.get("programs", []):
        try:
            key = (record["family"], record["seed"])
        except KeyError as error:
            raise SystemExit(
                f"cannot resume malformed campaign: program record missing {error.args[0]!r}"
            ) from error
        if key in seen:
            raise SystemExit(f"cannot resume campaign with duplicate record: {key!r}")
        if key not in expected_records:
            raise SystemExit(f"cannot resume campaign with out-of-window record: {key!r}")
        if verify_artifacts:
            validate_record_artifact(record)
        seen.add(key)


def run_one(run_script: Path, family: str, seed: int, cases: int,
            compilers: list[str], output_dir: Path) -> dict:
    command = [
        sys.executable,
        str(run_script),
        "--family",
        family,
        "--cases",
        str(cases),
        "--seed",
        str(seed),
        "--output-dir",
        str(output_dir),
    ]
    for compiler in compilers:
        command += ["--compiler", compiler]

    completed = subprocess.run(command, text=True, capture_output=True)
    manifest_path = output_dir / "manifest.json"
    manifest = {}
    if manifest_path.exists():
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))

    if completed.returncode == 0:
        status = "ok"
    elif completed.returncode == 3:
        status = "disagreement"
    elif any(record.get("compile_returncode", 0) != 0 for record in manifest.get("compilers", [])):
        status = "compile_failure"
    elif any(record.get("run_returncode", 0) != 0 for record in manifest.get("compilers", [])):
        status = "invariant_failure"
    else:
        status = "runner_failure"

    return {
        "family": family,
        "seed": seed,
        "cases": cases,
        "status": status,
        "returncode": completed.returncode,
        "output_dir": str(output_dir),
        "manifest_sha256": sha256_file(manifest_path) if manifest_path.exists() else None,
        "stdout": completed.stdout.strip(),
        "stderr": completed.stderr.strip(),
    }


def main() -> None:
    parser = argparse.ArgumentParser(description="Run a resumable CallMeMaybe differential-reflection campaign")
    parser.add_argument("--programs", type=int, default=100,
                        help="number of seeds in the global campaign window; each selected family runs once per seed")
    parser.add_argument("--cases-per-program", type=int, default=1)
    parser.add_argument("--seed-start", type=int, default=1)
    parser.add_argument("--family", choices=["core", "shapes", "both"], default="both")
    parser.add_argument("--compiler", action="append", required=True)
    parser.add_argument("--output-dir", type=Path, default=Path("reflection-fuzz-campaign"))
    parser.add_argument("--resume", action="store_true")
    parser.add_argument("--shard-count", type=int, default=1,
                        help="split the global seed window into this many deterministic shards")
    parser.add_argument("--shard-index", type=int, default=0,
                        help="zero-based shard to execute; seeds are assigned by global offset modulo shard count")
    args = parser.parse_args()

    if args.programs <= 0 or args.cases_per_program <= 0:
        raise SystemExit("--programs and --cases-per-program must be positive")
    if args.shard_count <= 0:
        raise SystemExit("--shard-count must be positive")
    if args.shard_index < 0 or args.shard_index >= args.shard_count:
        raise SystemExit("--shard-index must be in [0, --shard-count)")

    for compiler in args.compiler:
        if "=" not in compiler:
            raise SystemExit(f"invalid --compiler value: {compiler}")
        _, command = compiler.split("=", 1)
        if not shlex.split(command):
            raise SystemExit(f"empty compiler command: {compiler}")

    fingerprints = compiler_fingerprints(args.compiler)
    families = ["core", "shapes"] if args.family == "both" else [args.family]
    seeds = selected_seeds(args.seed_start, args.programs, args.shard_index, args.shard_count)
    identity = campaign_identity(
        args.programs,
        args.seed_start,
        args.shard_index,
        args.shard_count,
        families,
        args.cases_per_program,
        args.compiler,
        fingerprints,
    )
    expected_records = {(family, seed) for seed in seeds for family in families}

    args.output_dir.mkdir(parents=True, exist_ok=True)
    run_script = Path(__file__).with_name("run.py")
    summary_path = args.output_dir / "campaign.json"

    previous = {}
    if args.resume and summary_path.exists():
        prior_summary = json.loads(summary_path.read_text(encoding="utf-8"))
        validate_resume_summary(prior_summary, identity, expected_records)
        for record in prior_summary.get("programs", []):
            previous[(record["family"], record["seed"])] = record

    records = []
    counts = {
        "ok": 0,
        "disagreement": 0,
        "compile_failure": 0,
        "invariant_failure": 0,
        "runner_failure": 0,
    }

    for seed in seeds:
        for family in families:
            key = (family, seed)
            if key in previous:
                record = previous[key]
            else:
                program_dir = args.output_dir / family / f"seed-{seed:08d}"
                program_dir.mkdir(parents=True, exist_ok=True)
                record = run_one(
                    run_script,
                    family,
                    seed,
                    args.cases_per_program,
                    args.compiler,
                    program_dir,
                )
            records.append(record)
            counts[record["status"]] += 1

            summary = {
                **identity,
                "selected_seed_count": len(seeds),
                "generated_programs": len(records),
                "counts": counts,
                "programs": records,
            }
            summary_path.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")

    print(json.dumps({
        "generated_programs": len(records),
        "selected_seed_count": len(seeds),
        "shard": {
            "index": args.shard_index,
            "count": args.shard_count,
        },
        "compiler_fingerprints": fingerprints,
        "counts": counts,
        "summary": str(summary_path),
    }, indent=2))

    if counts["disagreement"]:
        raise SystemExit(3)
    if counts["compile_failure"] or counts["invariant_failure"] or counts["runner_failure"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
