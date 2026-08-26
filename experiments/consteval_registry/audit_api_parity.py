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

IGNORED = {
    "register_rrefl",
}


def public_inline_functions(path: Path) -> set[str]:
    text = path.read_text(encoding="utf-8")
    names = set(INLINE_FUNCTION.findall(text))
    return names - IGNORED


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

    runtime = public_inline_functions(args.runtime)
    static = public_inline_functions(args.static_header)

    missing = sorted(runtime - static)
    static_only = sorted(static - runtime)
    common = sorted(runtime & static)

    result = {
        "runtime_header": str(args.runtime),
        "static_header": str(args.static_header),
        "runtime_public_inline_count": len(runtime),
        "static_public_inline_count": len(static),
        "common_count": len(common),
        "missing_from_static": missing,
        "static_only": static_only,
        "parity": not missing,
    }

    if args.json:
        print(json.dumps(result, indent=2, sort_keys=True))
    else:
        print(f"runtime public inline functions: {len(runtime)}")
        print(f"static public inline functions:  {len(static)}")
        print(f"common:                          {len(common)}")
        if missing:
            print("missing from static:")
            for name in missing:
                print(f"  {name}")
        else:
            print("missing from static: none")
        if static_only:
            print("static-only:")
            for name in static_only:
                print(f"  {name}")

    return 0 if not missing else 1


if __name__ == "__main__":
    raise SystemExit(main())
