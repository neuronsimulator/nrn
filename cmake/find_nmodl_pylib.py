"""Print the shared libpython path for NMODL_PYLIB.

dlopen needs a shared library. On Windows that is pythonXY.dll or
libpythonX.Y.dll, not the import library CMake links (.lib or .dll.a).
"""
from __future__ import annotations

import os
import sys
import sysconfig


def _config_var(name: str) -> str:
    value = sysconfig.get_config_var(name) or ""
    if value in ("None",):
        return ""
    return value


def _windows_dll_candidates() -> list[str]:
    ver = f"{sys.version_info.major}.{sys.version_info.minor}"
    nodot = f"{sys.version_info.major}{sys.version_info.minor}"
    names: list[str] = []
    for key in ("LDLIBRARY", "DLLLIBRARY", "INSTSONAME"):
        base = os.path.basename(_config_var(key))
        if base and base not in names:
            names.append(base)
    for name in (
        f"python{nodot}.dll",
        f"python{nodot}_d.dll",
        f"libpython{ver}.dll",
        f"libpython{nodot}.dll",
    ):
        if name not in names:
            names.append(name)
    base_prefix = getattr(sys, "base_prefix", sys.prefix)
    libdir = _config_var("LIBDIR")
    lib_parent = os.path.dirname(libdir) if libdir else ""
    dirs = [
        _config_var("BINDIR"),
        base_prefix,
        os.path.join(base_prefix, "bin"),
        os.path.dirname(os.path.realpath(sys.executable)),
        libdir,
        lib_parent,
        os.path.join(lib_parent, "bin") if lib_parent else "",
    ]
    candidates = []
    seen: set[str] = set()
    for directory in dirs:
        if not directory:
            continue
        for name in names:
            path = os.path.join(directory, name)
            if path not in seen:
                seen.add(path)
                candidates.append(path)
    return candidates


def find_libpython() -> str:
    try:
        import find_libpython

        path = find_libpython.find_libpython()
        if path and os.path.isfile(path):
            return path
    except Exception:
        pass

    libdir = _config_var("LIBDIR")
    ldlibrary = _config_var("LDLIBRARY")
    ver = f"{sys.version_info.major}.{sys.version_info.minor}"
    fw = _config_var("PYTHONFRAMEWORKPREFIX")
    candidates: list[str] = []
    if sys.platform == "win32":
        candidates.extend(_windows_dll_candidates())
    if libdir and ldlibrary:
        candidates.append(os.path.join(libdir, ldlibrary))
        candidates.append(os.path.join(libdir, os.path.basename(ldlibrary)))
    if fw:
        candidates.append(
            os.path.join(fw, "Python.framework", "Versions", ver, "Python")
        )
    if libdir:
        candidates.extend(
            (
                os.path.join(libdir, f"libpython{ver}.dylib"),
                os.path.join(libdir, f"libpython{ver}.so"),
            )
        )
    for path in candidates:
        if path and os.path.isfile(path):
            return path
    return ""


if __name__ == "__main__":
    print(find_libpython())
