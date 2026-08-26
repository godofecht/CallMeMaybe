#!/usr/bin/env python3

import argparse
import hashlib
import json
import shlex
import subprocess
import tempfile
from pathlib import Path

from generate import render as render_core
from generate_shapes import render as render_shapes


def run_command(command, cwd):
    return subprocess.run(command, cwd=cwd, text=True, capture_output=True)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description="Compile and compare generated reflection corpora")
    parser.add_argument("--cases", type=int, default=32)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--family", choices=["core", "shapes"], default="core")
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

    renderer = render_core if args.family == "core" else render_shapes
    source = renderer(args.cases, args.seed)
    source_path = args.output_dir / f"corpus-{args.family}-seed-{args.seed}.cpp"
    source_path.write_text(source, encoding="utf-8")

    outputs = {}
    manifest = {
        "family": args.family,
        "seed": args.seed,
        "cases": args.cases,
        "source": str(source_path),
        "source_sha256": sha256_file(source_path),
        "compilers": [],
    }

    with tempfile.TemporaryDirectory(prefix="cmm-reflection-fuzz-") as temp_dir:
        temp = Path(temp_dir)

        for name, compiler_command in compiler_specs:
            binary = temp / f"corpus-{args.family}-{name}"
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

            compile_stdout_path = args.output_dir / f"{name}.compile.stdout.txt"
            compile_stderr_path = args.output_dir / f"{name}.compile.stderr.txt"
            compile_stdout_path.write_text(compiled.stdout, encoding="utf-8")
            compile_stderr_path.write_text(compiled.stderr, encoding="utf-8")
            compiler_record["compile_stdout_sha256"] = sha256_file(compile_stdout_path)
            compiler_record["compile_stderr_sha256"] = sha256_file(compile_stderr_path)

            if compiled.returncode != 0:
                manifest["compilers"].append(compiler_record)
                (args.output_dir / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
                raise SystemExit(f"{name}: compilation failed for {args.family} corpus")

            executed = run_command([str(binary)], repo_root)
            compiler_record["run_returncode"] = executed.returncode

            output_path = args.output_dir / f"{name}.fingerprint.tsv"
            run_stderr_path = args.output_dir / f"{name}.run.stderr.txt"
            output_path.write_text(executed.stdout, encoding="utf-8")
            run_stderr_path.write_text(executed.stderr, encoding="utf-8")
            compiler_record["fingerprint_sha256"] = sha256_file(output_path)
            compiler_record["run_stderr_sha256"] = sha256_file(run_stderr_path)
            manifest["compilers"].append(compiler_record)

            if executed.returncode != 0:
                (args.output_dir / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
                raise SystemExit(f"{name}: generated {args.family} corpus failed its internal invariants")

            outputs[name] = executed.stdout

    reference_name = compiler_specs[0][0]
    reference_output = outputs[reference_name]
    disagreements = []

    for name, _ in compiler_specs[1:]:
        if outputs[name] != reference_output:
            disagreements.append(name)

    manifest["reference"] = reference_name
    manifest["disagreements"] = disagreements
    (args.output_dir / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")

    if disagreements:
        print(f"reflection disagreement in {args.family}: {reference_name} != {', '.join(disagreements)}")
        return_code = 3
    else:
        print(f"all {len(compiler_specs)} compiler fingerprint(s) agree for {args.family} seed {args.seed}")
        return_code = 0

    raise SystemExit(return_code)


if __name__ == "__main__":
    main()
