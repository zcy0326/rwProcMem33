from __future__ import annotations

import pathlib
import tempfile
import unittest

from scripts.select_profile import kernel_pair, parse_report, select_profile
from scripts.verify_module import relocation_count, undefined_symbols


class ProfileSelectionTests(unittest.TestCase):
    def test_supported_kernel_families(self) -> None:
        expected = {
            "5.10.218-android12": "android12-5.10",
            "5.15.149-android13": "android13-5.15",
            "6.1.99-android14": "android14-6.1",
            "6.6.30-android15": "android15-6.6",
            "6.12.8-android16": "android16-6.12",
        }
        for release, profile in expected.items():
            with self.subTest(release=release):
                self.assertEqual(select_profile(release), profile)

    def test_unsupported_family_is_rejected(self) -> None:
        with self.assertRaisesRegex(ValueError, "unsupported kernel family"):
            select_profile("6.8.0-generic")

    def test_invalid_release_is_rejected(self) -> None:
        with self.assertRaisesRegex(ValueError, "cannot parse kernel release"):
            kernel_pair("not-a-kernel")

    def test_probe_report_parser(self) -> None:
        content = """\
[identity]
uname_r=6.6.30-android15
uname_m=aarch64

[memory]
page_size=16384
"""
        with tempfile.TemporaryDirectory() as directory:
            report = pathlib.Path(directory) / "probe.txt"
            report.write_text(content, encoding="utf-8")
            values = parse_report(report)
        self.assertEqual(values["uname_r"], "6.6.30-android15")
        self.assertEqual(values["page_size"], "16384")


class ModuleParserTests(unittest.TestCase):
    def test_undefined_symbol_parser(self) -> None:
        table = """
  1: 0000000000000000     0 NOTYPE  GLOBAL DEFAULT  UND copy_to_user
  2: 0000000000000000     0 NOTYPE  GLOBAL DEFAULT    1 local_symbol
  3: 0000000000000000     0 NOTYPE  GLOBAL DEFAULT  UND symbol@VER_1
"""
        self.assertEqual(undefined_symbols(table), {"copy_to_user", "symbol"})

    def test_relocation_counter(self) -> None:
        relocations = """
Relocation section '.rela.text' contains 2 entries:
000000000010  000100000113 R_AARCH64_ADR_PREL_PG_HI21
000000000014  000200000115 R_AARCH64_ADD_ABS_LO12_NC
"""
        self.assertEqual(relocation_count(relocations), 2)


if __name__ == "__main__":
    unittest.main()
