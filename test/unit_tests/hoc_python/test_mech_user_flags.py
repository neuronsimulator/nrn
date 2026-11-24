"""-incflags and -loadflags must survive CMake and reach the generated command.

The Python -D string is not enough. cmake_parse_arguments splits on ';', and
SHELL: treats a Windows backslash as a Unix escape.
"""

import os
import re
import shutil
import subprocess
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
MECH = REPO_ROOT / "cmake" / "neuronMechMaker.cmake"
MOD_REG = REPO_ROOT / "cmake" / "mod_reg_nrn.cpp.in"


def _probe_compiler():
    """A compiler path that exists. Configure must not run it.

    Forward slashes: CMake treats ``\\m`` in a quoted path as an escape.
    """
    if os.path.isfile("/bin/echo"):
        return "/bin/echo"
    for name in ("cc", "gcc", "cl"):
        found = shutil.which(name)
        if found:
            return Path(found).as_posix()
    raise RuntimeError("no compiler path for the mech-flag probe")


def _without_sanitizer_preload(env):
    """Copy env without a sanitizer preload.

    python_unit_tests preloads libtsan. CMake is not instrumented, and its
    configure then waits until ctest hits the 1000s timeout. Import children
    of this suite keep the preload.
    """
    child = env.copy()
    child.pop("LD_PRELOAD", None)
    child.pop("DYLD_INSERT_LIBRARIES", None)
    name = child.get("NRN_SANITIZER_PRELOAD_VAR") or ""
    if name:
        child.pop(name, None)
    return child


def _cmake_env():
    """Configure with a forced compiler and no sanitizer preload."""
    env = _without_sanitizer_preload(os.environ)
    for key in ("CC", "CXX", "CFLAGS", "CXXFLAGS", "LDFLAGS"):
        env.pop(key, None)
    return env


CMAKE = r"""
cmake_minimum_required(VERSION 3.20)
# Before project(). A later set does not stop CMake 3.20 from compiling an
# ABI probe with whatever CC the parent job exported.
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_C_COMPILER "@CC@")
set(CMAKE_C_COMPILER_ID "GNU")
set(CMAKE_C_COMPILE_OPTIONS_PIC "")
project(flagprobe C)
include("@MECH@")

function(roundtrip out_inc out_ld)
  _nrn_mech_arg_specs()
  cmake_parse_arguments(PARSE_ARGV 2 NRN_MECH "${options}" "${oneValueArgs}" "${multiValueArgs}")
  set(${out_inc} "${NRN_MECH_INCFLAGS}" PARENT_SCOPE)
  set(${out_ld} "${NRN_MECH_LDFLAGS}" PARENT_SCOPE)
endfunction()

roundtrip(got_inc got_ld NEURON MOD_FILES a.mod b.mod LIBRARY_OUTPUT_DIR "/tmp/out"
          INCFLAGS "/I C:/sdk/include;extra" LDFLAGS "/LIBPATH:\"C:\\Program Files\\SDK\\lib\"")
if(NOT got_inc STREQUAL "/I C:/sdk/include;extra")
  message(FATAL_ERROR "INC parse=[${got_inc}]")
endif()
if(NOT got_ld STREQUAL "/LIBPATH:\"C:\\Program Files\\SDK\\lib\"")
  message(FATAL_ERROR "LD parse=[${got_ld}]")
endif()

add_library(win SHARED probe.c)
nrn_mech_add_user_cli(win COMPILE_OPTIONS "/I\"C:\\Program Files\\SDK\\include\"" WINDOWS)
nrn_mech_add_user_cli(win COMPILE_OPTIONS "/I C:/sdk/include;extra" WINDOWS)
nrn_mech_add_user_cli(win LINK_OPTIONS "/LIBPATH:\"C:\\Program Files\\SDK\\lib\"" WINDOWS)

add_library(uni SHARED probe.c)
nrn_mech_add_user_cli(uni COMPILE_OPTIONS "/I\"C:\\Program Files\\SDK\\include\"" UNIX)
"""


def _target_blob(ninja, name):
    """Compile and link bindings for one library.

    CMake 3.20 on Unix lists the libraries alphabetically, so the Unix target
    comes first. Windows lists them in definition order. A drive letter on a
    build edge is ``$:``.
    """
    lines = ninja.splitlines()
    chunks = []
    i = 0
    while i < len(lines):
        line = lines[i]
        hit = (
            ("CMakeFiles/%s.dir" % name) in line
            or line.startswith("build lib%s." % name)
            or line.startswith("build %s:" % name)
        )
        if not hit:
            i += 1
            continue
        block = [line]
        i += 1
        while i < len(lines) and lines[i][:1] in (" ", "\t"):
            block.append(lines[i])
            i += 1
        chunks.append("\n".join(block))
    text = "\n".join(chunks).replace("$:", ":")
    if ("CMakeFiles/%s.dir" % name) not in text:
        raise AssertionError("no ninja rules for %s" % name)
    return text


def _assert_platform_flags(win, uni):
    """Windows keeps ';' and '\\'. Unix eats '\\'. Quotes and slash count vary."""
    assert "C:/sdk/include;extra" in win
    assert re.search(r"/IC:\\+Program Files\\+SDK\\+include", win)
    assert "Program FilesSDKinclude" not in win
    assert re.search(r"/LIBPATH:C:\\+Program Files\\+SDK\\+lib", win)
    assert "Program FilesSDKinclude" in uni
    assert "include;extra" not in uni


def test_generated_command_keeps_semicolon_and_windows_backslash(tmp_path):
    """Ninja argv, not the -D string. Windows \\ stays. ';' is not dropped."""
    text = MECH.read_text(encoding="utf-8")
    assert "cmake_parse_arguments(PARSE_ARGV 0" in text
    assert "SHELL:${NRN_MECH_INCFLAGS}" not in text
    (tmp_path / "probe.c").write_text(
        "int probe(void) { return 0; }\n", encoding="utf-8"
    )
    (tmp_path / "CMakeLists.txt").write_text(
        CMAKE.replace("@MECH@", MECH.as_posix()).replace("@CC@", _probe_compiler()),
        encoding="utf-8",
    )
    build = tmp_path / "b"
    subprocess.check_call(
        ["cmake", "-S", str(tmp_path), "-B", str(build), "-G", "Ninja"],
        cwd=tmp_path,
        env=_cmake_env(),
    )
    ninja = (build / "build.ninja").read_text(encoding="utf-8")
    win = _target_blob(ninja, "win")
    uni = _target_blob(ninja, "uni")
    _assert_platform_flags(win, uni)


REPEAT = r"""
cmake_minimum_required(VERSION 3.20)
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_C_COMPILER "@CC@")
set(CMAKE_C_COMPILER_ID "GNU")
set(CMAKE_C_COMPILE_OPTIONS_PIC "")
project(repeatprobe C)
include("@MECH@")
add_library(rep SHARED probe.c)
nrn_mech_add_user_cli(rep COMPILE_OPTIONS "-I dir1 -I dir2" UNIX)
nrn_mech_add_user_cli(rep LINK_OPTIONS "-L lib1 -L lib2 -l a -l b" UNIX)
add_library(repw SHARED probe.c)
nrn_mech_add_user_cli(repw COMPILE_OPTIONS "/I dir1 /I dir2" WINDOWS)
nrn_mech_add_user_cli(repw LINK_OPTIONS "/LIBPATH:lib1 /LIBPATH:lib2" WINDOWS)
"""


def test_repeated_include_and_link_options_keep_their_flags(tmp_path):
    """-I dir1 -I dir2 stays two include groups. CMake must not drop the second -I."""
    (tmp_path / "probe.c").write_text(
        "int probe(void) { return 0; }\n", encoding="utf-8"
    )
    (tmp_path / "CMakeLists.txt").write_text(
        REPEAT.replace("@MECH@", MECH.as_posix()).replace("@CC@", _probe_compiler()),
        encoding="utf-8",
    )
    build = tmp_path / "b"
    subprocess.check_call(
        ["cmake", "-S", str(tmp_path), "-B", str(build), "-G", "Ninja"],
        cwd=tmp_path,
        env=_cmake_env(),
    )
    ninja = (build / "build.ninja").read_text(encoding="utf-8")
    unix = _target_blob(ninja, "rep")
    win = _target_blob(ninja, "repw")
    assert "-I dir1 -I dir2" in unix or ("-Idir1" in unix and "-Idir2" in unix)
    assert "-I dir1 dir2" not in unix
    assert "-L lib1 -L lib2" in unix or ("-Llib1" in unix and "-Llib2" in unix)
    assert "-L lib1 lib2" not in unix
    assert "-la" in unix and "-lb" in unix
    assert "/Idir1" in win and "/Idir2" in win
    assert "/LIBPATH:lib1" in win and "/LIBPATH:lib2" in win


GEN = r"""
cmake_minimum_required(VERSION 3.20)
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)
set(CMAKE_C_COMPILER "@CC@")
set(CMAKE_CXX_COMPILER "@CC@")
set(CMAKE_C_COMPILER_ID "GNU")
set(CMAKE_CXX_COMPILER_ID "GNU")
set(CMAKE_C_COMPILE_OPTIONS_PIC "")
set(CMAKE_CXX_COMPILE_OPTIONS_PIC "")
project(genprobe C CXX)
include("@MECH@")
set(_NEURON_MECH_REG "@REG@")
set(_NEURON_MAIN_INCLUDE_DIR "@SRC@")
add_library(nrniv_stub SHARED probe.c)
add_library(neuron::nrniv ALIAS nrniv_stub)
set(options --foo --bar)
create_nrnmech(
  NEURON NMODL_NEURON_CODEGEN
  MOD_FILES "@MOD@"
  NMODL_EXECUTABLE "@CC@"
  NMODL_NEURON_EXTRA_ARGS "${options}"
  INCFLAGS "/I C:/sdk/include;extra"
  TARGET_LIBRARY_NAME genprobe
  LIBRARY_OUTPUT_DIR "@SRC@"
  ARTIFACTS_OUTPUT_DIR "@SRC@")
# Generated .cpp sources are not on disk yet, so the language is not inferred.
set_target_properties(genprobe PROPERTIES LINKER_LANGUAGE CXX)
"""


def test_create_nrnmech_keeps_quoted_list_as_separate_arguments(tmp_path):
    """NMODL_NEURON_EXTRA_ARGS "${options}" is two generator arguments, not one."""
    (tmp_path / "probe.c").write_text(
        "int probe(void) { return 0; }\n", encoding="utf-8"
    )
    mod = tmp_path / "a.mod"
    mod.write_text("TITLE a\n", encoding="utf-8")
    text = GEN
    for key, value in (
        ("@MECH@", MECH.as_posix()),
        ("@REG@", MOD_REG.as_posix()),
        ("@CC@", _probe_compiler()),
        ("@MOD@", mod.as_posix()),
        ("@SRC@", tmp_path.as_posix()),
    ):
        text = text.replace(key, value)
    (tmp_path / "CMakeLists.txt").write_text(text, encoding="utf-8")
    build = tmp_path / "b"
    subprocess.check_call(
        ["cmake", "-S", str(tmp_path), "-B", str(build), "-G", "Ninja"],
        cwd=tmp_path,
        env=_cmake_env(),
    )
    ninja = (build / "build.ninja").read_text(encoding="utf-8")
    assert "--foo;--bar" not in ninja
    assert "--foo" in ninja and "--bar" in ninja
    # The semicolon flag is still one argument. The quoted list was not that case.
    assert "include;extra" in ninja


def test_cmake_env_drops_sanitizer_preload(monkeypatch):
    """libtsan must not reach cmake. The parent environment stays intact."""
    monkeypatch.setenv("LD_PRELOAD", "/usr/lib/gcc/x86_64-linux-gnu/12/libtsan.so")
    monkeypatch.setenv("DYLD_INSERT_LIBRARIES", "/tmp/libclang_rt.tsan.dylib")
    monkeypatch.setenv("NRN_SANITIZER_PRELOAD_VAR", "LD_PRELOAD")
    monkeypatch.setenv("CC", "gcc-12")
    env = _cmake_env()
    assert "LD_PRELOAD" not in env
    assert "DYLD_INSERT_LIBRARIES" not in env
    assert "CC" not in env
    assert os.environ["LD_PRELOAD"].endswith("libtsan.so")
    assert os.environ["CC"] == "gcc-12"


def test_ninja_blob_accepts_either_target_order_and_escaped_colons():
    """Unix lists uni first and escapes ':'. Windows quotes nothing and uses one '\\'."""
    unix_order = "\n".join(
        [
            "build CMakeFiles/uni.dir/probe.c.o: C_COMPILER__uni_ ../probe.c",
            '  FLAGS = "/IC:Program FilesSDKinclude"',
            "build libuni.so: C_SHARED_LIBRARY_LINKER__uni_ CMakeFiles/uni.dir/probe.c.o",
            "  LINK_FLAGS = -shared",
            "build CMakeFiles/win.dir/probe.c.obj: C_COMPILER__win_ ../probe.c",
            '  FLAGS = "/IC$:\\\\Program Files\\\\SDK\\\\include" /I "C$:/sdk/include;extra"',
            "build libwin.so: C_SHARED_LIBRARY_LINKER__win_ CMakeFiles/win.dir/probe.c.obj",
            '  LINK_FLAGS = "/LIBPATH:C$:\\\\Program Files\\\\SDK\\\\lib"',
        ]
    )
    windows = "\n".join(
        [
            r"build CMakeFiles/win.dir/probe.c.obj: C_COMPILER__win_unscanned_ C$:/Users/probe.c",
            r'  FLAGS = "/IC:\Program Files\SDK\include" /I C:/sdk/include;extra',
            "build libwin.dll libwin.dll.a: C_SHARED_LIBRARY_LINKER__win_ "
            "CMakeFiles/win.dir/probe.c.obj",
            r'  LINK_FLAGS = -shared "/LIBPATH:C:\Program Files\SDK\lib"',
            r"build CMakeFiles/uni.dir/probe.c.obj: C_COMPILER__uni_unscanned_ C$:/Users/probe.c",
            r'  FLAGS = "/IC:Program FilesSDKinclude"',
            "build libuni.dll: C_SHARED_LIBRARY_LINKER__uni_ CMakeFiles/uni.dir/probe.c.obj",
        ]
    )
    for ninja in (unix_order, windows):
        _assert_platform_flags(_target_blob(ninja, "win"), _target_blob(ninja, "uni"))
