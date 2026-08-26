#!/usr/bin/env python3

import importlib.util
import json
import tempfile
import unittest
from pathlib import Path


MODULE_PATH = Path(__file__).with_name("build_results.py")
SPEC = importlib.util.spec_from_file_location("build_results", MODULE_PATH)
build_results = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(build_results)


class CampaignRenderingTests(unittest.TestCase):
    def campaign(self):
        digest = "a" * 64
        return {
            "requested_seed_count": 2,
            "selected_seed_count": 2,
            "seed_start": 1,
            "shard": {"index": "merged", "count": 2},
            "families": ["core", "shapes"],
            "cases_per_program": 1,
            "generated_programs": 4,
            "compilers": ["gcc=g++", "clang=clang++"],
            "compiler_fingerprints": [
                {
                    "label": "gcc",
                    "command": "g++",
                    "executable": "/usr/bin/g++",
                    "executable_sha256": "b" * 64,
                    "version": "gcc test",
                },
                {
                    "label": "clang",
                    "command": "clang++",
                    "executable": "/usr/bin/clang++",
                    "executable_sha256": "c" * 64,
                    "version": "clang test",
                },
            ],
            "counts": {
                "ok": 3,
                "disagreement": 1,
                "compile_failure": 0,
                "invariant_failure": 0,
                "runner_failure": 0,
            },
            "programs": [
                {"family": "core", "seed": 1, "status": "ok", "manifest_sha256": digest},
                {"family": "shapes", "seed": 1, "status": "ok", "manifest_sha256": digest},
                {"family": "core", "seed": 2, "status": "disagreement", "manifest_sha256": digest},
                {"family": "shapes", "seed": 2, "status": "ok", "manifest_sha256": digest},
            ],
        }

    def write_campaign(self, directory, data):
        path = Path(directory) / "campaign.json"
        path.write_text(json.dumps(data), encoding="utf-8")
        return path

    def test_merged_campaign_renders_counts_and_compiler_identity(self):
        with tempfile.TemporaryDirectory() as directory:
            path = self.write_campaign(directory, self.campaign())
            rendered = "\n".join(build_results.fuzz_section([path]))
            self.assertIn("Campaign manifests", rendered)
            self.assertIn("| 4 | core, shapes | gcc: gcc test; clang: clang test | 3 | 1 | 0 |", rendered)

    def test_incomplete_campaign_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            data = self.campaign()
            data["programs"].pop()
            data["generated_programs"] = 3
            path = self.write_campaign(directory, data)
            with self.assertRaises(SystemExit):
                build_results.fuzz_section([path])


if __name__ == "__main__":
    unittest.main()
