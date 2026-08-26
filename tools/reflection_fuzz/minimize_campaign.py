#!/usr/bin/env python3

import argparse
import json
import subprocess
import sys
from pathlib import Path


def load_campaign(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as handle:
        campaign = json.load(handle)

    compilers = campaign.get("compilers")
    if not isinstance(compilers, list) or len(compilers) < 2:
        raise SystemExit("campaign manifest must retain at least two compiler commands")

    programs = campaign.get("programs")
    if not isinstance(programs, list):
        raise SystemExit("campaign manifest is missing the programs array")

    disagreements = [record for record in programs if record.get("status") == "disagreement"]
    expected = campaign.get("counts", {}).get("disagreement")
    if expected is not None and expected != len(disagreements):
        raise SystemExit(
            f"campaign disagreement count is inconsistent: counts says {expected}, "
            f"records contain {len(disagreements)}"
        )

    return campaign


def minimize_one(minimize_script: Path, record: dict, compilers: list[str],
                 output_dir: Path) -> dict:
    try:
        family = record["family"]
        seed = int(record["seed"])
        cases = int(record["cases"])
    except (KeyError, TypeError, ValueError) as error:
        raise SystemExit(f"malformed disagreement record: {record!r}") from error

    if family not in {"core", "shapes"}:
        raise SystemExit(f"unsupported disagreement family: {family!r}")
    if cases <= 0:
        raise SystemExit(f"invalid disagreement case count for {family} seed {seed}: {cases}")

    target = output_dir / family / f"seed-{seed:08d}"
    command = [
        sys.executable,
        str(minimize_script),
        "--family",
        family,
        "--max-cases",
        str(cases),
        "--seed",
        str(seed),
        "--output-dir",
        str(target),
    ]
    for compiler in compilers:
        command += ["--compiler", compiler]

    completed = subprocess.run(command, text=True, capture_output=True)
    manifest_path = target / "manifest.json"

    result = {
        "family": family,
        "seed": seed,
        "source_cases": cases,
        "status": "minimized" if completed.returncode == 0 else "minimization_failure",
        "returncode": completed.returncode,
        "output_dir": str(target),
        "manifest": str(manifest_path) if manifest_path.exists() else None,
        "stdout": completed.stdout.strip(),
        "stderr": completed.stderr.strip(),
    }

    if manifest_path.exists():
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
        result["retained_disagreements"] = manifest.get("disagreements", [])

    return result


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Minimize every semantic disagreement retained by a differential campaign"
    )
    parser.add_argument("campaign", type=Path)
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("reflection-fuzz-minimized-campaign"),
    )
    args = parser.parse_args()

    campaign = load_campaign(args.campaign)
    disagreements = [
        record for record in campaign["programs"] if record.get("status") == "disagreement"
    ]

    args.output_dir.mkdir(parents=True, exist_ok=True)
    summary_path = args.output_dir / "minimized_campaign.json"

    results = []
    minimize_script = Path(__file__).with_name("minimize.py")
    for record in sorted(disagreements, key=lambda item: (int(item["seed"]), item["family"])):
        result = minimize_one(
            minimize_script,
            record,
            campaign["compilers"],
            args.output_dir,
        )
        results.append(result)
        summary = {
            "source_campaign": str(args.campaign),
            "compilers": campaign["compilers"],
            "source_disagreement_count": len(disagreements),
            "minimized_count": sum(item["status"] == "minimized" for item in results),
            "failure_count": sum(item["status"] != "minimized" for item in results),
            "results": results,
        }
        summary_path.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")

    if not disagreements:
        summary = {
            "source_campaign": str(args.campaign),
            "compilers": campaign["compilers"],
            "source_disagreement_count": 0,
            "minimized_count": 0,
            "failure_count": 0,
            "results": [],
        }
        summary_path.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")

    print(json.dumps({
        "source_disagreement_count": len(disagreements),
        "minimized_count": sum(item["status"] == "minimized" for item in results),
        "failure_count": sum(item["status"] != "minimized" for item in results),
        "summary": str(summary_path),
    }, indent=2))

    if any(item["status"] != "minimized" for item in results):
        raise SystemExit(1)


if __name__ == "__main__":
    main()
