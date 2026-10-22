"""Print the libpython path for NMODL_PYLIB (works on macOS framework builds)."""
from __future__ import annotations

import os
import sys
import sysconfig


def find_libpython() -> str:
    try:
        import find_libpython

        path = find_libpython.find_libpython()
        if path and os.path.isfile(path):
            return path
    except Exception:
        pass

    libdir = sysconfig.get_config_var("LIBDIR") or ""
    ldlibrary = sysconfig.get_config_var("LDLIBRARY") or ""
    ver = f"{sys.version_info.major}.{sys.version_info.minor}"
    fw = sysconfig.get_config_var("PYTHONFRAMEWORKPREFIX") or ""
    candidates = []
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
