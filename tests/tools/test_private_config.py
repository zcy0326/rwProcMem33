from __future__ import annotations

import json
import pathlib
import tempfile
import unittest

from scripts.gen_private_config import (
    build_config,
    render_header,
    validate_instance_id,
    validate_name,
)


class PrivateConfigTests(unittest.TestCase):
    def test_valid_configuration(self) -> None:
        name = validate_name("kfi_0123456789abcdef")
        instance = validate_instance_id("build-01234567")
        config = build_config(name, instance)
        self.assertEqual(
            config["endpoint"],
            "proc:/proc/kfi_0123456789abcdef/kfi_0123456789abcdef",
        )
        header = render_header(name, instance)
        self.assertIn('#define KFI_PRIVATE_PROC_NAME "' + name + '"', header)
        self.assertIn(instance, header)

    def test_invalid_names_are_rejected(self) -> None:
        for value in ("short", "UppercaseName", "1starts_with_digit", "bad/name"):
            with self.subTest(value=value), self.assertRaises(ValueError):
                validate_name(value)

    def test_invalid_instance_is_rejected(self) -> None:
        with self.assertRaises(ValueError):
            validate_instance_id("bad instance")

    def test_json_is_serializable(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / "endpoint.json"
            config = build_config("kfi_01234567", "instance-1234")
            path.write_text(json.dumps(config), encoding="utf-8")
            self.assertEqual(
                json.loads(path.read_text(encoding="utf-8"))["proc_name"],
                "kfi_01234567",
            )


if __name__ == "__main__":
    unittest.main()
