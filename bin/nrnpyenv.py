#!/usr/bin/env python
"""Print NRN_PY* exports so nrniv can load libpython without bash nrnpyenv.sh.

C++ parses lines of the form:
  export NRN_PYTHONEXE="..."
  export NRN_PYTHONVERSION="..."
  export NRN_PYLIB="..."

Unix and MinGW still use nrnpyenv.sh (Anaconda/cygcheck/lsof). This script is
the MSVC path: pythonXY.dll is a Windows ABI fact, not a MinGW shell helper.
An optional first argument is a Python executable to query instead of this one.
"""
import os
import sys


def _win_pylib():
    """Path of pythonXY.dll for this interpreter."""
    name = "python{}{}.dll".format(*sys.version_info[:2])
    roots = [
        os.path.dirname(os.path.abspath(sys.executable)),
        sys.base_prefix,
        os.path.join(sys.base_prefix, "DLLs"),
    ]
    for root in roots:
        cand = os.path.join(root, name)
        if os.path.isfile(cand):
            return cand
    # Venv python.exe is a launcher; the DLL is already loaded in this process.
    try:
        import ctypes
        from ctypes import wintypes

        k32 = ctypes.WinDLL("kernel32", use_last_error=True)
        k32.GetModuleHandleW.restype = wintypes.HMODULE
        k32.GetModuleHandleW.argtypes = [wintypes.LPCWSTR]
        k32.GetModuleFileNameW.restype = wintypes.DWORD
        k32.GetModuleFileNameW.argtypes = [
            wintypes.HMODULE,
            wintypes.LPWSTR,
            wintypes.DWORD,
        ]
        handle = k32.GetModuleHandleW(name)
        if handle:
            buf = ctypes.create_unicode_buffer(32768)
            n = k32.GetModuleFileNameW(handle, buf, len(buf))
            if n:
                return buf.value
    except Exception:
        pass
    return None


def _posix_pylib():
    import sysconfig

    libdir = sysconfig.get_config_var("LIBDIR")
    if not libdir or not os.path.isdir(libdir):
        return None
    ver = "{}.{}".format(*sys.version_info[:2])
    try:
        names = os.listdir(libdir)
    except OSError:
        return None
    for fname in names:
        if "libpython" in fname and ver in fname and ".so" in fname:
            return os.path.join(libdir, fname)
    return None


def find_pylib():
    if sys.platform.startswith("win"):
        return _win_pylib()
    return _posix_pylib()


def main(argv):
    if len(argv) > 1 and argv[1]:
        other = argv[1]
        if os.path.abspath(other) != os.path.abspath(sys.executable):
            if not os.path.isfile(other):
                sys.stderr.write("nrnpyenv.py: not a file: %s\n" % other)
                return 1
            # argv list, not a shell. other is a python executable path.
            os.execv(other, [other, os.path.abspath(__file__)])  # NOSONAR
    pylib = find_pylib()
    print('export NRN_PYTHONEXE="%s"' % sys.executable)
    print('export NRN_PYTHONVERSION="%d.%d"' % sys.version_info[:2])
    if pylib:
        print('export NRN_PYLIB="%s"' % pylib)
        return 0
    return 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
