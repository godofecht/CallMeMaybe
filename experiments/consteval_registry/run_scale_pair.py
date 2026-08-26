#!/usr/bin/env python3

import argparse
import json
import shlex
import shutil
import subprocess
import time
from pathlib import Path


def compile_one(compiler: list[str], source: Path, output: Path, include_dir: Path) -> dict:
    command = [*compiler, "-std=c++26", "-freflection", "-O3", "-DNDEBUG",
               "-I", str(include_dir), str(source), "-o", str(output)]
    begin = time.perf_counter_ns()
    completed = subprocess.run(command, text=True, capture_output=True)
    elapsed_ns = time.perf_counter_ns() - begin
    return {
        "command": command,
        "returncode": completed.returncode,
        "compile_wall_ns": elapsed_ns,
        "stdout": completed.stdout,
        "stderr": completed.stderr,
    }


def execute(binary: Path) -> dict:
    completed = subprocess.run([str(binary)], text=True, capture_output=True)
    result = {
        "returncode": completed.returncode,
        "stdout": completed.stdout,
        "stderr": completed.stderr,
    }
    if completed.returncode == 0:
        result["metrics"] = json.loads(completed.stdout)
    return result


def stripped_size(binary: Path) -> int | None:
    strip = shutil.which("strip")
    if strip is None:
        return None
    copy = binary.with_suffix(binary.suffix + ".stripped")
    shutil.copy2(binary, copy)
    completed = subprocess.run([strip, str(copy)], text=True, capture_output=True)
    if completed.returncode != 0:
        copy.unlink(missing_ok=True)
        return None
    size = copy.stat().st_size
    copy.unlink(missing_ok=True)
    return size


def main() -> None:
    parser = argparse.ArgumentParser(description="Run matched runtime/consteval registry scaling measurements")
    parser.add_argument("--compiler", default="g++", help="compiler command, e.g. 'g++-16'")
    parser.add_argument("--counts", nargs="+", type=int, default=[10, 100, 1000, 10000])
    parser.add_argument("--repo", type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--json", type=Path, required=True)
    args = parser.parse_args()

    if any(count <= 0 for count in args.counts):
        raise SystemExit("all --counts must be positive")

    compiler = shlex.split(args.compiler)
    if not compiler:
        raise SystemExit("--compiler must not be empty")

    generator = Path(__file__).with_name("generate_scale_pair.py")
    include_dir = args.repo / "include"
    args.output_dir.mkdir(parents=True, exist_ok=True)
    records = []

    for count in args.counts:
        case_dir = args.output_dir / str(count)
        subprocess.run([
            "python3", str(generator), "--count", str(count), "--output-dir", str(case_dir)
        ], check=True)

        record = {"entity_count": count, "backends": {}}
        for backend in ("runtime", "consteval"):
            source = case_dir / f"{backend}.cpp"
            binary = case_dir / backend
            compile_result = compile_one(compiler, source, binary, include_dir)
            backend_result = {"compile": compile_result}
            if compile_result["returncode"] == 0:
                backend_result["binary_size_bytes"] = binary.stat().st_size
                backend_result["stripped_size_bytes"] = stripped_size(binary)
                backend_result["run"] = execute(binary)
            record["backends"][backend] = backend_result
        records.append(record)

        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(json.dumps({
            "compiler": compiler,
            "counts": args.counts,
            "records": records,
        }, indent=2) + "\n", encoding="utf-8")

    failed = False
    for record in records:
        for backend in record["backends"].values():
            if backend["compile"]["returncode"] != 0:
                failed = True
            elif backend["run"]["returncode"] != 0:
                failed = True
    if failed:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
