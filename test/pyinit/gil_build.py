"""Reject a free-threaded CPython build before native code loads.

Py_GIL_DISABLED is the interpreter build. 1 and the string "1" are
free-threaded. 0, the string "0", and None are GIL builds. Enabling the GIL
at runtime does not make a free-threaded build acceptable. The check runs on
import, in nrnpyenv.sh, and, when dynamic loading is on, before dlopen when
NRN_PYLIB, NRN_PYTHONEXE, and NRN_PYTHONVERSION are all set.

No free-threaded interpreter is required. A stand-in executable runs the real
probe against a stub sysconfig while this process still has the GIL enabled.
"""

import os
import stat
import subprocess
import sys
import tempfile

PHRASE = "free-threaded CPython build (Py_GIL_DISABLED)"
SHELL_PROBE = "raise SystemExit(v in (1, '1'))"
CPP_PROBE = "print(1 if v in (1, '1') else 0)"

STANDIN = r"""
import os
import sys
import types

raw = os.environ.get("NRN_TEST_PY_GIL_DISABLED", "int0")
if raw == "int1":
    value = 1
elif raw == "str1":
    value = "1"
elif raw == "int0":
    value = 0
elif raw == "str0":
    value = "0"
elif raw == "none":
    value = None
else:
    raise SystemExit("bad NRN_TEST_PY_GIL_DISABLED")

mod = types.ModuleType("sysconfig")


def get_config_var(name):
    if name == "Py_GIL_DISABLED":
        return value
    raise SystemExit("unexpected config var " + str(name))


mod.get_config_var = get_config_var
sys.modules["sysconfig"] = mod

args = sys.argv[1:]
if args and args[0] == "-S":
    args = args[1:]
if len(args) < 2 or args[0] != "-c":
    raise SystemExit("stand-in expects -c")
log = os.environ.get("NRN_TEST_GIL_LOG")
if log:
    with open(log, "a", encoding="ascii") as out:
        out.write(args[1] + "\n---\n")
exec(args[1], {"__name__": "__main__"})
"""


def fail(message, proc=None):
    if proc is not None:
        sys.stderr.buffer.write(proc.stdout or b"")
        sys.stderr.buffer.write(proc.stderr or b"")
        sys.stderr.write("status {}\n".format(proc.returncode))
    raise SystemExit(message)


def run(cmd, env):
    return subprocess.run(cmd, env=env, capture_output=True, timeout=90)


def text(proc):
    return (proc.stdout + proc.stderr).decode("utf-8", "replace")


def write_standin(path, python):
    with open(path, "w", encoding="ascii", newline="\n") as out:
        out.write("#!" + python + "\n")
        out.write(STANDIN.lstrip("\n"))
    os.chmod(path, os.stat(path).st_mode | stat.S_IEXEC | stat.S_IXGRP | stat.S_IXOTH)


def import_code(value_src):
    return (
        "import sys\n"
        "import sysconfig\n"
        "value = {}\n"
        "real = sysconfig.get_config_var\n"
        "def get_config_var(name):\n"
        "    if name == 'Py_GIL_DISABLED':\n"
        "        return value\n"
        "    return real(name)\n"
        "sysconfig.get_config_var = get_config_var\n"
        "gil_on = (not hasattr(sys, '_is_gil_enabled')) or bool(sys._is_gil_enabled())\n"
        "print('GIL-ON:' + str(gil_on))\n"
        "try:\n"
        "    import neuron\n"
        "except ImportError as exc:\n"
        "    print('IMPORT-ERROR:' + str(exc))\n"
        "    loaded = ('neuron.hoc' in sys.modules) or ('hoc' in sys.modules)\n"
        "    print('HOC:' + str(loaded))\n"
        "else:\n"
        "    print('IMPORT-OK')\n"
        "    loaded = ('neuron.hoc' in sys.modules) or ('hoc' in sys.modules)\n"
        "    print('HOC:' + str(loaded))\n"
    ).format(value_src)


def main():
    if len(sys.argv) != 4:
        raise SystemExit("usage: gil_build.py nrniv python dynamic")
    nrniv = sys.argv[1]
    pyexe = sys.argv[2]
    dynamic = sys.argv[3] == "1"
    script = os.path.join(os.path.dirname(nrniv), "nrnpyenv.sh")
    if not os.path.isfile(script):
        raise SystemExit("nrnpyenv.sh is not next to nrniv: " + script)

    base = os.environ.copy()
    for key in (
        "NRN_PYLIB",
        "NRN_PYTHONEXE",
        "NRN_PYTHONVERSION",
        "NRN_TEST_PY_GIL_DISABLED",
        "NRN_TEST_GIL_LOG",
    ):
        base.pop(key, None)

    probe = run(
        [
            pyexe,
            "-S",
            "-c",
            "import sys, sysconfig\n"
            "v = sysconfig.get_config_var('Py_GIL_DISABLED')\n"
            "kind = {1: 'int1', '1': 'str1', 0: 'int0', '0': 'str0', None: 'none'}.get(v)\n"
            "gil_on = (not hasattr(sys, '_is_gil_enabled')) or bool(sys._is_gil_enabled())\n"
            "print('KIND:' + (kind or ('other:' + repr(v))))\n"
            "print('GIL-ON:' + str(gil_on))\n",
        ],
        base,
    )
    probed = text(probe)
    if probe.returncode != 0 or "GIL-ON:True" not in probed:
        fail("need a GIL-enabled runtime so the old check would accept it", probe)
    if (
        "KIND:int0" not in probed
        and "KIND:str0" not in probed
        and "KIND:none" not in probed
    ):
        fail("this interpreter is already a free-threaded build", probe)

    for value_src in ("1", "'1'"):
        proc = run([pyexe, "-c", import_code(value_src)], base)
        out = text(proc)
        if (
            proc.returncode != 0
            or "IMPORT-ERROR:" not in out
            or PHRASE not in out
            or "HOC:False" not in out
            or "GIL-ON:True" not in out
            or "IMPORT-OK" in out
        ):
            fail("import accepted Py_GIL_DISABLED {}".format(value_src), proc)

    for value_src in ("0", "'0'", "None"):
        proc = run([pyexe, "-c", import_code(value_src)], base)
        out = text(proc)
        if proc.returncode != 0 or "IMPORT-OK" not in out or "HOC:True" not in out:
            fail("import rejected Py_GIL_DISABLED {}".format(value_src), proc)

    plain = run([pyexe, "-c", "import neuron\nprint('IMPORT-OK')\n"], base)
    if plain.returncode != 0 or b"IMPORT-OK" not in plain.stdout:
        fail("unpatched import failed", plain)

    version = run(
        [pyexe, "-S", "-c", "import sys; print('{}.{}'.format(*sys.version_info[:2]))"],
        base,
    )
    pyver = version.stdout.decode("ascii", "replace").strip()
    if version.returncode != 0 or not pyver:
        fail("could not read the Python version", version)

    with tempfile.TemporaryDirectory(prefix="nrn-gil-build-") as tmp:
        standin = os.path.join(tmp, "python")
        log = os.path.join(tmp, "probes.txt")
        write_standin(standin, pyexe)
        env = base.copy()
        env["NRN_TEST_GIL_LOG"] = log

        real = run(["bash", script, pyexe], env)
        if real.returncode != 0 or b"export NRN_PYLIB=" not in real.stdout:
            fail("nrnpyenv.sh rejected the GIL build", real)

        for mode in ("int1", "str1"):
            open(log, "w").close()
            env["NRN_TEST_PY_GIL_DISABLED"] = mode
            proc = run(["bash", script, standin], env)
            logged = open(log, encoding="ascii").read()
            out = text(proc)
            if (
                proc.returncode == 0
                or PHRASE not in out
                or b"export NRN_PYLIB=" in proc.stdout
                or "Py_GIL_DISABLED" not in logged
                or SHELL_PROBE not in logged
            ):
                sys.stderr.write(logged)
                fail("nrnpyenv.sh accepted free-threaded mode {}".format(mode), proc)

        for mode in ("int0", "str0", "none"):
            open(log, "w").close()
            env["NRN_TEST_PY_GIL_DISABLED"] = mode
            proc = run(["bash", script, standin], env)
            logged = open(log, encoding="ascii").read()
            if (
                proc.returncode != 0
                or b"export NRN_PYLIB=" not in proc.stdout
                or SHELL_PROBE not in logged
            ):
                sys.stderr.write(logged)
                fail("nrnpyenv.sh rejected GIL mode {}".format(mode), proc)

        if dynamic:
            lib = run(
                [
                    pyexe,
                    "-S",
                    "-c",
                    "import os, sysconfig\n"
                    "libdir = sysconfig.get_config_var('LIBDIR') or ''\n"
                    "ld = sysconfig.get_config_var('LDLIBRARY') or ''\n"
                    "print(ld if os.path.isabs(ld) else os.path.join(libdir, ld))\n",
                ],
                base,
            )
            pylib = lib.stdout.decode("utf-8", "replace").strip()
            if lib.returncode != 0 or not os.path.isfile(pylib):
                fail("could not find libpython at {!r}".format(pylib), lib)

            for mode in ("int1", "str1"):
                open(log, "w").close()
                env["NRN_TEST_PY_GIL_DISABLED"] = mode
                env["NRN_PYLIB"] = os.path.join(tmp, "no-such-libpython")
                env["NRN_PYTHONEXE"] = standin
                env["NRN_PYTHONVERSION"] = pyver
                proc = run(
                    [
                        nrniv,
                        "-notatty",
                        "-python",
                        "-c",
                        "print('gil-build-should-not-run')",
                    ],
                    env,
                )
                logged = open(log, encoding="ascii").read()
                out = text(proc)
                if (
                    proc.returncode == 0
                    or PHRASE not in out
                    or "Could not dlopen" in out
                    or b"gil-build-should-not-run" in proc.stdout
                    or CPP_PROBE not in logged
                ):
                    sys.stderr.write(logged)
                    fail(
                        "populated NRN_PY* accepted free-threaded mode {}".format(mode),
                        proc,
                    )

            env.pop("NRN_TEST_PY_GIL_DISABLED", None)
            env["NRN_PYLIB"] = pylib
            env["NRN_PYTHONEXE"] = pyexe
            env["NRN_PYTHONVERSION"] = pyver
            proc = run(
                [
                    nrniv,
                    "-notatty",
                    "-python",
                    "-c",
                    "import neuron\nprint('gil-build-nrniv-ok')\n",
                ],
                env,
            )
            if proc.returncode != 0 or b"gil-build-nrniv-ok" not in proc.stdout:
                fail("populated NRN_PY* rejected the GIL build", proc)
        else:
            print("dynamic-path-skipped")

    print("gil-build-ok")


if __name__ == "__main__":
    main()
