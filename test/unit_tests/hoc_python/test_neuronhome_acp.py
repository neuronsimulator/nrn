"""A wheel NEURONHOME the ANSI code page cannot store must be refused.

binwrapper sets NEURONHOME, then nrniv.cmd would overwrite it. Launch the
wheel nrniv.exe so the value reaches the wide-environment check.
"""

import os
import subprocess
import sys
from importlib.util import find_spec
from pathlib import Path

import pytest

pytestmark = pytest.mark.skipif(sys.platform != "win32", reason="Windows code page")

# 1252 cannot store these. Euro is in 1252, so it is not a probe.
_PROBE_CHARS = ("\u03b1", "\u042f", "\u4f60")


def _non_acp_char():
    for ch in _PROBE_CHARS:
        try:
            ch.encode("mbcs")
        except UnicodeEncodeError:
            return ch
    return None


def _wheel_nrniv():
    spec = find_spec("neuron")
    if spec is None or not spec.origin:
        return None
    exe = Path(spec.origin).parent / ".data" / "bin" / "nrniv.exe"
    if exe.is_file():
        return exe
    return None


def test_launcher_neuronhome_outside_acp_is_refused():
    """Non-ACP NEURONHOME exits. The directory does not need to exist."""
    ch = _non_acp_char()
    if ch is None:
        pytest.skip("the active code page can encode the probe characters")
    exe = _wheel_nrniv()
    if exe is None:
        pytest.skip("wheel nrniv.exe is not installed")
    env = os.environ.copy()
    env["NEURONHOME"] = "C:/" + ch + "-nrn/share/nrn"
    proc = subprocess.run(
        [str(exe), "-nogui", "-c", "quit()"],
        env=env,
        stdin=subprocess.DEVNULL,
        capture_output=True,
        text=True,
        timeout=30,
        encoding="utf-8",
        errors="replace",
    )
    assert proc.returncode != 0
    assert "does not fit the active ANSI code page" in (proc.stderr or "")


def test_import_neuronhome_outside_acp_does_not_kill_the_interpreter():
    """Loading hoc raises. C exit(1) would skip the except and kill Python."""
    ch = _non_acp_char()
    if ch is None:
        pytest.skip("the active code page can encode the probe characters")
    spec = find_spec("neuron.hoc")
    if spec is None or not spec.origin:
        pytest.skip("neuron.hoc is not installed")
    home = "C:/" + ch + "-nrn/share/nrn"
    script = (
        "import importlib.util, os, sys\n"
        "os.environ['NEURONHOME'] = sys.argv[1]\n"
        "pyd = sys.argv[2]\n"
        "root = os.path.dirname(os.path.abspath(pyd))\n"
        "if hasattr(os, 'add_dll_directory'):\n"
        "    for extra in (\n"
        "        os.path.join(root, '.data', 'bin'),\n"
        "        os.path.join(root, '.data', 'lib'),\n"
        "    ):\n"
        "        if os.path.isdir(extra):\n"
        "            os.add_dll_directory(extra)\n"
        "            os.environ['PATH'] = extra + os.pathsep + os.environ.get('PATH', '')\n"
        "spec = importlib.util.spec_from_file_location('hoc', pyd)\n"
        "mod = importlib.util.module_from_spec(spec)\n"
        "try:\n"
        "    spec.loader.exec_module(mod)\n"
        "except Exception as exc:\n"
        "    sys.stderr.write('%s: %s\\n' % (type(exc).__name__, exc))\n"
        "    raise SystemExit(0)\n"
        "raise SystemExit(2)\n"
    )
    proc = subprocess.run(
        [sys.executable, "-c", script, home, spec.origin],
        stdin=subprocess.DEVNULL,
        capture_output=True,
        text=True,
        timeout=60,
        encoding="utf-8",
        errors="replace",
    )
    assert proc.returncode == 0, proc.stderr
    assert "does not fit the active ANSI code page" in (proc.stderr or "")


def test_acp_neuronhome_still_starts():
    """The real wheel share/nrn is representable and nrniv still exits 0."""
    exe = _wheel_nrniv()
    if exe is None:
        pytest.skip("wheel nrniv.exe is not installed")
    env = os.environ.copy()
    env["NEURONHOME"] = str(exe.parent.parent / "share" / "nrn")
    proc = subprocess.run(
        [str(exe), "-nogui", "-c", "quit()"],
        env=env,
        stdin=subprocess.DEVNULL,
        capture_output=True,
        text=True,
        timeout=30,
        encoding="utf-8",
        errors="replace",
    )
    assert proc.returncode == 0, proc.stderr
