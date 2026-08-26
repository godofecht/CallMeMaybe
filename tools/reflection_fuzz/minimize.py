#!/usr/bin/env python3

import argparse
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path


def run_prefix(run_script, family, cases, seed, compilers, output_dir):
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
    return subprocess.run(command, text=True, capture_output=True)


def main():
    parser = argparse.ArgumentParser(description="Minimize a reflection disagreement to the smallest generated prefix")
    parser.add_argument("--family", choices=["core", "shapes"], default="core")
    parser.add_argument("--max-cases", type=int, required=True)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--compiler", action="append", required=True)
    parser.add_argument("--output-dir", type=Path, default=Path("reflection-fuzz-minimized"))
    args = parser.parse_args()

    if args.max_cases <= 0:
        raise SystemExit("--max-cases must be positive")
    if len(args.compiler) < 2:
        raise SystemExit("minimization requires at least two compilers")

    run_script = Path(__file__).with_name("run.py")

    with tempfile.TemporaryDirectory(prefix="cmm-reflection-minimize-") as temporary:
        temp = Path(temporary)

        probe = run_prefix(
            run_script,
            args.family,
            args.max_cases,
            args.seed,
            args.compiler,
            temp / "probe",
        )
        if probe.returncode != 3:
            sys.stderr.write(probe.stdout)
            sys.stderr.write(probe.stderr)
            raise SystemExit("the supplied corpus does not reproduce a semantic disagreement")

        low = 1
        high = args.max_cases
        while low < high:
            mid = (low + high) // 2
            result = run_prefix(
                run_script,
                args.family,
                mid,
                args.seed,
                args.compiler,
                temp / f"prefix-{mid}",
            )
            if result.returncode == 3:
                high = mid
            elif result.returncode == 0:
                low = mid + 1
            else:
                sys.stderr.write(result.stdout)
                sys.stderr.write(result.stderr)
                raise SystemExit(f"prefix {mid} failed for a reason other than semantic disagreement")

        if args.output_dir.exists():
            shutil.rmtree(args.output_dir)
        final_result = run_prefix(
            run_script,
            args.family,
            low,
            args.seed,
            args.compiler,
            args.output_dir,
        )
        if final_result.returncode != 3:
            sys.stderr.write(final_result.stdout)
            sys.stderr.write(final_result.stderr)
            raise SystemExit("minimal prefix stopped reproducing during final capture")

        print(f"smallest disagreeing prefix: {low} case(s), family {args.family}, seed {args.seed}")
        print(f"retained artifacts: {args.output_dir}")


if __name__ == "__main__":
    main()
