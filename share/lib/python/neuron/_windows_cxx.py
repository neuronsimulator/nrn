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


# Longer names first so clang-cl.exe is not read as cl.exe.
_MSVC_CXX_BASENAMES = ("clang-cl.exe", "clang-cl", "cl.exe", "cl")

# Longest first. These cl.exe options can be glued to a path: /IC:\sdk\include.
_ATTACHED_MSVC_FLAG_PREFIXES = (
    "/external:env:",
    "/exportheader",
    "/ifcoutput",
    "/headerunit",
    "/external:i",
    "/doc",
    "/fi",
    "/fe",
    "/fo",
    "/fd",
    "/fp",
    "/fa",
    "/fm",
    "/fr",
    "/tp",
    "/tc",
    "/i",
)


def _attached_msvc_flag(token):
    """True for /I or /Fe glued to a path. /bar\\cl.exe is a path, not a flag."""
    low = token.lower()
    for prefix in _ATTACHED_MSVC_FLAG_PREFIXES:
        if not low.startswith(prefix) or len(low) == len(prefix):
            continue
        rest = token[len(prefix) :]
        if rest[:1] in "\\/":
            return True
        if len(rest) >= 3 and rest[1] == ":" and rest[2] in "\\/":
            return True
    return False


def _is_cxx_flag(token):
    """A compiler option. A later token with \\ is still a directory name."""
    if not token:
        return False
    if token[0] in "-@":
        return True
    if token[0] != "/":
        return False
    if _attached_msvc_flag(token):
        return True
    # /nologo, /O2, /std:c++17. /usr/bin/g++ and /bar\\cl.exe are paths.
    if "\\" in token or "/" in token[1:]:
        return False
    return True


def _cxx_executable(cxx):
    """Compiler path from CXX. A cl.exe path that contains spaces stays one token.

    Quoted text is the executable, quotes removed. Otherwise tokens belong to
    the path until a flag. ``/I C:\\sdk\\include`` is not another directory of
    an unquoted Program Files path. ``C:\\cl tools\\bin\\g++.exe`` is one GNU
    path: the ``cl`` component is not cl.exe.
    """
    text = (cxx or "").strip()
    if not text:
        return ""
    if text[0] in "\"'":
        quote = text[0]
        end = text.find(quote, 1)
        if end < 0:
            body = text[1:].strip()
            return body.split()[0] if body else ""
        return text[1:end]
    parts = text.split()
    end = 0
    for i, part in enumerate(parts):
        if i > 0 and _is_cxx_flag(part):
            break
        end = i + 1
    return " ".join(parts[:end])


def _cxx_basename(path):
    """Last path component. Both separators count, including on a Unix host."""
    return path.replace("\\", "/").rsplit("/", 1)[-1].lower()


def _is_msvc_cxx(cxx):
    return _cxx_basename(_cxx_executable(cxx)) in _MSVC_CXX_BASENAMES


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


def msvc_env_ready(env):
    """True when cl.exe is already on PATH and INCLUDE and LIB are set.

    An x64 Native Tools prompt configures that before vswhere runs.
    """
    if not _win_env_get(env, "INCLUDE") or not _win_env_get(env, "LIB"):
        return False
    return _win_env_find_exe(env, "cl.exe") is not None


def windows_rxd_compiler(env, neuronhome):
    """Compiler for RxD when CXX is unset.

    setup.exe's bundled MinGW is used when that g++ is present. Otherwise
    cl.exe already on PATH, with INCLUDE and LIB set, is the toolchain.
    vswhere runs only when that environment is not already configured.

    Returns (cxx, use_msvc), or None when no compiler is available.
    """
    mingw = os.path.join(
        neuronhome or "",
        "mingw",
        "mingw64",
        "bin",
        "x86_64-w64-mingw32-g++.exe",
    )
    if os.path.isfile(mingw):
        env["PATH"] = os.path.dirname(mingw) + ";" + env.get("PATH", "")
        return mingw, False
    if msvc_env_ready(env):
        return _win_env_find_exe(env, "cl.exe"), True
    vc = msvc_vc_env()
    if not vc:
        return None
    _apply_vc_env(env, vc)
    return _win_env_find_exe(vc, "cl.exe") or "cl.exe", True


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
    _vc_env_cache = None
    return None


def msvc_cl_available():
    """True if cl.exe is on PATH or vswhere/vcvars can find Visual C++ tools."""
    cxx = os.environ.get("CXX")
    if cxx:
        path = _cxx_executable(cxx)
        if path and os.path.isfile(path):
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
