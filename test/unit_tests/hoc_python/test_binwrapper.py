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


def test_resolve_pylibs_keeps_existing_nrn_pylib(monkeypatch):
    """NRN_PYLIB is the documented override and must survive the wrapper."""
    bw = _binwrapper()

    def fail_find():
        raise AssertionError("find_libpython should not run when both are set")

    monkeypatch.setattr(bw, "find_libpython", fail_find)
    monkeypatch.setenv("NRN_PYLIB", "/explicit/python.dll")
    monkeypatch.setenv("NMODL_PYLIB", "/other/nmodl.dll")
    bw._resolve_pylibs()
    assert os.environ["NRN_PYLIB"] == "/explicit/python.dll"
    assert os.environ["NMODL_PYLIB"] == "/other/nmodl.dll"


def test_resolve_pylibs_fills_nrn_from_nmodl(monkeypatch):
    bw = _binwrapper()
    monkeypatch.setattr(bw, "find_libpython", lambda: "/discovered.so")
    monkeypatch.delenv("NRN_PYLIB", raising=False)
    monkeypatch.setenv("NMODL_PYLIB", "/other/nmodl.dll")
    bw._resolve_pylibs()
    assert os.environ["NRN_PYLIB"] == "/other/nmodl.dll"
    assert os.environ["NMODL_PYLIB"] == "/other/nmodl.dll"


def test_resolve_pylibs_uses_find_when_unset(monkeypatch):
    bw = _binwrapper()
    monkeypatch.setattr(bw, "find_libpython", lambda: "/discovered.so")
    monkeypatch.delenv("NRN_PYLIB", raising=False)
    monkeypatch.delenv("NMODL_PYLIB", raising=False)
    bw._resolve_pylibs()
    assert os.environ["NRN_PYLIB"] == "/discovered.so"
    assert os.environ["NMODL_PYLIB"] == "/discovered.so"


def test_resolve_pylibs_nrn_alone_does_not_search(monkeypatch):
    bw = _binwrapper()

    def fail_find():
        raise AssertionError("find_libpython should not run when NRN_PYLIB is set")

    monkeypatch.setattr(bw, "find_libpython", fail_find)
    monkeypatch.setenv("NRN_PYLIB", "/explicit/python.dll")
    monkeypatch.delenv("NMODL_PYLIB", raising=False)
    bw._resolve_pylibs()
    assert os.environ["NRN_PYLIB"] == "/explicit/python.dll"
    assert os.environ["NMODL_PYLIB"] == "/explicit/python.dll"


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
