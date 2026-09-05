"""MSVC cl.exe lookup for the Windows pip wheel (not setup.exe MinGW)."""

import os
import subprocess
from shutil import which

# Build Tools page; Desktop development with C++ is the workload that provides cl.exe.
MSVC_CXX_MISSING = """\
Windows pip wheels compile MOD files and RxD reactions with Microsoft cl.exe.
The wheel does not ship a compiler (the setup.exe installer still bundles MinGW).

Install Build Tools for Visual Studio with the "Desktop development with C++" workload:
  https://visualstudio.microsoft.com/visual-cpp-build-tools/
Then open an "x64 Native Tools Command Prompt for VS" (or run vcvarsall x64) and retry.
CMake must be on PATH for nrnivmodl. Or set CXX to the full path of cl.exe.
"""


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


def msvc_vc_env():
    """cl.exe needs the vcvars INCLUDE/LIB/PATH. vswhere is the public lookup."""
    vswhere = os.path.join(
        os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"),
        "Microsoft Visual Studio",
        "Installer",
        "vswhere.exe",
    )
    if os.path.isfile(vswhere):
        try:
            inst = subprocess.check_output(
                [
                    vswhere,
                    "-latest",
                    "-products",
                    "*",
                    "-requires",
                    "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
                    "-property",
                    "installationPath",
                ],
                text=True,
            ).strip()
            vcvars = os.path.join(inst, "VC", "Auxiliary", "Build", "vcvarsall.bat")
            if os.path.isfile(vcvars):
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
                    return env
        except (OSError, subprocess.CalledProcessError):
            pass
    try:
        from setuptools._distutils._msvccompiler import _get_vc_env

        extra = _get_vc_env("x86_amd64")
        if extra and _win_env_find_exe(extra, "cl.exe"):
            return extra
    except Exception:
        pass
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
