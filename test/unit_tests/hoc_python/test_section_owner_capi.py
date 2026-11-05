# A C API interface (nrn_section_owner_hooks_set) owns a section, then NEURON's
# Python module is imported in the same process. The module would install its
# own hooks over the interface's and read its owners as Python Sections
# (h.allsec() aborted); the import now fails until the interface clears its
# hooks. Runs in a subprocess: this process has already imported neuron.
import subprocess
import sys

SCRIPT = r"""
import ctypes
import importlib.machinery
import importlib.util
import os

# Load NEURON without importing neuron: dlopen does not run PyInit_hoc.
neuron_dir = importlib.util.find_spec("neuron").submodule_search_locations[0]
# A build can hold one hoc module per Python version: take this interpreter's.
paths = (os.path.join(neuron_dir, "hoc" + s) for s in importlib.machinery.EXTENSION_SUFFIXES)
hoc_so = next(p for p in paths if os.path.exists(p))
lib = ctypes.PyDLL(hoc_so, mode=ctypes.RTLD_GLOBAL)
argv = (ctypes.c_char_p * 4)(b"capi", b"-nogui", b"-nopython", None)
lib.nrn_init(3, argv)

NAME = ctypes.CFUNCTYPE(ctypes.c_void_p, ctypes.c_void_p)
CELL = ctypes.CFUNCTYPE(ctypes.c_void_p, ctypes.c_void_p)
EQUALS = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p)
name = ctypes.create_string_buffer(b"capi_cell.soma")
hooks = (NAME(lambda sec: ctypes.addressof(name)), CELL(lambda sec: None), EQUALS(lambda s, o: 0))
err = ctypes.create_string_buffer(256)
for f in ("nrn_section_owner_hooks_set", "nrn_section_owner_hooks_clear"):
    getattr(lib, f).restype = ctypes.c_bool
lib.nrn_section_new_owned.restype = ctypes.c_void_p
lib.nrn_section_new_owned.argtypes = [ctypes.c_void_p]
lib.nrn_section_owned_release.argtypes = [ctypes.c_void_p]
lib.nrn_secname.restype = ctypes.c_char_p
lib.nrn_secname.argtypes = [ctypes.c_void_p]

assert lib.nrn_section_owner_hooks_set(*hooks, err, len(err))
owner = ctypes.create_string_buffer(64)  # the interface's object
sec = lib.nrn_section_new_owned(ctypes.addressof(owner))
assert sec

try:
    import neuron
except ImportError as e:
    assert "nrn_section_owner_hooks_set" in str(e), e
    print("import refused")
else:
    raise AssertionError("import neuron succeeded over C API hooks")
assert lib.nrn_secname(sec) == b"capi_cell.soma", lib.nrn_secname(sec)

lib.nrn_section_owned_release(sec)
assert lib.nrn_section_owner_hooks_clear(err, len(err)), err.value
from neuron import h

s = h.Section(name="pysoma")
assert [str(x) for x in h.allsec()] == ["pysoma"]
print("import after clear")
"""


def test_section_owner_capi():
    r = subprocess.run([sys.executable, "-c", SCRIPT], capture_output=True, text=True)
    assert r.returncode == 0, r.stdout + r.stderr
    assert r.stdout.split("\n")[-3:-1] == [
        "import refused",
        "import after clear",
    ], r.stdout


if __name__ == "__main__":
    test_section_owner_capi()
