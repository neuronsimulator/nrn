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


def _touch(path):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text("TITLE fake\n", encoding="utf-8")


def test_collect_mod_files_expands_each_directory(tmp_path, monkeypatch):
    """nrnivmodl channels synapses keeps MOD files from both directories."""
    monkeypatch.chdir(tmp_path)
    _touch(tmp_path / "channels" / "na.mod")
    _touch(tmp_path / "channels" / "k.mod")
    _touch(tmp_path / "synapses" / "ampa.mod")
    bw = _binwrapper()
    got = [p.name for p in bw._collect_mod_files(["channels", "synapses"])]
    assert got == ["k.mod", "na.mod", "ampa.mod"]


def test_collect_mod_files_mixes_directory_and_file(tmp_path, monkeypatch):
    """nrnivmodl channels extra.mod keeps the directory and the extra file."""
    monkeypatch.chdir(tmp_path)
    _touch(tmp_path / "channels" / "na.mod")
    _touch(tmp_path / "extra.mod")
    bw = _binwrapper()
    got = [p.name for p in bw._collect_mod_files(["channels", "extra.mod"])]
    assert got == ["na.mod", "extra.mod"]


def test_collect_mod_files_missing_name_still_errors(tmp_path, monkeypatch):
    monkeypatch.chdir(tmp_path)
    (tmp_path / "channels").mkdir()
    bw = _binwrapper()
    try:
        bw._collect_mod_files(["channels", "missing.mod"])
    except SystemExit as exc:
        assert "missing.mod" in str(exc)
    else:
        raise AssertionError("a missing MOD file must be rejected")


def _gui_tree(tmp_path):
    """Minimal NEURONHOME and nrniv.exe so the GUI wrappers reach subprocess.call."""
    home = tmp_path / "share" / "nrn"
    hoc_dir = home / "lib" / "hoc"
    hoc_dir.mkdir(parents=True)
    (hoc_dir / "nrngui.hoc").write_text("\n", encoding="utf-8")
    demo = home / "demo"
    release = demo / "release"
    release.mkdir(parents=True)
    (demo / "demo.hoc").write_text("\n", encoding="utf-8")
    (release / "nrnmech.dll").write_bytes(b"")
    bindir = tmp_path / "bin"
    bindir.mkdir()
    (bindir / "nrniv.exe").write_bytes(b"")
    return home


def test_gui_wrappers_keep_redirected_stdin(monkeypatch, tmp_path):
    """A non-tty stdin is the trailing '-' input, not something to discard."""
    bw = _binwrapper()
    home = _gui_tree(tmp_path)
    monkeypatch.setenv("NEURONHOME", str(home))
    monkeypatch.setenv("NRNHOME", str(tmp_path))
    seen = []

    def fake_call(cmd, **kwargs):
        seen.append((cmd, kwargs))
        return 0

    monkeypatch.setattr(bw.subprocess, "call", fake_call)
    monkeypatch.setattr(bw.sys.stdin, "isatty", lambda: False)
    assert bw._nrngui(["-c", "quit()"]) == 0
    assert bw._neurondemo(["-nogui", "-c", "quit()"]) == 0
    assert len(seen) == 2
    for cmd, kwargs in seen:
        assert kwargs == {}
        assert cmd[-1] == "-"
        assert "nrniv.exe" in cmd[0]


def test_windows_launch_uses_python_for_a_shebang(tmp_path):
    """rdcellstate runs under sys.executable. A bash script is not one."""
    bw = _binwrapper()
    script = tmp_path / "rdcellstate"
    script.write_text("#!/usr/bin/env python3\n", encoding="utf-8")
    bash = tmp_path / "sortspike"
    bash.write_text("#!/usr/bin/env bash\n", encoding="utf-8")
    exe = tmp_path / "nrniv.exe"
    exe.write_bytes(b"MZ")
    assert bw._windows_launch_argv(str(script), ["a.dat"]) == [
        bw.sys.executable,
        str(script),
        "a.dat",
    ]
    assert bw._windows_launch_argv(str(bash), ["in", "out"]) == [
        str(bash),
        "in",
        "out",
    ]
    assert bw._windows_launch_argv(str(exe), ["-c", "quit()"]) == [
        str(exe),
        "-c",
        "quit()",
    ]


def test_windows_wheel_omits_bash_launchers():
    text = (
        REPO_ROOT / "share" / "lib" / "python" / "scripts" / "CMakeLists.txt"
    ).read_text(encoding="utf-8")
    assert "list(REMOVE_ITEM scripts mkthreadsafe sortspike)" in text


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
