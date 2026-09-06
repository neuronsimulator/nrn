"""MSVC cl.exe / cmake lookup for the Windows pip wheel (not setup.exe MinGW)."""

import os
import subprocess
from shutil import which

# Build Tools page; Desktop development with C++ is the workload that provides cl.exe.
MSVC_CXX_MISSING = """\
Windows pip wheels compile MOD files and RxD reactions with Microsoft cl.exe.
The wheel does not ship a compiler (the setup.exe installer still bundles MinGW).

In Visual Studio Installer, on the Workloads page, select only
"Desktop development with C++" (leave recommended components checked).
  https://visualstudio.microsoft.com/visual-cpp-build-tools/
Then open an "x64 Native Tools Command Prompt for VS" (or run vcvarsall x64)
and retry. Or set CXX to the full path of cl.exe.
"""

NRNIVMODL_WIN_TOOLS_MISSING = """\
nrnivmodl on a Windows pip wheel needs cl.exe and CMake.
The wheel does not ship a compiler (the setup.exe installer still bundles MinGW).

In Visual Studio Installer, on the Workloads page, select only:
  Desktop development with C++
Leave its recommended components checked (MSVC, Windows SDK, CMake).
  https://visualstudio.microsoft.com/visual-cpp-build-tools/

If that workload is already installed, open an "x64 Native Tools Command Prompt
for VS" (or run vcvarsall x64) so cmake and cl.exe are on PATH, then retry.
Or set CXX to the full path of cl.exe.
"""

_vswhere = os.path.join(
    os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"),
    "Microsoft Visual Studio",
    "Installer",
    "vswhere.exe",
)

_vc_env_cache = None
_vc_env_tried = False


def _win_env_get(env, name):
    want = name.lower()
    for k, v in env.items():
        if k.lower() == want:
            return v
    return ""


def _win_env_find_exe(env, exe):
    for p in _win_env_get(env, "PATH").split(";"):
        cand = os.path.join(p, exe)
        if p and os.path.isfile(cand):
            return cand
    return None


def _apply_vc_env(env, vc):
    env.update(vc)
    for name in ("PATH", "INCLUDE", "LIB", "LIBPATH"):
        val = _win_env_get(vc, name)
        if val:
            env[name] = val


def _is_msvc_cxx(cxx):
    name = os.path.basename(cxx.replace('"', "").split()[0]).lower()
    return name in ("cl", "cl.exe", "clang-cl", "clang-cl.exe")


def _vswhere_install_path(require_vc=False):
    if not os.path.isfile(_vswhere):
        return None
    cmd = [_vswhere, "-latest", "-products", "*"]
    if require_vc:
        cmd.extend(["-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64"])
    cmd.extend(["-property", "installationPath"])
    try:
        inst = subprocess.check_output(cmd, text=True).strip()
    except (OSError, subprocess.CalledProcessError):
        return None
    return inst or None


def msvc_vc_env():
    """cl.exe needs the vcvars INCLUDE/LIB/PATH. vswhere is the public lookup."""
    global _vc_env_cache, _vc_env_tried
    if _vc_env_tried:
        return _vc_env_cache
    _vc_env_tried = True
    inst = _vswhere_install_path(require_vc=True)
    if inst:
        vcvars = os.path.join(inst, "VC", "Auxiliary", "Build", "vcvarsall.bat")
        if os.path.isfile(vcvars):
            try:
                blob = subprocess.check_output(
                    f'"{vcvars}" x64 >nul && set',
                    shell=True,
                    text=True,
                    stderr=subprocess.DEVNULL,
                )
                env = {}
                for line in blob.splitlines():
                    if "=" in line:
                        k, _, v = line.partition("=")
                        env[k] = v
                if _win_env_find_exe(env, "cl.exe"):
                    _vc_env_cache = env
                    return _vc_env_cache
            except (OSError, subprocess.CalledProcessError):
                pass
    try:
        from setuptools._distutils._msvccompiler import _get_vc_env

        extra = _get_vc_env("x86_amd64")
        if extra and _win_env_find_exe(extra, "cl.exe"):
            _vc_env_cache = extra
            return _vc_env_cache
    except Exception:
        pass
    _vc_env_cache = None
    return None


def msvc_cl_available():
    """True if cl.exe is on PATH or vswhere/vcvars can find Visual C++ tools."""
    cxx = os.environ.get("CXX")
    if cxx:
        path = cxx.replace('"', "").split()[0]
        if os.path.isfile(path):
            return True
    for name in ("cl.exe", "cl"):
        if which(name):
            return True
    return msvc_vc_env() is not None


def find_cmake():
    """cmake.exe on PATH, under vcvars, Program Files, or VS CMake tools."""
    found = which("cmake") or which("cmake.exe")
    if found:
        return found
    vc = msvc_vc_env()
    if vc:
        found = _win_env_find_exe(vc, "cmake.exe")
        if found:
            return found
    for base in (
        os.environ.get("ProgramFiles", r"C:\Program Files"),
        os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"),
    ):
        cand = os.path.join(base, "CMake", "bin", "cmake.exe")
        if os.path.isfile(cand):
            return cand
    inst = _vswhere_install_path(require_vc=False)
    if inst:
        cand = os.path.join(
            inst,
            "Common7",
            "IDE",
            "CommonExtensions",
            "Microsoft",
            "CMake",
            "CMake",
            "bin",
            "cmake.exe",
        )
        if os.path.isfile(cand):
            return cand
    return None
