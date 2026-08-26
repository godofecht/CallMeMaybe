#!/usr/bin/env python3

import hashlib
import importlib.util
import sys
import unittest
from pathlib import Path


MODULE_PATH = Path(__file__).with_name("collect_machine.py")
SPEC = importlib.util.spec_from_file_location("collect_machine", MODULE_PATH)
collect_machine = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(collect_machine)


class CompilerFingerprintTests(unittest.TestCase):
    def test_resolved_compiler_is_content_addressed(self):
        executable = collect_machine.resolve_compiler([sys.executable])
        expected = hashlib.sha256(executable.read_bytes()).hexdigest()

        self.assertTrue(executable.is_absolute())
        self.assertEqual(collect_machine.sha256_file(executable), expected)
        self.assertEqual(len(expected), 64)

    def test_missing_compiler_is_rejected(self):
        with self.assertRaises(SystemExit):
            collect_machine.resolve_compiler(["cmm-compiler-that-does-not-exist"])


if __name__ == "__main__":
    unittest.main()
