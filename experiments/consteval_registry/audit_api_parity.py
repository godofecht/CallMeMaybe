#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import re
from pathlib import Path


INLINE_FUNCTION = re.compile(
    r"^inline\s+(?:[\w:<>]+(?:\s*[*&])?|auto)\s+([A-Za-z_]\w*)\s*\(",
    re.MULTILINE,
)

TYPE_PREDICATE_INVOCATION = re.compile(
    r"^CMM_DEFINE_(?:STATIC_)?TYPE_PREDICATE\(\s*([A-Za-z_]\w*)\s*,",
    re.MULTILINE,
)

IGNORED = {
    "register_rrefl",
}


def public_inline_functions(path: Path) -> set[str]:
    text = path.read_text(encoding="utf-8")
    names = set(INLINE_FUNCTION.findall(text))
    names.update(TYPE_PREDICATE_INVOCATION.findall(text))
    return names - IGNORED


def compare_surfaces(runtime_path: Path, static_path: Path) -> dict:
    runtime = public_inline_functions(runtime_path)
    static = public_inline_functions(static_path)

    missing = sorted(runtime - static)
    static_only = sorted(static - runtime)
    common = sorted(runtime & static)

    return {
        "runtime_header": str(runtime_path),
        "static_header": str(static_path),
        "runtime_public_inline_count": len(runtime),
        "static_public_inline_count": len(static),
        "common_count": len(common),
        "missing_from_static": missing,
        "static_only": static_only,
        "parity": not missing,
    }


def main() -> int:
    parser = argparse.ArgumentParser(
        description=(
            "Compare the public inline query surface of the stabilized runtime "
            "metadata API with the consteval/static metadata API."
        )
    )
    parser.add_argument(
        "--runtime",
        type=Path,
        default=Path("include/cmm/meta.hpp"),
        help="stabilized runtime metadata header",
    )
    parser.add_argument(
        "--static",
        dest="static_header",
        type=Path,
        default=Path("include/cmm/static_meta.hpp"),
        help="consteval/static metadata header",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="emit machine-readable JSON",
    )
    args = parser.parse_args()

    result = compare_surfaces(args.runtime, args.static_header)

    if args.json:
        print(json.dumps(result, indent=2, sort_keys=True))
    else:
        print(f"runtime public inline functions: {result['runtime_public_inline_count']}")
        print(f"static public inline functions:  {result['static_public_inline_count']}")
        print(f"common:                          {result['common_count']}")
        if result["missing_from_static"]:
            print("missing from static:")
            for name in result["missing_from_static"]:
                print(f"  {name}")
        else:
            print("missing from static: none")
        if result["static_only"]:
            print("static-only:")
            for name in result["static_only"]:
                print(f"  {name}")

    return 0 if result["parity"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
