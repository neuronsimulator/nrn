"""nrniv wrapper PATH for delvewheel-vendored MSVC CRT."""

import importlib.util
import os
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
BINWRAPPER = REPO_ROOT / "share" / "lib" / "python" / "scripts" / "binwrapper.py"


def _binwrapper():
    spec = importlib.util.spec_from_file_location("nrn_binwrapper", BINWRAPPER)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def test_cmd_launcher_finds_prefix_python_before_path():
    """Hosted CPython keeps python.exe above Scripts; a venv keeps it beside the .cmd."""
    cmd = (
        REPO_ROOT / "share" / "lib" / "python" / "scripts" / "binwrapper.cmd"
    ).read_text(encoding="utf-8")
    beside = cmd.index('set "PY=%~dp0python.exe"')
    prefix = cmd.index('set "PY=%~dp0..\\python.exe"')
    fallback = cmd.index('set "PY=python"')
    assert beside < prefix < fallback


def test_vendored_wheel_lib_dirs(tmp_path):
    bw = _binwrapper()
    assert bw._vendored_wheel_lib_dirs(tmp_path) == []
    nightly = tmp_path / "neuron_nightly.libs"
    regular = tmp_path / "neuron.libs"
    nightly.mkdir()
    regular.mkdir()
    got = bw._vendored_wheel_lib_dirs(tmp_path)
    assert got == [str(nightly), str(regular)]
    assert bw._vendored_wheel_lib_dirs(regular) == []
