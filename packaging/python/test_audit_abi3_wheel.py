#!/usr/bin/env python3
"""Unit tests for the abi3 wheel audit gate. No wheel or abi3audit required."""
from __future__ import annotations

import unittest

import audit_abi3_wheel as audit

LINUX = "neuron-9.1.0-cp312-abi3-manylinux_2_28_x86_64.whl"
MUSL = "neuron-9.1.0-cp312-abi3-musllinux_1_2_x86_64.whl"
MACOS = "neuron-9.1.0-cp312-abi3-macosx_11_0_arm64.whl"
WINDOWS = "neuron-9.1.0-cp312-abi3-win_amd64.whl"


def _ok(name: str) -> dict:
    return {
        "name": name,
        "result": {"is_abi3": True, "is_abi3_baseline_compatible": True},
    }


def _report(objects: list[dict]) -> dict:
    return {"specs": {"wheel.whl": {"kind": "wheel", "wheel": objects}}}


def _linux_objects() -> list[dict]:
    return [
        _ok("hoc.abi3.so"),
        _ok("graphicsPrimitives.abi3.so"),
        _ok("ctng.abi3.so"),
        _ok("libnrnpython.abi3.so"),
    ]


class AuditAbi3WheelTests(unittest.TestCase):
    def test_linux_ignores_failing_libpywrapper(self) -> None:
        objects = _linux_objects() + [
            {
                "name": "libpywrapper.so",
                "result": {"is_abi3": False, "is_abi3_baseline_compatible": False},
            }
        ]
        code, messages = audit.judge(_report(objects), 1, LINUX)
        self.assertEqual(code, 0)
        self.assertTrue(any("ignoring non-abi3 CLI libpywrapper.so" in line for line in messages))
        self.assertTrue(any(line.startswith("checked 4 abi3 objects:") for line in messages))

    def test_empty_specs(self) -> None:
        code, _messages = audit.judge({"specs": {}}, 0, LINUX)
        self.assertEqual(code, 1)

    def test_empty_wheel_list(self) -> None:
        code, _messages = audit.judge({"specs": {"w": {"kind": "wheel", "wheel": []}}}, 0, LINUX)
        self.assertEqual(code, 1)

    def test_error_report(self) -> None:
        code, messages = audit.judge({"error": "schema"}, 0, LINUX)
        self.assertEqual(code, 1)
        self.assertIn("abi3 audit: specs is missing or empty", messages)

    def test_report_not_an_object(self) -> None:
        code, _messages = audit.judge(["not", "a", "report"], 0, LINUX)
        self.assertEqual(code, 1)

    def test_unexplained_nonzero_status(self) -> None:
        code, messages = audit.judge(_report(_linux_objects()), 1, LINUX)
        self.assertEqual(code, 1)
        self.assertTrue(any("no ignored libpywrapper failure" in line for line in messages))

    def test_is_abi3_false(self) -> None:
        objects = _linux_objects()
        objects[0] = {
            "name": "hoc.abi3.so",
            "result": {"is_abi3": False, "is_abi3_baseline_compatible": True},
        }
        code, messages = audit.judge(_report(objects), 1, LINUX)
        self.assertEqual(code, 1)
        self.assertTrue(any("hoc.abi3.so: is_abi3 is not true" in line for line in messages))

    def test_missing_is_abi3(self) -> None:
        objects = _linux_objects()
        objects[0] = {"name": "hoc.abi3.so", "result": {"is_abi3_baseline_compatible": True}}
        code, _messages = audit.judge(_report(objects), 0, LINUX)
        self.assertEqual(code, 1)

    def test_baseline_false(self) -> None:
        objects = _linux_objects()
        objects[0] = {
            "name": "hoc.abi3.so",
            "result": {"is_abi3": True, "is_abi3_baseline_compatible": False},
        }
        code, messages = audit.judge(_report(objects), 1, LINUX)
        self.assertEqual(code, 1)
        self.assertTrue(any("is_abi3_baseline_compatible is not true" in line for line in messages))

    def test_missing_ctng(self) -> None:
        objects = [obj for obj in _linux_objects() if not obj["name"].startswith("ctng")]
        code, messages = audit.judge(_report(objects), 0, LINUX)
        self.assertEqual(code, 1)
        self.assertIn("abi3 audit: missing ctng", messages)

    def test_longer_hoc_name_does_not_count(self) -> None:
        objects = _linux_objects()
        objects[0] = _ok("hocfoo.abi3.so")
        code, messages = audit.judge(_report(objects), 0, LINUX)
        self.assertEqual(code, 1)
        self.assertIn("abi3 audit: missing hoc", messages)

    def test_macos_without_nrnpython(self) -> None:
        objects = [_ok("hoc.abi3.so"), _ok("graphicsPrimitives.abi3.so"), _ok("ctng.abi3.so")]
        code, _messages = audit.judge(_report(objects), 0, MACOS)
        self.assertEqual(code, 0)

    def test_windows_pyd_without_nrnpython(self) -> None:
        objects = [_ok("hoc.pyd"), _ok("graphicsPrimitives.pyd"), _ok("ctng.pyd")]
        code, _messages = audit.judge(_report(objects), 0, WINDOWS)
        self.assertEqual(code, 0)

    def test_linux_without_nrnpython(self) -> None:
        objects = [_ok("hoc.abi3.so"), _ok("graphicsPrimitives.abi3.so"), _ok("ctng.abi3.so")]
        code, messages = audit.judge(_report(objects), 0, LINUX)
        self.assertEqual(code, 1)
        self.assertIn("abi3 audit: missing libnrnpython", messages)

    def test_musllinux_without_nrnpython(self) -> None:
        objects = [_ok("hoc.abi3.so"), _ok("graphicsPrimitives.abi3.so"), _ok("ctng.abi3.so")]
        code, _messages = audit.judge(_report(objects), 0, MUSL)
        self.assertEqual(code, 1)

    def test_only_libpywrapper(self) -> None:
        objects = [{"name": "libpywrapper.so", "result": {"is_abi3": False}}]
        code, messages = audit.judge(_report(objects), 1, LINUX)
        self.assertEqual(code, 1)
        self.assertTrue(any(line.startswith("checked 0 abi3 objects") for line in messages))

    def test_objects_key_fallback(self) -> None:
        report = {"specs": {"wheel.whl": {"objects": _linux_objects()}}}
        code, _messages = audit.judge(report, 0, LINUX)
        self.assertEqual(code, 0)

    def test_nrnpython_stem_without_lib_prefix(self) -> None:
        objects = [_ok("hoc.abi3.so"), _ok("graphicsPrimitives.abi3.so"), _ok("ctng.abi3.so")]
        objects.append(_ok("nrnpython.abi3.so"))
        code, _messages = audit.judge(_report(objects), 0, LINUX)
        self.assertEqual(code, 0)


if __name__ == "__main__":
    unittest.main()
