"""nrniv -python must install NEURON's PyOS_ReadlineFunctionPointer hook.

CPython calls that pointer only when stdin and stdout are both ttys
(PyOS_Readline). A pipe, including the interactive-console EOF test, does
not. The pointer is not part of the stable ABI: python3.dll does not export
it, and a versioned Windows build may not load python3.dll at all. Read the
global back and require the function to live in nrniv or nrnpython.

The pointer check closes stdin. On Unix this script then submits one
statement on a pty, after the hook has been shown to point into NEURON, and
requires the product of that statement. Windows CI has no pty.
"""

import os
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


def _check_pointer(nrniv):
    # nrn_istty_ follows stdin. On a tty, -python -c runs the snippet and then
    # stays in the interactive console, so a ctest started from a terminal
    # never sees readline-hook-ok. The pointer is installed before -c and does
    # not need a tty. Same batching as the path_semantics driver.
    proc = subprocess.run(
        [nrniv, "-nobanner", "-python", "-c", INNER],
        stdin=subprocess.DEVNULL,
        capture_output=True,
    )
    combined = proc.stdout + proc.stderr
    if proc.returncode != 0 or b"readline-hook-ok" not in proc.stdout:
        sys.stderr.buffer.write(combined)
        sys.stderr.write("status {}\n".format(proc.returncode))
        raise SystemExit("readline hook check failed")


def _write_all(fd, data):
    view = memoryview(data)
    while view:
        n = os.write(fd, view)
        if n <= 0:
            raise SystemExit("write to the pty failed")
        view = view[n:]


def _read_for(master, buf, predicate, timeout):
    import select
    import time

    deadline = time.monotonic() + timeout
    while not predicate(buf) and time.monotonic() < deadline:
        remaining = deadline - time.monotonic()
        ready, _, _ = select.select([master], [], [], remaining)
        if not ready:
            break
        try:
            chunk = os.read(master, 4096)
        except OSError:
            break
        if not chunk:
            break
        buf.extend(chunk)
    return buf


def _stop(proc):
    if proc.poll() is None:
        proc.kill()
        proc.wait(timeout=5)


def _check_interactive(nrniv):
    # -isatty on a pipe makes NEURON call readline, but CPython still uses
    # fgets unless both stdin and stdout are ttys. GitHub's Windows runner
    # has no pty, so this half cannot run there.
    if sys.platform == "win32":
        print("readline-interactive-skipped")
        return
    import fcntl
    import pty
    import struct
    import termios

    # The pty echoes the typed text, so the success bytes must not already
    # appear there. 101*4193 is 423493. Dl_info.dli_fname is the first pointer.
    lines = [
        "import ctypes",
        "api=ctypes.pythonapi",
        "name='PyOS_ReadlineFunctionPointer'",
        "a=ctypes.c_void_p.in_dll(api, name).value",
        "b=ctypes.create_string_buffer(32)",
        "d=ctypes.CDLL(None)",
        "d.dladdr.argtypes=[ctypes.c_void_p, ctypes.c_void_p]",
        "d.dladdr.restype=ctypes.c_int",
        "ok=d.dladdr(a, ctypes.addressof(b))",
        "fp=ctypes.cast(b, ctypes.POINTER(ctypes.c_void_p))[0]",
        "raw=ctypes.cast(fp, ctypes.c_char_p).value if ok else None",
        "p=raw.decode().lower() if raw else ''",
        "hit=('nrniv' in p) or ('nrnpython' in p)",
        "print('readline-hook-' + ('ok' if hit else 'bad'))",
        "print(101*4193)",
        "exit()",
    ]
    master, slave = pty.openpty()
    # rows, cols. A zero-size pty makes libedit wrap a line into a newline.
    fcntl.ioctl(slave, termios.TIOCSWINSZ, struct.pack("HHHH", 40, 200, 0, 0))

    def _own_tty():
        os.setsid()
        fcntl.ioctl(0, termios.TIOCSCTTY, 0)

    proc = subprocess.Popen(
        [nrniv, "-nobanner", "-nogui", "-python"],
        stdin=slave,
        stdout=slave,
        stderr=slave,
        preexec_fn=_own_tty,
        close_fds=True,
    )
    os.close(slave)
    buf = bytearray()
    try:
        _read_for(master, buf, lambda data: b">>>" in data, 15)
        if b">>>" not in buf:
            raise SystemExit("readline interactive check failed")
        # Queued before the replies. Each input() runs, then the next read.
        # The product line is therefore read after the pointer print has run.
        _write_all(master, ("\n".join(lines) + "\n").encode("ascii"))
        _read_for(
            master,
            buf,
            lambda data: b"readline-hook-ok" in data and b"423493" in data,
            15,
        )
        if b"readline-hook-ok" not in buf or b"423493" not in buf:
            raise SystemExit("readline interactive check failed")
        try:
            proc.wait(timeout=15)
        except subprocess.TimeoutExpired:
            raise SystemExit("readline interactive check failed")
        if proc.returncode != 0:
            raise SystemExit("readline interactive check failed")
    except SystemExit:
        sys.stderr.buffer.write(buf)
        sys.stderr.write("\nstatus {}\n".format(proc.returncode))
        raise
    finally:
        _stop(proc)
        os.close(master)
    print("readline-interactive-ok")


def main():
    nrniv = sys.argv[1]
    _check_pointer(nrniv)
    _check_interactive(nrniv)
    print("readline-hook-ok")


if __name__ == "__main__":
    main()
