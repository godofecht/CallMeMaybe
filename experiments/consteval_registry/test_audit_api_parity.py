#!/usr/bin/env python3

from __future__ import annotations

import tempfile
import unittest
from pathlib import Path

from audit_api_parity import compare_surfaces, public_inline_functions


class ApiParityAuditTest(unittest.TestCase):
    def write_header(self, directory: Path, name: str, text: str) -> Path:
        path = directory / name
        path.write_text(text, encoding="utf-8")
        return path

    def test_macro_generated_predicates_are_public_api(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            header = self.write_header(
                directory,
                "meta.hpp",
                """
inline bool reflect_name(int) { return true; }
#define CMM_DEFINE_TYPE_PREDICATE(name, flag) inline bool name(int) { return flag; }
CMM_DEFINE_TYPE_PREDICATE(is_pointer_type, is_pointer)
CMM_DEFINE_TYPE_PREDICATE(is_const_type, is_const)
""",
            )

            self.assertEqual(
                public_inline_functions(header),
                {"reflect_name", "is_pointer_type", "is_const_type"},
            )

    def test_missing_macro_generated_predicate_breaks_parity(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            runtime = self.write_header(
                directory,
                "runtime.hpp",
                """
inline bool reflect_name(int) { return true; }
CMM_DEFINE_TYPE_PREDICATE(is_pointer_type, is_pointer)
CMM_DEFINE_TYPE_PREDICATE(is_const_type, is_const)
""",
            )
            static = self.write_header(
                directory,
                "static.hpp",
                """
inline bool reflect_name(int) { return true; }
CMM_DEFINE_STATIC_TYPE_PREDICATE(is_pointer_type, is_pointer)
""",
            )

            result = compare_surfaces(runtime, static)
            self.assertFalse(result["parity"])
            self.assertEqual(result["missing_from_static"], ["is_const_type"])

    def test_register_rrefl_remains_deliberately_excluded(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            runtime = self.write_header(
                directory,
                "runtime.hpp",
                "inline int register_rrefl() { return 0; }\ninline int reflect_name() { return 0; }\n",
            )
            static = self.write_header(
                directory,
                "static.hpp",
                "inline int reflect_name() { return 0; }\n",
            )

            result = compare_surfaces(runtime, static)
            self.assertTrue(result["parity"])
            self.assertEqual(result["missing_from_static"], [])


if __name__ == "__main__":
    unittest.main()
