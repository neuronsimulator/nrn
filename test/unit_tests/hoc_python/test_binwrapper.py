"""nrniv wrapper PATH for delvewheel-vendored MSVC CRT."""

import importlib.util
import os
from pathlib import Path

import pytest

REPO_ROOT = Path(__file__).resolve().parents[3]
BINWRAPPER = REPO_ROOT / "share" / "lib" / "python" / "scripts" / "binwrapper.py"


def _binwrapper():
    spec = importlib.util.spec_from_file_location("nrn_binwrapper", BINWRAPPER)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def test_cmd_launcher_keeps_percent_star_and_doubled_quotes():
    """%* stays. A quoted -c is documented as doubled quotes, not a new launcher."""
    cmd = (
        REPO_ROOT / "share" / "lib" / "python" / "scripts" / "binwrapper.cmd"
    ).read_text(encoding="utf-8")
    assert '"%PY%" "%~dp0%~n0" %*' in cmd
    assert 'print ""hello""' in cmd
    doc = (REPO_ROOT / "docs" / "install" / "install_instructions.md").read_text(
        encoding="utf-8"
    )
    assert 'print ""hello""' in doc
    assert "cmd /s /c" in doc


def test_cmd_launcher_finds_prefix_python_before_path():
    """A venv python.exe is beside the .cmd. Otherwise use the pip shebang.

    pip install --user does not put Scripts next to python.exe. The script
    shebang is #!python so pip rewrites it to that interpreter. PATH is not
    a fallback.
    """
    cmd = (
        REPO_ROOT / "share" / "lib" / "python" / "scripts" / "binwrapper.cmd"
    ).read_text(encoding="utf-8")
    beside = cmd.index('set "PY=%~dp0python.exe"')
    prefix = cmd.index('set "PY=%~dp0..\\python.exe"')
    shebang = cmd.index('set /p SHEBANG=<"%~dp0%~n0"')
    assert beside < prefix < shebang
    assert 'set "PY=%SHEBANG:~2%"' in cmd
    assert 'set "PY=python"' not in cmd
    assert "exit /b %ERRORLEVEL%" in cmd
    first = BINWRAPPER.read_text(encoding="utf-8").splitlines()[0]
    assert first == "#!python"


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


def test_collect_mod_files_accepts_mod_prefix(tmp_path, monkeypatch):
    """nrnivmodl hh compiles hh.mod. An existing file wins over the prefix."""
    monkeypatch.chdir(tmp_path)
    _touch(tmp_path / "hh.mod")
    _touch(tmp_path / "kept")
    _touch(tmp_path / "kept.mod")
    _touch(tmp_path / "foo.bar.mod")
    bw = _binwrapper()
    got = [p.name for p in bw._collect_mod_files(["hh", "kept", "foo.bar"])]
    assert got == ["hh.mod", "kept", "foo.bar.mod"]


def test_nrnivmodl_accepts_incflags_that_look_like_options():
    """-incflags takes the next argument, including one that starts with -I."""
    bw = _binwrapper()
    parsed = bw._nrnivmodl_parse_args(
        [
            "-loadflags",
            "first.lib",
            "-incflags",
            "-I/usr/include",
            "-loadflags",
            "/LIBPATH:C:/sdk",
            "hh.mod",
        ]
    )
    assert parsed["help"] is False
    assert parsed["incflags"] == "-I/usr/include"
    assert parsed["loadflags"] == "/LIBPATH:C:/sdk"
    assert parsed["rest"] == ["hh.mod"]


def test_nrnivmodl_flags_stop_at_the_first_mod_file():
    bw = _binwrapper()
    parsed = bw._nrnivmodl_parse_args(["hh.mod", "-incflags", "-I/usr/include"])
    assert parsed["incflags"] == ""
    assert parsed["rest"] == ["hh.mod", "-incflags", "-I/usr/include"]


def test_nrnivmodl_flag_without_a_value_is_rejected():
    bw = _binwrapper()
    try:
        bw._nrnivmodl_parse_args(["-incflags"])
    except SystemExit as exc:
        assert "-incflags" in str(exc)
    else:
        raise AssertionError("-incflags without a value must be rejected")


def test_nrnivmodl_rejects_unix_only_codegen_options():
    bw = _binwrapper()
    for opt in ("-coreneuron", "-nmodl", "-nmodlflags", "-unknown"):
        try:
            bw._nrnivmodl_parse_args([opt, "hh.mod"])
        except SystemExit as exc:
            assert opt in str(exc)
        else:
            raise AssertionError(f"{opt} must be rejected")


def test_nrnivmodl_help_names_the_windows_flag_strings(capsys):
    bw = _binwrapper()
    parsed = bw._nrnivmodl_parse_args(["-h", "hh.mod"])
    assert parsed["help"] is True
    bw._nrnivmodl_help()
    out = capsys.readouterr().out
    assert "-incflags" in out
    assert "-loadflags" in out
    assert "/LIBPATH:" in out
    assert "cl.exe" in out
    assert "-coreneuron" in out
    assert "-nmodl" in out


def test_nrnivmodl_flag_defs_keep_spaces_and_semicolons():
    bw = _binwrapper()
    defs = bw._nrnivmodl_flag_defs("/I C:/sdk/include;extra", "foo.lib")
    assert defs == [
        "-DNRNIVMODL_INCFLAGS=/I C:/sdk/include;extra",
        "-DNRNIVMODL_LDFLAGS=foo.lib",
    ]
    assert bw._nrnivmodl_flag_defs("", "") == []


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


def _refuse_build_dir(bw, path):
    try:
        bw._nt_prepare_build_dir(path)
    except SystemExit as exc:
        text = str(exc)
        assert "will not delete" in text
        assert "NEURON nrnivmodl build directory" in text
        return text
    raise AssertionError("an unmarked directory must be refused")


def _forbid_rmtree(*_args, **_kwargs):
    raise AssertionError("rmtree must not run on an unrelated directory")


def test_nrnivmodl_refuses_unrelated_amd64_directory(tmp_path, monkeypatch):
    """An AMD64 folder with no NEURON marker is left in place. rmtree is not called."""
    amd = tmp_path / "AMD64"
    amd.mkdir()
    keep = amd / "keep.txt"
    keep.write_text("unrelated\n", encoding="utf-8")
    bw = _binwrapper()

    monkeypatch.setattr(bw.shutil, "rmtree", _forbid_rmtree)
    _refuse_build_dir(bw, amd)
    assert keep.read_text(encoding="utf-8") == "unrelated\n"
    assert amd.is_dir()
    assert list(tmp_path.glob("AMD64.old.*")) == []


def test_nrnivmodl_refuses_other_cmake_project(tmp_path, monkeypatch):
    amd = tmp_path / "AMD64"
    amd.mkdir()
    cache = amd / "CMakeCache.txt"
    cache.write_text("CMAKE_PROJECT_NAME:STATIC=notneuron\n", encoding="utf-8")
    bw = _binwrapper()
    monkeypatch.setattr(bw.shutil, "rmtree", _forbid_rmtree)
    _refuse_build_dir(bw, amd)
    assert cache.read_text(encoding="utf-8").startswith(
        "CMAKE_PROJECT_NAME:STATIC=notneuron"
    )


def test_nrnivmodl_comment_is_not_a_marker(tmp_path, monkeypatch):
    amd = tmp_path / "AMD64"
    amd.mkdir()
    cache = amd / "CMakeCache.txt"
    cache.write_text("# NRNIVMODL_NEURON:BOOL=ON\n", encoding="utf-8")
    bw = _binwrapper()
    monkeypatch.setattr(bw.shutil, "rmtree", _forbid_rmtree)
    _refuse_build_dir(bw, amd)
    assert cache.is_file()


def test_nrnivmodl_refuses_a_file_named_amd64(tmp_path, monkeypatch):
    amd = tmp_path / "AMD64"
    amd.write_text("precious\n", encoding="utf-8")
    bw = _binwrapper()
    monkeypatch.setattr(bw.shutil, "rmtree", _forbid_rmtree)
    _refuse_build_dir(bw, amd)
    assert amd.read_text(encoding="utf-8") == "precious\n"


def test_nrnivmodl_refuses_a_symlink(tmp_path, monkeypatch):
    target = tmp_path / "real"
    target.mkdir()
    (target / "CMakeCache.txt").write_text(
        "CMAKE_PROJECT_NAME:STATIC=nrnivmodl\nNRNIVMODL_NEURON:BOOL=ON\n",
        encoding="utf-8",
    )
    link = tmp_path / "AMD64"
    try:
        link.symlink_to(target, target_is_directory=True)
    except OSError:
        pytest.skip("this platform cannot create a directory symlink")
    bw = _binwrapper()
    monkeypatch.setattr(bw.shutil, "rmtree", _forbid_rmtree)
    _refuse_build_dir(bw, link)
    assert link.is_symlink()
    assert (target / "CMakeCache.txt").is_file()


def test_nrnivmodl_missing_build_directory_is_left_missing(tmp_path):
    amd = tmp_path / "AMD64"
    bw = _binwrapper()
    bw._nt_prepare_build_dir(amd)
    assert not amd.exists()


def test_nrnivmodl_replaces_its_own_cache(tmp_path):
    """A previous nrnivmodl tree is removed so stale CMake state does not linger."""
    amd = tmp_path / "AMD64"
    amd.mkdir()
    (amd / "CMakeCache.txt").write_text(
        "# comment NRNIVMODL_ should not count\n"
        "CMAKE_PROJECT_NAME:STATIC=nrnivmodl\n"
        "NRNIVMODL_NEURON:BOOL=ON\n",
        encoding="utf-8",
    )
    (amd / "stale.txt").write_text("old\n", encoding="utf-8")
    bw = _binwrapper()
    bw._nt_prepare_build_dir(amd)
    assert not amd.exists()
    assert list(tmp_path.glob("AMD64.old.*")) == []


@pytest.mark.parametrize(
    "dll_parts",
    [
        ("nrnmech.dll",),
        ("Release", "nrnmech.dll"),
        ("RelWithDebInfo", "nrnmech.dll"),
        ("Debug", "nrnmech.dll"),
        ("MinSizeRel", "nrnmech.dll"),
    ],
)
def test_nrnivmodl_refuses_nrnmech_dll_without_cache(tmp_path, monkeypatch, dll_parts):
    """A nrnmech.dll filename is not ownership. The directory stays."""
    amd = tmp_path / "AMD64"
    dll = amd.joinpath(*dll_parts)
    dll.parent.mkdir(parents=True, exist_ok=True)
    dll.write_bytes(b"MZ")
    bw = _binwrapper()
    monkeypatch.setattr(bw.shutil, "rmtree", _forbid_rmtree)
    _refuse_build_dir(bw, amd)
    assert dll.read_bytes() == b"MZ"
    assert amd.is_dir()


def test_nrnivmodl_refuses_solution_without_cache(tmp_path, monkeypatch):
    """nrnivmodl.sln is not ownership. The directory stays."""
    amd = tmp_path / "AMD64"
    amd.mkdir()
    sln = amd / "nrnivmodl.sln"
    sln.write_text("Microsoft Visual Studio Solution File\n", encoding="utf-8")
    bw = _binwrapper()
    monkeypatch.setattr(bw.shutil, "rmtree", _forbid_rmtree)
    _refuse_build_dir(bw, amd)
    assert sln.is_file()
    assert amd.is_dir()


def test_nrnivmodl_refuses_other_project_with_nrnmech_dll(tmp_path, monkeypatch):
    """A foreign CMake cache stays foreign when a nrnmech.dll is also present."""
    amd = tmp_path / "AMD64"
    release = amd / "Release"
    release.mkdir(parents=True)
    (amd / "CMakeCache.txt").write_text(
        "CMAKE_PROJECT_NAME:STATIC=notneuron\n", encoding="utf-8"
    )
    dll = release / "nrnmech.dll"
    dll.write_bytes(b"MZ")
    bw = _binwrapper()
    monkeypatch.setattr(bw.shutil, "rmtree", _forbid_rmtree)
    _refuse_build_dir(bw, amd)
    assert dll.read_bytes() == b"MZ"


def test_nrnivmodl_refuses_other_project_with_solution(tmp_path, monkeypatch):
    """A foreign CMake cache stays foreign when nrnivmodl.sln is also present."""
    amd = tmp_path / "AMD64"
    amd.mkdir()
    (amd / "CMakeCache.txt").write_text(
        "CMAKE_PROJECT_NAME:STATIC=notneuron\n", encoding="utf-8"
    )
    sln = amd / "nrnivmodl.sln"
    sln.write_text("Microsoft Visual Studio Solution File\n", encoding="utf-8")
    bw = _binwrapper()
    monkeypatch.setattr(bw.shutil, "rmtree", _forbid_rmtree)
    _refuse_build_dir(bw, amd)
    assert sln.is_file()


def test_windows_wheel_docs_name_the_ansi_code_page_limit():
    wheels = (REPO_ROOT / "docs" / "install" / "python_wheels.md").read_text(
        encoding="utf-8"
    )
    install = (REPO_ROOT / "docs" / "install" / "install_instructions.md").read_text(
        encoding="utf-8"
    )
    for doc in (wheels, install):
        assert "active ANSI code page" in doc
        assert "best-fit" in doc
        assert "import neuron" in doc


def test_windows_wheel_docs_name_nocmodl_without_nmodl_flags():
    wheels = (REPO_ROOT / "docs" / "install" / "python_wheels.md").read_text(
        encoding="utf-8"
    )
    install = (REPO_ROOT / "docs" / "install" / "install_instructions.md").read_text(
        encoding="utf-8"
    )
    for doc in (wheels, install):
        assert "nocmodl" in doc
        assert "-nmodl" in doc
        assert "-nmodlflags" in doc
