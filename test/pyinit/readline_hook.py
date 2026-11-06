"""nrniv -python must install NEURON's PyOS_ReadlineFunctionPointer hook.

Python calls that pointer only when stdin and stdout are ttys, so the
interactive-console EOF test (a pipe) does not observe it. The pointer is
not part of the stable ABI: python3.dll does not export it, and a versioned
Windows build may not load python3.dll at all. Read the global back and
require the function to live in nrniv or nrnpython.
"""

import subprocess
import sys

INNER = r"""
import ctypes
import os
import sys


def module_path(addr):
    if sys.platform == "win32":
        kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
        handle = ctypes.c_void_p()
        # FROM_ADDRESS | UNCHANGED_REFCOUNT. The second argument is an address.
        kernel32.GetModuleHandleExW.argtypes = [
            ctypes.c_uint32,
            ctypes.c_void_p,
            ctypes.POINTER(ctypes.c_void_p),
        ]
        kernel32.GetModuleHandleExW.restype = ctypes.c_int
        ok = kernel32.GetModuleHandleExW(0x4 | 0x2, ctypes.c_void_p(addr), ctypes.byref(handle))
        if not ok:
            raise SystemExit("GetModuleHandleExW failed")
        buf = ctypes.create_unicode_buffer(32768)
        kernel32.GetModuleFileNameW.argtypes = [
            ctypes.c_void_p,
            ctypes.c_wchar_p,
            ctypes.c_uint32,
        ]
        kernel32.GetModuleFileNameW.restype = ctypes.c_uint32
        n = kernel32.GetModuleFileNameW(handle, buf, len(buf))
        if not n:
            raise SystemExit("GetModuleFileNameW failed")
        return buf.value
    class Dl_info(ctypes.Structure):
        _fields_ = [
            ("dli_fname", ctypes.c_char_p),
            ("dli_fbase", ctypes.c_void_p),
            ("dli_sname", ctypes.c_char_p),
            ("dli_saddr", ctypes.c_void_p),
        ]
    libdl = ctypes.CDLL(None)
    libdl.dladdr.argtypes = [ctypes.c_void_p, ctypes.POINTER(Dl_info)]
    libdl.dladdr.restype = ctypes.c_int
    info = Dl_info()
    if not libdl.dladdr(ctypes.c_void_p(addr), ctypes.byref(info)) or not info.dli_fname:
        raise SystemExit("dladdr failed for the readline hook")
    return info.dli_fname.decode()


def hook_address():
    try:
        addr = ctypes.c_void_p.in_dll(ctypes.pythonapi, "PyOS_ReadlineFunctionPointer").value
    except ValueError:
        addr = None
    if addr:
        return addr
    if sys.platform != "win32":
        raise SystemExit("PyOS_ReadlineFunctionPointer is missing")
    # GetModuleHandle only. WinDLL(name) would LoadLibrary a python3XY.dll
    # that this process has not loaded, and that copy's pointer is not the hook.
    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel32.GetModuleHandleW.argtypes = [ctypes.c_wchar_p]
    kernel32.GetModuleHandleW.restype = ctypes.c_void_p
    names = []
    pylib = os.environ.get("NRN_PYLIB")
    if pylib:
        names.append(pylib)
        names.append(os.path.basename(pylib))
    names.append("python3.dll")
    names.extend("python3%d.dll" % minor for minor in range(10, 31))
    seen = set()
    for name in names:
        if not name or name in seen:
            continue
        seen.add(name)
        handle = kernel32.GetModuleHandleW(name)
        if not handle:
            continue
        dll = ctypes.WinDLL(name, handle=handle)
        try:
            addr = ctypes.c_void_p.in_dll(dll, "PyOS_ReadlineFunctionPointer").value
        except ValueError:
            continue
        if addr:
            return addr
    raise SystemExit("PyOS_ReadlineFunctionPointer is missing")


path = module_path(hook_address())
base = os.path.basename(path).lower()
if "nrniv" not in base and "nrnpython" not in base:
    raise SystemExit("readline hook is in %s, not nrniv or nrnpython" % path)
print("readline-hook-ok")
"""


def main():
    nrniv = sys.argv[1]
    proc = subprocess.run(
        [nrniv, "-nobanner", "-python", "-c", INNER],
        capture_output=True,
    )
    combined = proc.stdout + proc.stderr
    if proc.returncode != 0 or b"readline-hook-ok" not in proc.stdout:
        sys.stderr.buffer.write(combined)
        sys.stderr.write("status {}\n".format(proc.returncode))
        raise SystemExit("readline hook check failed")
    print("readline-hook-ok")


if __name__ == "__main__":
    main()
