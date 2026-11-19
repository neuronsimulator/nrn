"""-incflags and -loadflags must survive CMake and reach the generated command.

The Python -D string is not enough. cmake_parse_arguments splits on ';', and
SHELL: treats a Windows backslash as a Unix escape.
"""

import subprocess
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parents[3]
MECH = REPO_ROOT / "cmake" / "neuronMechMaker.cmake"

CMAKE = r"""
cmake_minimum_required(VERSION 3.20)
project(flagprobe C)
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_C_COMPILER "/bin/echo")
set(CMAKE_C_COMPILER_ID "GNU")
set(CMAKE_C_COMPILE_OPTIONS_PIC "")
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


def _target_span(ninja, target, stop_before):
    """Object rule through the link rule. LINK_FLAGS sits on the next build line."""
    lines = ninja.splitlines()
    start = next(
        i for i, line in enumerate(lines) if "CMakeFiles/%s.dir/" % target in line
    )
    if stop_before is None:
        end = len(lines)
    else:
        end = next(
            i for i, line in enumerate(lines) if i > start and stop_before in line
        )
    return "\n".join(lines[start:end])


def test_generated_command_keeps_semicolon_and_windows_backslash(tmp_path):
    """Ninja argv, not the -D string. Windows \\ stays. ';' is not dropped."""
    text = MECH.read_text(encoding="utf-8")
    assert "cmake_parse_arguments(PARSE_ARGV 0" in text
    assert "SHELL:${NRN_MECH_INCFLAGS}" not in text
    (tmp_path / "probe.c").write_text(
        "int probe(void) { return 0; }\n", encoding="utf-8"
    )
    (tmp_path / "CMakeLists.txt").write_text(
        CMAKE.replace("@MECH@", MECH.as_posix()), encoding="utf-8"
    )
    build = tmp_path / "b"
    subprocess.check_call(
        ["cmake", "-S", str(tmp_path), "-B", str(build), "-G", "Ninja"],
        cwd=tmp_path,
    )
    ninja = (build / "build.ninja").read_text(encoding="utf-8")
    win = _target_span(ninja, "win", "CMakeFiles/uni.dir/")
    uni = _target_span(ninja, "uni", None)
    assert '/I "C:/sdk/include;extra"' in win
    assert r"/IC:\\Program Files\\SDK\\include" in win
    assert "Program FilesSDKinclude" not in win
    assert r"/LIBPATH:C:\\Program Files\\SDK\\lib" in win
    assert "Program FilesSDKinclude" in uni
    assert "include;extra" not in uni
