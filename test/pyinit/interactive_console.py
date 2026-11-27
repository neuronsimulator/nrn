"""EOF in nrniv -python must not print the InteractiveConsole exit line.

-isatty starts that loop when stdin is a pipe. exitmsg="" keeps the ending quiet. The
code.py in the working directory, also named by an empty PYTHONPATH entry, must not replace
the standard library's.
"""

import os
import subprocess
import sys
import tempfile


def main():
    nrniv = sys.argv[1]
    with tempfile.TemporaryDirectory() as directory:
        open(os.path.join(directory, "code.py"), "w").close()
        env = dict(os.environ)
        env["PYTHONPATH"] = os.pathsep + env.get("PYTHONPATH", "")
        proc = subprocess.run(
            [nrniv, "-nobanner", "-isatty", "-python"],
            input=b"import hoc\nprint(int(hoc.HocObject().ref(987654)[0]))\n",
            capture_output=True,
            cwd=directory,
            env=env,
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
