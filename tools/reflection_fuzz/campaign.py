#!/usr/bin/env python3

import argparse
import json
import shlex
import subprocess
import sys
from pathlib import Path


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
        "stdout": completed.stdout.strip(),
        "stderr": completed.stderr.strip(),
    }


def main() -> None:
    parser = argparse.ArgumentParser(description="Run a resumable CallMeMaybe differential-reflection campaign")
    parser.add_argument("--programs", type=int, default=100)
    parser.add_argument("--cases-per-program", type=int, default=1)
    parser.add_argument("--seed-start", type=int, default=1)
    parser.add_argument("--family", choices=["core", "shapes", "both"], default="both")
    parser.add_argument("--compiler", action="append", required=True)
    parser.add_argument("--output-dir", type=Path, default=Path("reflection-fuzz-campaign"))
    parser.add_argument("--resume", action="store_true")
    args = parser.parse_args()

    if args.programs <= 0 or args.cases_per_program <= 0:
        raise SystemExit("--programs and --cases-per-program must be positive")

    for compiler in args.compiler:
        if "=" not in compiler:
            raise SystemExit(f"invalid --compiler value: {compiler}")
        _, command = compiler.split("=", 1)
        if not shlex.split(command):
            raise SystemExit(f"empty compiler command: {compiler}")

    families = ["core", "shapes"] if args.family == "both" else [args.family]
    args.output_dir.mkdir(parents=True, exist_ok=True)
    run_script = Path(__file__).with_name("run.py")
    summary_path = args.output_dir / "campaign.json"

    previous = {}
    if args.resume and summary_path.exists():
        for record in json.loads(summary_path.read_text(encoding="utf-8")).get("programs", []):
            previous[(record["family"], record["seed"])] = record

    records = []
    counts = {
        "ok": 0,
        "disagreement": 0,
        "compile_failure": 0,
        "invariant_failure": 0,
        "runner_failure": 0,
    }

    for offset in range(args.programs):
        seed = args.seed_start + offset
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
                "requested_seed_count": args.programs,
                "families": families,
                "cases_per_program": args.cases_per_program,
                "generated_programs": len(records),
                "compilers": args.compiler,
                "counts": counts,
                "programs": records,
            }
            summary_path.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")

    print(json.dumps({
        "generated_programs": len(records),
        "counts": counts,
        "summary": str(summary_path),
    }, indent=2))

    if counts["disagreement"]:
        raise SystemExit(3)
    if counts["compile_failure"] or counts["invariant_failure"] or counts["runner_failure"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
