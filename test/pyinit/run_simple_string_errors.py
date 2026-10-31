"""nrnpy_run_simple_string prints errors, exits on SystemExit, and clears the error.

assert False must show an AssertionError and exit 1. sys.exit(7) must exit 7.
A later nrnpython() call after an error must still run.
"""

import subprocess
import sys

LATER = b"987654"


def launch(nrniv, args):
    return subprocess.run([nrniv, "-notatty", *args], capture_output=True)


def show(label, proc):
    sys.stderr.buffer.write(label)
    sys.stderr.buffer.write(proc.stdout)
    sys.stderr.buffer.write(proc.stderr)
    sys.stderr.buffer.write("status {}\n".format(proc.returncode).encode("ascii"))


def main():
    nrniv = sys.argv[1]
    failed = launch(nrniv, ["-python", "-c", "assert False"])
    exited = launch(nrniv, ["-python", "-c", "import sys; sys.exit(7)"])
    later = launch(
        nrniv,
        ["-c", 'nrnpython("assert False")', "-c", 'nrnpython("print(987654)")'],
    )
    ok = (
        failed.returncode == 1
        and b"Traceback" in failed.stderr
        and b"AssertionError" in failed.stderr
        and b"Exception ignored" not in failed.stderr
        and exited.returncode == 7
        and b"Traceback" not in exited.stderr
        and b"Exception ignored" not in exited.stderr
        and later.returncode == 0
        and b"AssertionError" in later.stderr
        and LATER in later.stdout
        and b"Exception ignored" not in later.stderr
    )
    if not ok:
        show(b"--- assert False ---\n", failed)
        show(b"--- sys.exit(7) ---\n", exited)
        show(b"--- later nrnpython ---\n", later)
        raise SystemExit("run-simple-string failed")
    print("run-simple-string-ok")


if __name__ == "__main__":
    main()
