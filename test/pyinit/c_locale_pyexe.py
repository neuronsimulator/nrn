"""Non-ASCII -pyexe under LC_ALL=C must round-trip through sys.executable.

Py_DecodeLocale runs before Py_Initialize. Under the C locale the path is
surrogate code points, and ascii/surrogateescape encodes them back to the
original bytes. A throw, or a different byte string, is a failure.
"""

import os
import subprocess
import sys
import tempfile


def main():
    nrniv = sys.argv[1]
    pyexe = sys.argv[2]
    # café, spelled as UTF-8 bytes so the directory does not depend on the
    # driver's locale.
    name = bytes([0x63, 0x61, 0x66, 0xC3, 0xA9])
    with tempfile.TemporaryDirectory(prefix="nrn-c-locale-") as tmp:
        parent = os.fsdecode(os.fsencode(tmp))
        dest = os.path.join(parent, os.fsdecode(name))
        os.mkdir(dest)
        link = os.path.join(dest, "python")
        os.symlink(pyexe, link)
        link_bytes = os.fsencode(link)
        inner = "import os, sys\n" "print('EXEBYTES:' + os.fsencode(sys.executable).hex())\n"
        env = os.environ.copy()
        env["LC_ALL"] = "C"
        for key in ("LANG", "LC_CTYPE", "LC_MESSAGES", "LANGUAGE"):
            env.pop(key, None)
        proc = subprocess.run(
            [nrniv, "-notatty", "-pyexe", os.fsdecode(link_bytes), "-python", "-c", inner],
            env=env,
            capture_output=True,
        )
        marker = b"EXEBYTES:"
        got = None
        for line in proc.stdout.splitlines():
            if line.startswith(marker):
                got = bytes.fromhex(line[len(marker) :].decode("ascii"))
        if proc.returncode != 0 or got != link_bytes:
            sys.stderr.buffer.write(proc.stdout)
            sys.stderr.buffer.write(proc.stderr)
            raise SystemExit(
                "c-locale pyexe failed: status {} got {!r} expected {!r}".format(
                    proc.returncode, got, link_bytes
                )
            )
    print("c-locale-pyexe-ok")


if __name__ == "__main__":
    main()
