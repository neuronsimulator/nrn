"""CXX with spaces must still name MSVC cl.exe."""

import importlib.util
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
MOD = REPO_ROOT / "share" / "lib" / "python" / "neuron" / "_windows_cxx.py"

CL = (
    r"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools"
    r"\MSVC\14.44.35207\bin\Hostx64\x64\cl.exe"
)
CLANG = r"C:\Program Files\LLVM\bin\clang-cl.exe"
CL_FWD = (
    "C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools"
    "/MSVC/14.44.35207/bin/Hostx64/x64/cl.exe"
)


def _mod():
    spec = importlib.util.spec_from_file_location("nrn_windows_cxx", MOD)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod


def test_quoted_cl_path_with_spaces():
    m = _mod()
    cxx = f'"{CL}"'
    assert m._cxx_executable(cxx) == CL
    assert m._is_msvc_cxx(cxx)


def test_unquoted_cl_path_with_spaces():
    m = _mod()
    assert m._cxx_executable(CL) == CL
    assert m._is_msvc_cxx(CL)


def test_quoted_cl_path_drops_trailing_flags():
    m = _mod()
    cxx = f'"{CL}" /nologo'
    assert m._cxx_executable(cxx) == CL
    assert m._is_msvc_cxx(cxx)


def test_unquoted_cl_path_with_trailing_flag():
    m = _mod()
    cxx = CL + " /nologo"
    assert m._cxx_executable(cxx) == CL
    assert m._is_msvc_cxx(cxx)


def test_forward_slash_program_files_path():
    m = _mod()
    assert m._cxx_executable(CL_FWD) == CL_FWD
    assert m._is_msvc_cxx(CL_FWD)


def test_clang_cl_path_with_spaces_is_not_cl():
    m = _mod()
    assert m._cxx_executable(CLANG) == CLANG
    assert m._is_msvc_cxx(CLANG)
    assert m._cxx_executable(CLANG + " -O2") == CLANG


def test_bare_names_and_non_msvc():
    m = _mod()
    assert m._is_msvc_cxx("cl")
    assert m._is_msvc_cxx("cl.exe")
    assert m._is_msvc_cxx("clang-cl")
    assert m._is_msvc_cxx("clang-cl.exe")
    assert m._is_msvc_cxx("CL.EXE /nologo")
    assert not m._is_msvc_cxx("g++")
    assert not m._is_msvc_cxx("g++ -shared")
    assert m._cxx_executable("g++ -shared") == "g++"
    assert not m._is_msvc_cxx(r"C:\Program Files\mingw64\bin\g++.exe")
    # A directory named cl is not the compiler.
    nested = r"C:\cl\bin\clang-cl.exe"
    assert m._cxx_executable(nested) == nested
    assert m._is_msvc_cxx(nested)


def test_msvc_cl_available_accepts_spaced_path(monkeypatch, tmp_path):
    m = _mod()
    cl = tmp_path / "Program Files" / "cl.exe"
    cl.parent.mkdir()
    cl.write_bytes(b"")
    monkeypatch.setenv("CXX", f'"{cl}"')

    def fail_which(name):
        raise AssertionError("which should not run when CXX names a file")

    def fail_vc():
        raise AssertionError("vcvars should not run when CXX names a file")

    monkeypatch.setattr(m, "which", fail_which)
    monkeypatch.setattr(m, "msvc_vc_env", fail_vc)
    assert m.msvc_cl_available()
