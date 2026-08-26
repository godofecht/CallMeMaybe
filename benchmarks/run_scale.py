#!/usr/bin/env python3

import argparse
import json
import shlex
import shutil
import subprocess
import tempfile
from pathlib import Path

from generate_scale import render


def compile_with_metrics(command, source, binary, repo_root):
    time_tool = shutil.which("time") or "/usr/bin/time"
    metrics_file = binary.with_suffix(".compile-metrics.txt")
    compile_command = [
        time_tool,
        "-f",
        "%e %M",
        "-o",
        str(metrics_file),
        *command,
        str(source),
        "-I",
        str(repo_root / "include"),
        "-O3",
        "-DNDEBUG",
        "-o",
        str(binary),
    ]
    completed = subprocess.run(compile_command, cwd=repo_root, text=True, capture_output=True)
    if completed.returncode != 0:
        raise RuntimeError(f"compile failed for {source}:\n{completed.stderr}")

    elapsed_s, peak_kib = metrics_file.read_text(encoding="utf-8").strip().split()
    return float(elapsed_s), int(peak_kib), completed


def main():
    parser = argparse.ArgumentParser(description="Measure CallMeMaybe registration scaling")
    parser.add_argument("--compiler", required=True, help="compiler command, e.g. 'g++-16 -std=c++26 -freflection'")
    parser.add_argument("--counts", default="10,100,1000,10000")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[1]
    compiler = shlex.split(args.compiler)
    counts = [int(value) for value in args.counts.split(",") if value]
    results = []

    with tempfile.TemporaryDirectory(prefix="cmm-scale-") as temporary:
        temp = Path(temporary)
        for count in counts:
            source = temp / f"scale-{count}.cpp"
            binary = temp / f"scale-{count}"
            source.write_text(render(count), encoding="utf-8")

            compile_seconds, peak_kib, _ = compile_with_metrics(compiler, source, binary, repo_root)
            executed = subprocess.run([str(binary)], cwd=repo_root, text=True, capture_output=True)
            if executed.returncode != 0:
                raise RuntimeError(f"scale run failed at {count}:\n{executed.stderr}")

            runtime = json.loads(executed.stdout)
            runtime.update(
                {
                    "compile_seconds": compile_seconds,
                    "compiler_peak_kib": peak_kib,
                    "binary_bytes": binary.stat().st_size,
                }
            )

            stripped = temp / f"scale-{count}.stripped"
            shutil.copy2(binary, stripped)
            strip_tool = shutil.which("strip")
            if strip_tool:
                stripped_result = subprocess.run([strip_tool, str(stripped)], text=True, capture_output=True)
                runtime["stripped_binary_bytes"] = stripped.stat().st_size if stripped_result.returncode == 0 else None
            else:
                runtime["stripped_binary_bytes"] = None

            results.append(runtime)
            print(
                f"{count}: compile={compile_seconds:.3f}s peak={peak_kib}KiB "
                f"binary={runtime['binary_bytes']}B registration={runtime['registration_ns_per_entity']:.3f}ns/entity"
            )

    output = {
        "compiler_command": compiler,
        "counts": counts,
        "results": results,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(output, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
