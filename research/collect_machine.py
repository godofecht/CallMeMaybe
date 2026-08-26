#!/usr/bin/env python3

import argparse
import datetime as dt
import hashlib
import json
import platform
import shlex
import shutil
import subprocess
from pathlib import Path


def capture(command, cwd=None):
    completed = subprocess.run(command, cwd=cwd, text=True, capture_output=True)
    if completed.returncode != 0:
        return ""
    return completed.stdout.strip()


def sha256_file(path):
    digest = hashlib.sha256()
    with Path(path).open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def resolve_compiler(command):
    executable = shutil.which(command[0])
    if executable is None:
        raise SystemExit(f"compiler executable not found: {command[0]}")
    return Path(executable).resolve()


def cpu_model():
    system = platform.system()
    if system == "Darwin":
        value = capture(["sysctl", "-n", "machdep.cpu.brand_string"])
        if value:
            return value
        return capture(["sysctl", "-n", "hw.model"])
    if system == "Linux":
        cpuinfo = Path("/proc/cpuinfo")
        if cpuinfo.exists():
            for line in cpuinfo.read_text(encoding="utf-8", errors="replace").splitlines():
                if line.lower().startswith("model name") and ":" in line:
                    return line.split(":", 1)[1].strip()
    return platform.processor()


def main():
    parser = argparse.ArgumentParser(description="Collect reproducibility metadata for CallMeMaybe research runs")
    parser.add_argument("--compiler", required=True, help="compiler command used for the measured build")
    parser.add_argument("--build-type", default="Release")
    parser.add_argument("--notes", default="")
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    repo_root = Path(__file__).resolve().parents[1]
    compiler_command = shlex.split(args.compiler)
    if not compiler_command:
        raise SystemExit("--compiler cannot be empty")

    compiler_executable = resolve_compiler(compiler_command)
    version = capture([str(compiler_executable), *compiler_command[1:], "--version"], cwd=repo_root).splitlines()
    if not version:
        raise SystemExit(f"failed to identify compiler: {compiler_executable}")

    commit_sha = capture(["git", "rev-parse", "HEAD"], cwd=repo_root)
    if not commit_sha:
        raise SystemExit("failed to identify repository commit")

    metadata = {
        "timestamp_utc": dt.datetime.now(dt.timezone.utc).isoformat(),
        "platform": platform.platform(),
        "machine": platform.machine(),
        "processor": platform.processor(),
        "cpu_model": cpu_model(),
        "compiler_command": compiler_command,
        "compiler_executable": str(compiler_executable),
        "compiler_executable_sha256": sha256_file(compiler_executable),
        "compiler_version": version[0],
        "commit_sha": commit_sha,
        "build_type": args.build_type,
        "notes": args.notes,
    }

    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(metadata, indent=2) + "\n", encoding="utf-8")
    print(args.output)


if __name__ == "__main__":
    main()
