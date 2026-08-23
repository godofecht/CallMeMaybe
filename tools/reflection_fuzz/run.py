#!/usr/bin/env python3

import argparse
import json
import shlex
import subprocess
import tempfile
from pathlib import Path

from generate import render


def run_command(command, cwd):
    return subprocess.run(command, cwd=cwd, text=True, capture_output=True)


def main() -> None:
    parser = argparse.ArgumentParser(description="Compile and compare generated reflection corpora")
    parser.add_argument("--cases", type=int, default=32)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument(
        "--compiler",
        action="append",
        required=True,
        help="NAME=compiler command, e.g. gcc='g++ -std=c++26 -freflection'",
    )
    parser.add_argument("--output-dir", type=Path, default=Path("reflection-fuzz-results"))
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[2]
    args.output_dir.mkdir(parents=True, exist_ok=True)

    compiler_specs = []
    for item in args.compiler:
        if "=" not in item:
            raise SystemExit(f"invalid --compiler value: {item}")
        name, command = item.split("=", 1)
        compiler_specs.append((name, shlex.split(command)))

    source = render(args.cases, args.seed)
    source_path = args.output_dir / f"corpus-seed-{args.seed}.cpp"
    source_path.write_text(source, encoding="utf-8")

    outputs = {}
    manifest = {
        "seed": args.seed,
        "cases": args.cases,
        "compilers": [],
    }

    with tempfile.TemporaryDirectory(prefix="cmm-reflection-fuzz-") as temp_dir:
        temp = Path(temp_dir)

        for name, compiler_command in compiler_specs:
            binary = temp / f"corpus-{name}"
            compile_command = [
                *compiler_command,
                str(source_path),
                "-I",
                str(repo_root / "include"),
                "-O0",
                "-o",
                str(binary),
            ]

            compiled = run_command(compile_command, repo_root)
            compiler_record = {
                "name": name,
                "command": compile_command,
                "compile_returncode": compiled.returncode,
            }

            (args.output_dir / f"{name}.compile.stdout.txt").write_text(compiled.stdout, encoding="utf-8")
            (args.output_dir / f"{name}.compile.stderr.txt").write_text(compiled.stderr, encoding="utf-8")

            if compiled.returncode != 0:
                manifest["compilers"].append(compiler_record)
                (args.output_dir / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
                raise SystemExit(f"{name}: compilation failed")

            executed = run_command([str(binary)], repo_root)
            compiler_record["run_returncode"] = executed.returncode
            manifest["compilers"].append(compiler_record)

            output_path = args.output_dir / f"{name}.fingerprint.tsv"
            output_path.write_text(executed.stdout, encoding="utf-8")
            (args.output_dir / f"{name}.run.stderr.txt").write_text(executed.stderr, encoding="utf-8")

            if executed.returncode != 0:
                (args.output_dir / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
                raise SystemExit(f"{name}: generated corpus failed its internal invariants")

            outputs[name] = executed.stdout

    reference_name = compiler_specs[0][0]
    reference_output = outputs[reference_name]
    disagreements = []

    for name, _ in compiler_specs[1:]:
        if outputs[name] != reference_output:
            disagreements.append(name)

    manifest["reference"] = reference_name
    manifest["disagreements"] = disagreements
    (args.output_dir / "manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")

    if disagreements:
        print(f"reflection disagreement: {reference_name} != {', '.join(disagreements)}")
        return_code = 3
    else:
        print(f"all {len(compiler_specs)} compiler fingerprint(s) agree for seed {args.seed}")
        return_code = 0

    raise SystemExit(return_code)


if __name__ == "__main__":
    main()
