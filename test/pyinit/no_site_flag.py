"""-Py_NoSiteFlag must keep a marker sitecustomize from running.

Py_Initialize copies Py_NoSiteFlag into site_import. The marker module is on
PYTHONPATH. Without the flag it prints and site is loaded. With the flag the
marker must not print, site must stay out of sys.modules, and the ignore
warning must stay absent. The same script runs on Windows.
"""

import os
import subprocess
import sys
import tempfile

MARKER = b"NRN-SITECUSTOMIZE-RAN"
CHECKED = b"NRN-NOSITE-CHECKED"
SITE_LOADED = b"SITE-LOADED"
SITE_ABSENT = b"SITE-ABSENT"
WARNING = b"Py_NoSiteFlag is ignored"


def launch(nrniv, env, nosite):
    command = [nrniv, "-notatty"]
    if nosite:
        command.append("-Py_NoSiteFlag")
    command.extend(
        [
            "-python",
            "-c",
            "import sys\n"
            "print('SITE-LOADED' if 'site' in sys.modules else 'SITE-ABSENT')\n"
            "print('NRN-NOSITE-CHECKED')\n",
        ]
    )
    return subprocess.run(command, env=env, capture_output=True)


def main():
    nrniv = sys.argv[1]
    with tempfile.TemporaryDirectory(prefix="nrn-nosite-") as tmp:
        with open(
            os.path.join(tmp, "sitecustomize.py"), "w", encoding="ascii", newline="\n"
        ) as out:
            out.write("print('NRN-SITECUSTOMIZE-RAN', flush=True)\n")
        env = os.environ.copy()
        env.pop("PYTHONSTARTUP", None)
        env["PYTHONNOUSERSITE"] = "1"
        previous = env.get("PYTHONPATH")
        env["PYTHONPATH"] = tmp if not previous else tmp + os.pathsep + previous
        plain = launch(nrniv, env, False)
        flagged = launch(nrniv, env, True)
        ok = (
            plain.returncode == 0
            and MARKER in plain.stdout
            and SITE_LOADED in plain.stdout
            and flagged.returncode == 0
            and MARKER not in flagged.stdout
            and MARKER not in flagged.stderr
            and SITE_ABSENT in flagged.stdout
            and CHECKED in flagged.stdout
            and WARNING not in flagged.stderr
            and WARNING not in flagged.stdout
        )
        if not ok:
            sys.stderr.buffer.write(b"--- without flag ---\n")
            sys.stderr.buffer.write(plain.stdout)
            sys.stderr.buffer.write(plain.stderr)
            sys.stderr.buffer.write("plain status {}\n".format(plain.returncode).encode("ascii"))
            sys.stderr.buffer.write(b"--- with -Py_NoSiteFlag ---\n")
            sys.stderr.buffer.write(flagged.stdout)
            sys.stderr.buffer.write(flagged.stderr)
            sys.stderr.buffer.write(
                "flagged status {}\n".format(flagged.returncode).encode("ascii")
            )
            raise SystemExit("no-site flag failed")
    print("no-site-flag-ok")


if __name__ == "__main__":
    main()
