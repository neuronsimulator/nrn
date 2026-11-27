"""MSVC cl.exe / cmake lookup for the Windows pip wheel (not setup.exe MinGW)."""

import os
import shlex
import subprocess
import sys
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


# Basename, without .exe. A leading ccache/distcc word is argv[0], not part of
# the compiler path. ``ccache g++ -O2`` used to become an executable named
# "ccache g++", and ccache then treated -I as its own option.
_CXX_WRAPPERS = ("ccache", "distcc", "sccache", "icecc")


def _split_unix_words(text):
    """POSIX argv words. ``#`` is not a comment: it can sit inside a -D value."""
    lexer = shlex.shlex(text, posix=True)
    lexer.whitespace_split = True
    lexer.commenters = ""
    return list(lexer)


def _split_windows_words(text):
    """CommandLine-style words. Backslash is a path character, not an escape.

    POSIX shlex eats ``\\`` in ``C:\\sdk``. These tests run on Linux, so a
    string that contains a backslash uses this splitter even on Unix.
    """
    words = []
    i = 0
    n = len(text)
    while i < n:
        while i < n and text[i] in " \t":
            i += 1
        if i >= n:
            break
        buf = []
        if text[i] in "\"'":
            quote = text[i]
            i += 1
            while i < n and text[i] != quote:
                buf.append(text[i])
                i += 1
            if i < n:
                i += 1
            words.append("".join(buf))
            continue
        while i < n and text[i] not in " \t":
            if text[i] in "\"'":
                quote = text[i]
                i += 1
                while i < n and text[i] != quote:
                    buf.append(text[i])
                    i += 1
                if i < n:
                    i += 1
                continue
            buf.append(text[i])
            i += 1
        words.append("".join(buf))
    return words


def _split_cxx_words(text):
    if sys.platform == "win32" or "\\" in text:
        return _split_windows_words(text)
    return _split_unix_words(text)


def _is_cxx_wrapper(token):
    base = token.replace("\\", "/").rsplit("/", 1)[-1].lower()
    if base.endswith(".exe"):
        base = base[:-4]
    return base in _CXX_WRAPPERS


def _cxx_tokens(cxx):
    """Wrapper-and-compiler argv prefix, then the flags that follow.

    ``ccache g++ -O2`` is ``["ccache", "g++"]`` and ``["-O2"]``. An unquoted
    ``C:\\Program Files\\...\\cl.exe`` stays one path until a flag.
    ``/I C:\\sdk\\include`` is not another directory of that path. A quoted
    flag value, including one with spaces, stays one argument.
    """
    text = (cxx or "").strip()
    if not text:
        return [], []
    words = _split_cxx_words(text)
    if not words:
        return [], []
    prefix = []
    i = 0
    while i < len(words) and _is_cxx_wrapper(words[i]):
        prefix.append(words[i])
        i += 1
    if i < len(words) and not _is_cxx_flag(words[i]):
        end = i + 1
        while end < len(words) and not _is_cxx_flag(words[end]):
            end += 1
        prefix.append(" ".join(words[i:end]))
        i = end
    return prefix, words[i:]


def _cxx_command_prefix(cxx):
    """argv words before user flags. A compiler wrapper stays in front."""
    return _cxx_tokens(cxx)[0]


def _cxx_executable(cxx):
    """Compiler path from CXX. A cl.exe path that contains spaces stays one token."""
    prefix = _cxx_command_prefix(cxx)
    return prefix[-1] if prefix else ""


def _is_cl_exe_name(token):
    """True for cl.exe. clang-cl is a different compiler."""
    base = _cxx_basename(token)
    if base.endswith(".exe"):
        base = base[:-4]
    return base == "cl"


def _cxx_resolve_exe(token, vc):
    """Existing executable for a compiler token, or None.

    A token that contains a directory separator uses isfile. A bare name is
    looked up on PATH from the Visual Studio environment. ``clang-cl`` on
    PATH is that program. isfile of a bare name only sees the current
    directory.
    """
    if not token:
        return None
    if os.path.isfile(token):
        return token
    if os.path.isabs(token) or "/" in token or "\\" in token:
        return None
    names = [token]
    if not token.lower().endswith(".exe"):
        names.append(token + ".exe")
    if vc is None:
        return None
    for name in names:
        found = _win_env_find_exe(vc, name)
        if found:
            return found
    return None


def _cxx_argv_prefix(cxx, vc=None):
    """Prefix for RxD. A compiler that resolves is kept. The wrapper is kept.

    A bare name is resolved on PATH before any substitution. A full path
    still uses isfile. A missing cl.exe may still be replaced with the
    Visual Studio compiler. A missing clang-cl is not.
    """
    prefix = list(_cxx_command_prefix(cxx))
    if not prefix:
        return [cxx] if cxx else []
    if vc is None or not prefix[-1]:
        return prefix
    resolved = _cxx_resolve_exe(prefix[-1], vc)
    if resolved:
        prefix[-1] = resolved
        return prefix
    if _is_cl_exe_name(prefix[-1]):
        found = _win_env_find_exe(vc, "cl.exe")
        if found:
            prefix[-1] = found
    return prefix


def _cxx_owns_output(token):
    """True when the flag names the object or the DLL RxD writes itself.

    ``-o`` is lowercase. ``-O2`` is an optimization flag. ``-openmp`` is not
    an output file. MSVC ``/Fe``, ``/Fo``, and ``/OUT`` are case-insensitive.
    """
    if token == "--output" or token.startswith("--output="):
        return True
    if token.startswith("-o") and token != "-openmp":
        return True
    low = token.lower()
    if low == "/link":
        return True
    return low.startswith(("/fe", "/fo", "/out"))


def _cxx_user_args(cxx):
    """Flags after the compiler. A flag that names the output file is an error."""
    flags = _cxx_tokens(cxx)[1]
    for token in flags:
        if _cxx_owns_output(token):
            raise ValueError(
                "RxD sets the output file itself and cannot accept %s in CXX. "
                "Include and define flags are forwarded." % token
            )
    return flags


def rxd_compile_command(
    executable, user_args, *, include, src, out, rxdmath, msvc, win, sysroot
):
    """argv for one RxD reaction DLL. User flags sit after the include path.

    executable is a compiler path or a prefix list (ccache, then g++). The
    wrapper stays in front so it does not treat ``-I`` as its own option.
    """
    if isinstance(executable, (list, tuple)):
        prefix = list(executable)
    else:
        prefix = [executable]
    user_args = list(user_args)
    if msvc:
        return [
            *prefix,
            "/nologo",
            "/LD",
            "/EHsc",
            "/O2",
            "/I" + include,
            *user_args,
            src,
            "/Fe" + out,
            "/Fo" + os.path.splitext(out)[0] + ".obj",
            "/link",
            "/LIBPATH:" + os.path.dirname(rxdmath),
            "rxdmath.lib",
            "/EXPORT:reaction",
        ]
    return [
        *prefix,
        "-I" + include,
        *user_args,
        *list(sysroot or []),
        "-shared",
        *(["-fPIC"] if not win else []),
        src,
        rxdmath,
        "-o",
        out,
        *(["-lm"] if not win else []),
    ]


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
