"""EOF in nrniv -python must not print the InteractiveConsole exit line.

-isatty starts that loop when stdin is a pipe. exitmsg="" keeps the ending quiet.
"""

import subprocess
import sys


def main():
    nrniv = sys.argv[1]
    proc = subprocess.run(
        [nrniv, "-nobanner", "-isatty", "-python"],
        input=b"print(987654)\n",
        capture_output=True,
    )
    combined = proc.stdout + proc.stderr
    if (
        proc.returncode != 0
        or b"987654" not in proc.stdout
        or b"now exiting InteractiveConsole" in combined
    ):
        sys.stderr.buffer.write(proc.stdout)
        sys.stderr.buffer.write(proc.stderr)
        sys.stderr.write("status {}\n".format(proc.returncode))
        raise SystemExit("interactive console eof failed")
    print("interactive-console-ok")


if __name__ == "__main__":
    main()
