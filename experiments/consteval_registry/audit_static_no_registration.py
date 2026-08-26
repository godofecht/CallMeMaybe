#!/usr/bin/env python3
"""Reject startup registration calls from the static-backend regression corpus."""

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path

CALL = re.compile(r"\bregister_rrefl\s*\(")


def strip_comments_and_literals(source: str) -> str:
    out: list[str] = []
    i = 0
    n = len(source)
    while i < n:
        if source.startswith("//", i):
            end = source.find("\n", i + 2)
            if end < 0:
                break
            out.append("\n")
            i = end + 1
            continue
        if source.startswith("/*", i):
            end = source.find("*/", i + 2)
            if end < 0:
                raise ValueError("unterminated block comment")
            out.extend("\n" for c in source[i:end + 2] if c == "\n")
            i = end + 2
            continue
        if source[i] in {'"', "'"}:
            quote = source[i]
            out.append(" ")
            i += 1
            while i < n:
                if source[i] == "\\":
                    i += 2
                    continue
                if source[i] == quote:
                    i += 1
                    break
                if source[i] == "\n":
                    out.append("\n")
                i += 1
            else:
                raise ValueError("unterminated string/character literal")
            continue
        out.append(source[i])
        i += 1
    return "".join(out)


def find_calls(path: Path) -> list[int]:
    source = strip_comments_and_literals(path.read_text(encoding="utf-8"))
    calls: list[int] = []
    for match in CALL.finditer(source):
        calls.append(source.count("\n", 0, match.start()) + 1)
    return calls


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "root",
        nargs="?",
        type=Path,
        default=Path(__file__).resolve().parents[2],
        help="repository root",
    )
    parser.add_argument("--json", action="store_true")
    args = parser.parse_args()

    tests_dir = args.root / "tests"
    files = sorted(tests_dir.glob("static_*.cpp"))
    if not files:
        raise SystemExit("no static-backend regression sources found")

    violations = []
    for path in files:
        lines = find_calls(path)
        if lines:
            violations.append({
                "path": str(path.relative_to(args.root)),
                "lines": lines,
            })

    payload = {
        "checked_files": [str(path.relative_to(args.root)) for path in files],
        "startup_registration_calls": violations,
        "status": "pass" if not violations else "fail",
    }

    if args.json:
        print(json.dumps(payload, indent=2, sort_keys=True))
    elif violations:
        for violation in violations:
            lines = ",".join(str(line) for line in violation["lines"])
            print(f'{violation["path"]}: register_rrefl call at line(s) {lines}')
    else:
        print(f"PASS: {len(files)} static-backend regression sources contain no register_rrefl calls")

    return 0 if not violations else 1


if __name__ == "__main__":
    raise SystemExit(main())
