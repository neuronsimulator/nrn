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
    """Starting NEURON fails. C exit(1) would kill Python.

    The C call that import neuron makes: the package sets a wheel's NEURONHOME.
    """
    ch = _non_acp_char()
    if ch is None:
        pytest.skip("the active code page can encode the probe characters")
    exe = _wheel_nrniv()
    if exe is None:
        pytest.skip("wheel nrniv.exe is not installed")
    home = "C:/" + ch + "-nrn/share/nrn"
    script = (
        "import ctypes, os, sys\n"
        "os.environ['NEURONHOME'] = sys.argv[1]\n"
        "nrniv = ctypes.PyDLL(sys.argv[2])\n"
        "argv = (ctypes.c_char_p * 4)(b'NEURON', b'-nogui', b'-nopython', None)\n"
        "raise SystemExit(0 if nrniv.nrn_init(3, argv) else 2)\n"
    )
    proc = subprocess.run(
        [sys.executable, "-c", script, home, str(exe.with_suffix(".dll"))],
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
