"""CXX with spaces must still name MSVC cl.exe."""

import importlib.util
from pathlib import Path

import pytest

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


def test_spaced_directory_named_cl_stays_gnu():
    """C:\\cl tools\\bin\\g++.exe is g++, not the directory component cl."""
    m = _mod()
    path = r"C:\cl tools\bin\g++.exe"
    assert m._cxx_executable(path) == path
    assert not m._is_msvc_cxx(path)
    flagged = path + " -shared"
    assert m._cxx_executable(flagged) == path
    assert not m._is_msvc_cxx(flagged)


def test_include_path_is_not_part_of_the_compiler():
    """A path after /I is an include directory, not more of cl.exe."""
    m = _mod()
    spaced = r"C:\Program Files\VS\bin\cl.exe"
    separate = spaced + r" /I C:\sdk\include"
    assert m._cxx_executable(separate) == spaced
    assert m._is_msvc_cxx(separate)
    attached = spaced + r" /IC:\sdk\include"
    assert m._cxx_executable(attached) == spaced
    assert m._is_msvc_cxx(attached)
    fe = CL + r" /FeC:\out\nrnmech.dll"
    assert m._cxx_executable(fe) == CL
    assert m._is_msvc_cxx(fe)
    assert m._cxx_executable("g++ -I/usr/include") == "g++"
    assert not m._is_msvc_cxx("g++ -I/usr/include")
    assert m._cxx_executable("/usr/bin/g++ -O2") == "/usr/bin/g++"
    assert m._cxx_executable("cl.exe @resp.rsp") == "cl.exe"
    # /bar\cl.exe is a directory token, not /I glued to a path.
    continued = r"C:\prefix /bar\cl.exe -O2"
    assert m._cxx_executable(continued) == r"C:\prefix /bar\cl.exe"


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


def test_rxd_compiler_uses_inherited_cl_before_vswhere(monkeypatch, tmp_path):
    """cl.exe plus INCLUDE and LIB is enough when vswhere cannot run."""
    m = _mod()
    cl = tmp_path / "cl.exe"
    cl.write_bytes(b"")
    env = {"PATH": str(tmp_path), "INCLUDE": r"C:\includes", "LIB": r"C:\libs"}

    def fail_vc():
        raise AssertionError("vswhere must not run when the toolchain is set")

    monkeypatch.setattr(m, "msvc_vc_env", fail_vc)
    cxx, use_msvc = m.windows_rxd_compiler(env, str(tmp_path / "nrn"))
    assert use_msvc
    assert cxx == str(cl)
    assert env["INCLUDE"] == r"C:\includes"
    assert env["LIB"] == r"C:\libs"


def test_rxd_compiler_discovers_when_include_is_unset(monkeypatch, tmp_path):
    m = _mod()
    cl = tmp_path / "cl.exe"
    cl.write_bytes(b"")
    env = {"PATH": str(tmp_path)}
    monkeypatch.setattr(m, "msvc_vc_env", lambda: None)
    assert m.windows_rxd_compiler(env, str(tmp_path / "nrn")) is None


def test_user_args_follow_the_compiler():
    """Flags after the compiler are kept. /I is not part of cl.exe."""
    m = _mod()
    assert m._cxx_user_args(r"cl.exe /I C:\sdk\include") == ["/I", r"C:\sdk\include"]
    assert m._cxx_user_args(r"cl.exe /IC:\sdk\include") == [r"/IC:\sdk\include"]
    assert m._cxx_user_args("g++ -I/usr/include -DFOO=1") == [
        "-I/usr/include",
        "-DFOO=1",
    ]
    quoted = '"%s" /nologo /I C:\\sdk\\include' % CL
    assert m._cxx_executable(quoted) == CL
    assert m._cxx_user_args(quoted) == ["/nologo", "/I", r"C:\sdk\include"]
    assert m._cxx_user_args("g++") == []
    kept = m._cxx_user_args(
        "g++ -I/usr/include -DFOO -O2 -shared -fPIC -fopenmp -openmp -std=c++17"
    )
    assert kept == [
        "-I/usr/include",
        "-DFOO",
        "-O2",
        "-shared",
        "-fPIC",
        "-fopenmp",
        "-openmp",
        "-std=c++17",
    ]
    msvc_kept = m._cxx_user_args(r"cl.exe /nologo /O2 /LD /DFOO /I C:\sdk\include")
    assert "/O2" in msvc_kept
    assert "/LD" in msvc_kept
    assert r"C:\sdk\include" in msvc_kept


def test_output_flags_are_rejected():
    """RxD names the DLL. -o and /Fe must not be forwarded in silence."""
    m = _mod()
    rejected = (
        "g++ -o /tmp/x.so",
        "g++ -o/tmp/x.so",
        "g++ --output /tmp/x.so",
        "g++ --output=/tmp/x.so",
        r"cl.exe /FeC:\out.dll",
        r"cl.exe /FoC:\obj\rxd.obj",
        "cl.exe /OUT:x.dll",
        "cl.exe /link",
        "cl.exe /LINK",
        "g++ -O2 -ofast.so",
    )
    for cxx in rejected:
        with pytest.raises(ValueError, match="output file"):
            m._cxx_user_args(cxx)
    assert "-O2" in m._cxx_user_args("g++ -O2")
    assert "/O2" in m._cxx_user_args("cl.exe /O2")


def test_rxd_command_forwards_user_flags():
    """User flags sit after the include directory and before the source."""
    m = _mod()
    cmd = m.rxd_compile_command(
        "cl.exe",
        ["/I", r"C:\sdk\include"],
        include=r"C:\py\include",
        src="rxd.cpp",
        out=r"C:\out\rxd.so",
        rxdmath=r"C:\nrn\lib\rxdmath.lib",
        msvc=True,
        win=True,
        sysroot=[],
    )
    inc = "/I" + r"C:\py\include"
    at = cmd.index(inc)
    assert cmd[at + 1 : at + 3] == ["/I", r"C:\sdk\include"]
    assert cmd[at + 3] == "rxd.cpp"
    assert cmd.index("/link") > at + 3
    assert "/Fe" + r"C:\out\rxd.so" in cmd
    libpath = next(i for i, item in enumerate(cmd) if item.startswith("/LIBPATH:"))
    assert cmd.index("/link") < libpath

    bare = m.rxd_compile_command(
        "cl.exe",
        [],
        include="inc",
        src="s.cpp",
        out="o.so",
        rxdmath="lib/rxdmath.lib",
        msvc=True,
        win=True,
        sysroot=[],
    )
    assert bare == [
        "cl.exe",
        "/nologo",
        "/LD",
        "/EHsc",
        "/O2",
        "/Iinc",
        "s.cpp",
        "/Feo.so",
        "/link",
        "/LIBPATH:lib",
        "rxdmath.lib",
        "/EXPORT:reaction",
    ]

    gnu = m.rxd_compile_command(
        "g++",
        ["-I/usr/include", "-DFOO"],
        include="/usr/include/python",
        src="rxd.cpp",
        out="/tmp/rxd.so",
        rxdmath="/usr/lib/librxdmath.so",
        msvc=False,
        win=False,
        sysroot=["-isysroot", "/sdk"],
    )
    assert gnu[:6] == [
        "g++",
        "-I/usr/include/python",
        "-I/usr/include",
        "-DFOO",
        "-isysroot",
        "/sdk",
    ]
    assert "-shared" in gnu and "-fPIC" in gnu and "-lm" in gnu
    assert gnu[gnu.index("-o") + 1] == "/tmp/rxd.so"
    assert gnu.index("rxd.cpp") < gnu.index("-o")

    win_gnu = m.rxd_compile_command(
        "g++",
        ["-DFOO"],
        include="inc",
        src="s.cpp",
        out="o.so",
        rxdmath="lib/rxdmath.a",
        msvc=False,
        win=True,
        sysroot=[],
    )
    assert "-fPIC" not in win_gnu
    assert "-lm" not in win_gnu
    assert win_gnu[2] == "-DFOO"


def test_rxd_compile_uses_forwarded_flags():
    text = (
        REPO_ROOT / "share" / "lib" / "python" / "neuron" / "rxd" / "rxd.py"
    ).read_text(encoding="utf-8")
    assert "_cxx_user_args(" in text
    assert "rxd_compile_command(" in text


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
