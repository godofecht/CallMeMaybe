#!/usr/bin/env python3

from __future__ import annotations

import importlib.util
import tempfile
from pathlib import Path


MODULE_PATH = Path(__file__).with_name("audit_static_no_registration.py")
SPEC = importlib.util.spec_from_file_location("audit_static_no_registration", MODULE_PATH)
assert SPEC and SPEC.loader
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def write(root: Path, name: str, source: str) -> Path:
    path = root / name
    path.write_text(source, encoding="utf-8")
    return path


def main() -> int:
    with tempfile.TemporaryDirectory() as tmp:
        root = Path(tmp)

        clean = write(
            root,
            "clean.cpp",
            """
            // register_rrefl(fake);
            const char* text = "register_rrefl(fake)";
            void test() { reflect_invoke(1); }
            """,
        )
        assert MODULE.find_calls(clean) == []

        direct = write(root, "direct.cpp", "void test() { register_rrefl(meta); }\n")
        assert MODULE.find_calls(direct) == [1]

        multiline = write(
            root,
            "multiline.cpp",
            """
            void test()
            {
                register_rrefl
                (meta);
            }
            """,
        )
        assert MODULE.find_calls(multiline) == [4]

        block = write(
            root,
            "block.cpp",
            """
            /* register_rrefl(meta); */
            void test() {}
            """,
        )
        assert MODULE.find_calls(block) == []

    print("PASS: static startup-registration audit parser regression")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
