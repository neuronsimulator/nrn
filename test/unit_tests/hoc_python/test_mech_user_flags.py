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
    env = os.environ.copy()
    for key in ("CC", "CXX", "CFLAGS", "CXXFLAGS", "LDFLAGS"):
        env.pop(key, None)
    subprocess.check_call(
        ["cmake", "-S", str(tmp_path), "-B", str(build), "-G", "Ninja"],
        cwd=tmp_path,
        env=env,
    )
    ninja = (build / "build.ninja").read_text(encoding="utf-8")
    win = _target_blob(ninja, "win")
    uni = _target_blob(ninja, "uni")
    _assert_platform_flags(win, uni)


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
