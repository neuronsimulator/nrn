"""Filesystem path semantics for HOC / nocmodl / modlunit / nrniv -dll.

Three facts (Windows native paths; Unix slash/colon stays the control):

1. ``:`` is not a list separator on Windows. ``os.pathsep`` is ``;`` there
   and ``:`` is a drive letter.
2. ``\\`` in HOC ``"..."``, C ``"..."``, and InterViews Style is an escape
   (``\\n`` newline, ``\\b`` backspace), not a path character. ``fopen`` and
   LoadLibrary accept ``/``.
3. "Last ``/``" is not the directory separator on Windows. Use both ``/``
   and ``\\`` (or ``std::filesystem``).

Add a test function for each later extract. Name it after the user-visible
job. Point the commit message at the function.

Linux CI runs the slash/colon rows. Native backslash / GUI ``-dll`` rows
are ``win32`` only.
"""

from __future__ import print_function

import os
import re
import shutil
import subprocess
import sys

import pytest

from neuron import h

WIN = sys.platform == "win32"
REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", ".."))
HH_MOD = os.path.join(REPO_ROOT, "src", "nrnoc", "hh.mod")


def _tool(name):
    exe = shutil.which(name)
    if exe is None and WIN:
        exe = shutil.which(name + ".exe")
    assert exe, "missing %s on PATH" % name
    return exe


def _run(args, cwd=None, env=None, timeout=30):
    return subprocess.run(
        args,
        cwd=cwd,
        env=env,
        timeout=timeout,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        universal_newlines=True,
    )


def _write(path, text):
    parent = os.path.dirname(path)
    if parent and not os.path.isdir(parent):
        os.makedirs(parent)
    with open(path, "w") as f:
        f.write(text)


# Child processes do not inherit add_dll_directory cookies.
_IMPORT_NEURON = (
    "import os, sys\n"
    "if sys.platform == 'win32':\n"
    "    for p in os.environ.get('PATH', '').split(os.pathsep):\n"
    "        if p and os.path.isfile(os.path.join(p, 'nrniv.dll')):\n"
    "            os.add_dll_directory(p)\n"
    "            break\n"
    "import neuron\n"
)


# ---------------------------------------------------------------------------
# fa7caed9e / 90e397c88 — HOC_LIBRARY_PATH list + load_proc missing dir
# ---------------------------------------------------------------------------


def test_load_file_hoc_library_path_os_pathsep(tmp_path):
    """load_file searches HOC_LIBRARY_PATH with os.pathsep (fa7caed9e)."""
    lib = str(tmp_path / "lib1")
    missing = str(tmp_path / "missing")
    _write(
        os.path.join(lib, "fromlib.hoc"),
        'strdef pathsem_load_file\npathsem_load_file = "fromlib-ok"\n',
    )
    os.environ["HOC_LIBRARY_PATH"] = os.pathsep.join([missing, lib])
    assert h.load_file("fromlib.hoc") == 1.0
    assert h.pathsem_load_file == "fromlib-ok"


def test_load_file_relative_xopen_from_dir(tmp_path):
    """load_file chdir's to the file's directory for relative xopen (73130ea48)."""
    d = str(tmp_path / "hoclib")
    _write(os.path.join(d, "rel.hoc"), 'strdef pathsem_rel\npathsem_rel = "rel-ok"\n')
    _write(os.path.join(d, "fromlib.hoc"), 'xopen("rel.hoc")\n')
    path = os.path.join(d, "fromlib.hoc")
    assert h.load_file(path) == 1.0
    assert h.pathsem_rel == "rel-ok"


def test_load_proc_skips_missing_dir(tmp_path):
    """load_proc does not throw on a missing HOC_LIBRARY_PATH dir (90e397c88)."""
    lib = str(tmp_path / "lib1")
    missing = str(tmp_path / "no_such_dir")
    _write(os.path.join(lib, "fromproc.hoc"), "proc pathsem_fromproc() { }\n")
    os.environ["HOC_LIBRARY_PATH"] = os.pathsep.join([missing, lib])
    h.load_proc("pathsem_fromproc")
    h.pathsem_fromproc()


def test_load_proc_missing_only_does_not_throw(tmp_path):
    """A list of only missing dirs prints Couldn't find; no directory_iterator."""
    missing = str(tmp_path / "no_such_dir")
    os.environ["HOC_LIBRARY_PATH"] = missing
    h.load_proc("pathsem_never_defined")


@pytest.mark.skipif(not WIN, reason="Windows filenames are case-insensitive")
def test_load_proc_windows_hoc_extension_case(tmp_path):
    """load_proc finds FROMFUNC.HOC; the filesystem is case-insensitive.

    directory_iterator compared extension == ".hoc" so .HOC was skipped
    (Couldn't find). fopen of that name already succeeds.
    """
    lib = str(tmp_path / "lib1")
    _write(os.path.join(lib, "FROMFUNC.HOC"), "proc pathsem_hoc_case() { }\n")
    os.environ["HOC_LIBRARY_PATH"] = lib
    h.load_proc("pathsem_hoc_case")
    h.pathsem_hoc_case()


def test_nrniv_py_file(tmp_path):
    """nrniv script.py runs the file as Python."""
    p = str(tmp_path / "script.py")
    _write(p, "print('pathsem-py-ok')\n")
    r = _run([_tool("nrniv"), "-nogui", "-nobanner", p], timeout=20)
    assert r.returncode == 0, r.stdout
    assert "pathsem-py-ok" in (r.stdout or "")


@pytest.mark.skipif(not WIN, reason="Windows filenames are case-insensitive")
def test_nrniv_windows_py_extension_case(tmp_path):
    """nrniv SCRIPT.PY runs the file as Python; the filesystem is case-insensitive.

    hoc_moreinput compared the last 3 chars to ".py" so .PY was xopen'd as
    HOC (syntax error). Unix stays case-sensitive.
    """
    p = str(tmp_path / "SCRIPT.PY")
    _write(p, "print('pathsem-py-case-ok')\n")
    r = _run([_tool("nrniv"), "-nogui", "-nobanner", p], timeout=20)
    assert r.returncode == 0, r.stdout
    assert "pathsem-py-case-ok" in (r.stdout or "")
    assert "syntax error" not in (r.stdout or "")


@pytest.mark.skipif(not WIN, reason="Windows filenames are case-insensitive")
def test_nrniv_python_windows_hoc_extension_case(tmp_path):
    """nrniv -python FROMFUNC.HOC xopen's the file; filenames are case-insensitive.

    With -python, hoc_moreinput compared the last 4 chars to ".hoc" so .HOC
    was skipped. Unix stays case-sensitive.
    """
    p = str(tmp_path / "FROMFUNC.HOC")
    _write(p, 'print "pathsem-hoc-case-ok"\n')
    r = _run(
        [_tool("nrniv"), "-python", "-nogui", "-nobanner", p],
        timeout=20,
    )
    assert r.returncode == 0, r.stdout
    assert "pathsem-hoc-case-ok" in (r.stdout or "")


# ---------------------------------------------------------------------------
# 6f92db085 — NRN_NMODL_PATH os.pathsep (import-time; subprocess)
# ---------------------------------------------------------------------------


def test_nrn_nmodl_path_os_pathsep(tmp_path):
    """NRN_NMODL_PATH splits on os.pathsep, not ':' (6f92db085)."""
    missing = str(tmp_path / "missing_mechs")
    env = os.environ.copy()
    env["NRN_NMODL_PATH"] = missing
    env["NEURON_MODULE_OPTIONS"] = "-nogui"
    r = _run([sys.executable, "-c", _IMPORT_NEURON], env=env)
    out = r.stdout or ""
    assert "not found in %s" % missing in out
    if WIN:
        # A ':' split of C:\\... would search "C" then the rest.
        assert "not found in C\n" not in out
        assert "not found in C." not in out


# ---------------------------------------------------------------------------
# nocmodl INCLUDE / -o / nmodl_filename  (44ded77b8, f802de62b, 996c6da8b,
# 1dc8133ce)
# ---------------------------------------------------------------------------


def _tiny_mod(suffix):
    return "NEURON { SUFFIX %s }\n" % suffix


def _tiny_mod_include(suffix):
    return 'NEURON { SUFFIX %s }\nINCLUDE "fromlib.inc"\n' % suffix


def test_nocmodl_include_next_to_mod(tmp_path):
    """INCLUDE next to a path .mod, run from another cwd (f802de62b)."""
    mods = str(tmp_path / "mods")
    elsewhere = str(tmp_path / "elsewhere")
    os.makedirs(elsewhere)
    _write(os.path.join(mods, "fromlib.inc"), ": pathsem include marker\n")
    _write(os.path.join(mods, "fromlib.mod"), _tiny_mod_include("pathseminc"))
    r = _run(
        [_tool("nocmodl"), os.path.join(mods, "fromlib.mod")],
        cwd=elsewhere,
    )
    assert r.returncode == 0, r.stdout
    assert "INCLUDEing" in r.stdout
    assert "fromlib.inc" in r.stdout
    assert "Couldn't open" not in r.stdout


def test_nocmodl_modl_include_os_pathsep(tmp_path):
    """MODL_INCLUDE is os.pathsep-separated (44ded77b8)."""
    inc = str(tmp_path / "inc")
    mods = str(tmp_path / "mods")
    elsewhere = str(tmp_path / "elsewhere")
    os.makedirs(elsewhere)
    _write(os.path.join(inc, "fromlib.inc"), ": pathsem modl_include marker\n")
    _write(os.path.join(mods, "fromlib.mod"), _tiny_mod_include("pathsemmi"))
    env = os.environ.copy()
    env["MODL_INCLUDE"] = inc
    r = _run(
        [_tool("nocmodl"), os.path.join(mods, "fromlib.mod")],
        cwd=elsewhere,
        env=env,
    )
    assert r.returncode == 0, r.stdout
    assert "INCLUDEing" in r.stdout
    assert "fromlib.inc" in r.stdout


def test_nocmodl_dash_o_writes_basename(tmp_path):
    """nocmodl -o uses the filename, not the whole path (996c6da8b)."""
    mods = str(tmp_path / "mods")
    out = str(tmp_path / "outdir")
    os.makedirs(out)
    _write(os.path.join(mods, "fromlib.mod"), _tiny_mod("pathsemo"))
    r = _run(
        [_tool("nocmodl"), "-o", out, os.path.join(mods, "fromlib.mod")],
    )
    assert r.returncode == 0, r.stdout
    assert os.path.isfile(os.path.join(out, "fromlib.cpp"))
    # Did not treat C:\\mods\\fromlib.mod as a relative name under outdir.
    leftover = [n for n in os.listdir(out) if n != "fromlib.cpp" and "fromlib" in n]
    assert leftover == [], leftover


def test_nocmodl_noext_dotdot_not_dotdot_cpp(tmp_path):
    """nocmodl ../mods/fromlib (no .mod) must not write ..cpp (996c6da8b)."""
    mods = str(tmp_path / "mods")
    cwd = str(tmp_path / "cwd")
    os.makedirs(cwd)
    _write(os.path.join(mods, "fromlib.mod"), _tiny_mod("pathsemnoext"))
    rel = os.path.join("..", "mods", "fromlib")
    r = _run([_tool("nocmodl"), rel], cwd=cwd)
    assert r.returncode == 0, r.stdout
    assert not os.path.isfile(os.path.join(cwd, "..cpp"))
    assert os.path.isfile(os.path.join(mods, "fromlib.cpp"))


def test_nocmodl_nmodl_filename_narrow_slash(tmp_path):
    """Generated nmodl_filename is a narrow '/' path (1dc8133ce, d091d2c40)."""
    mods = str(tmp_path / "mods")
    _write(os.path.join(mods, "fromlib.mod"), _tiny_mod("pathsemfn"))
    r = _run([_tool("nocmodl"), os.path.join(mods, "fromlib.mod")])
    assert r.returncode == 0, r.stdout
    cpp_path = os.path.join(mods, "fromlib.cpp")
    assert os.path.isfile(cpp_path)
    with open(cpp_path, "r") as f:
        cpp = f.read()
    m = re.search(r'nmodl_filename\s*=\s*"([^"]*)"', cpp)
    assert m, "nmodl_filename not in generated C"
    val = m.group(1)
    assert "fromlib.mod" in val
    # generic_string: no raw backslash (would be a C escape).
    assert "\\" not in val
    # wchar_t %s used to print only the drive letter "C".
    assert val != "C"


# ---------------------------------------------------------------------------
# 62391b5c5 — modlunit .mod is the filename extension, not a path substring
# ---------------------------------------------------------------------------


def test_modlunit_mod_named_directory(tmp_path):
    """modlunit foo.mod/hh.mod checks hh.mod, not the directory (62391b5c5)."""
    d = str(tmp_path / "foo.mod")
    os.makedirs(d)
    dest = os.path.join(d, "hh.mod")
    shutil.copy(HH_MOD, dest)
    r = _run([_tool("modlunit"), dest])
    out = r.stdout or ""
    assert r.returncode == 0, out
    assert "hh.mod" in out
    assert "Can't open" not in out
    assert "Couldn't open" not in out


# ---------------------------------------------------------------------------
# a2a8445c2 — GUI nrniv -dll  ...\\nrnmech.dll  (Style \\n)
# ---------------------------------------------------------------------------


@pytest.mark.skipif(not WIN, reason="InterViews Style \\n split is a Windows path")
def test_nrniv_dll_native_backslash_style():
    """GUI nrniv -dll C:\\...\\nrnmech.dll must not eat \\n (a2a8445c2).

    A missing DLL is enough: Style used to turn \\nrnmech into a newline
    plus 'rnmech.dll' in the dlopen message.
    """
    dll = r"C:\pathsem-missing\nrnmech.dll"
    env = os.environ.copy()
    r = _run(
        [_tool("nrniv"), "-nobanner", "-dll", dll, "-c", "quit()"],
        env=env,
        timeout=20,
    )
    out = r.stdout or ""
    # Newline inserted between the directory and rnmech.dll.
    assert "\nrnmech.dll" not in out
    assert "\rnmech.dll" not in out
    leftover = out.replace("nrnmech.dll", "")
    assert "rnmech.dll" not in leftover


@pytest.mark.skipif(not WIN, reason="native backslash -dll is a Windows path")
def test_nrniv_dll_nogui_native_backslash():
    """-nogui -dll of a native backslash path still parses the filename."""
    dll = r"C:\pathsem-missing\nrnmech.dll"
    r = _run(
        [_tool("nrniv"), "-nogui", "-nobanner", "-dll", dll, "-c", "quit()"],
        timeout=20,
    )
    out = r.stdout or ""
    leftover = out.replace("nrnmech.dll", "")
    assert "rnmech.dll" not in leftover


# ---------------------------------------------------------------------------
# Import3d GUI chooser directory — last / is not the directory on Windows
# ---------------------------------------------------------------------------

# Same egrep class as import3d_gui.hoc (and mulfit .*[/:\\] basename).
_IMPORT3D_CHOOSER_DIR_RE = "[^/\\\\]*$"


def test_import3d_gui_chooser_dir(tmp_path):
    """Import3d GUI chooser starts in the SWC directory.

    import3d_gui.hoc used [^/]*$ so a native C:\\dir\\cell.swc left the
    chooser directory empty (cwd) instead of C:\\dir\\.
    """
    gui_hoc = os.path.join(
        REPO_ROOT, "share", "lib", "hoc", "import3d", "import3d_gui.hoc"
    )
    with open(gui_hoc) as f:
        assert 'file.getname(), "[^/]*$"' not in f.read()
    morph = str(tmp_path / "morph")
    os.makedirs(morph)
    swc = os.path.join(morph, "cell.swc")
    _write(swc, "1 1 0 0 0 1 -1\n2 3 10 0 0 0.5 1\n")
    h.load_file("import3d.hoc")
    cell = h.Import3d_SWC_read()
    cell.input(swc)
    name = str(cell.file.getname())
    head = h.ref("")
    h.StringFunctions().head(name, _IMPORT3D_CHOOSER_DIR_RE, head)
    got = os.path.normpath(str(head[0]))
    want = os.path.normpath(morph)
    assert got == want, (name, head[0], got, want)


# ---------------------------------------------------------------------------
# File.getname — \ in HOC "..." is an escape (same as load_file / mktemp)
# ---------------------------------------------------------------------------


def test_file_getname_hoc_quoted_xopen(tmp_path):
    """File.getname interpolated into HOC xopen(\"...\") must keep the path.

    stdrun save_session and retrieve do sprint/execute of getname inside
    quotes. A stored C:\\build-...\\fromgetname.hoc became
    C:<BS>uild-...fromgetname.hoc (\\b backspace, other \\X dropped).
    fopen accepts /.
    """
    d = str(tmp_path / "hoclib")
    hoc = os.path.join(d, "fromgetname.hoc")
    _write(
        hoc,
        'strdef pathsem_file_getname\npathsem_file_getname = "fromgetname-ok"\n',
    )
    f = h.File(hoc)
    name = str(f.getname())
    assert "\\" not in name
    h('xopen("%s")' % name)
    assert h.pathsem_file_getname == "fromgetname-ok"


# ---------------------------------------------------------------------------
# neuronhome() — getenv NEURONHOME is a native path; ...\nrn has \n
# ---------------------------------------------------------------------------


def test_neuronhome_hoc_quoted_ropen():
    """neuronhome() interpolated into HOC File.ropen(\"...\") must keep .../nrn.

    getenv NEURONHOME is a native path (wheel os.path.abspath, cmake probe).
    A stored C:\\...\\share\\nrn became share + newline + rn (\\n). fopen
    accepts /. Same as File.getname / getcwd / setneuronhome.
    """
    nh = str(h.neuronhome())
    assert "\\" not in nh
    stdlib = nh + "/lib/hoc/stdlib.hoc"
    h("objref pathsem_nh_f_")
    h("pathsem_nh_f_ = new File()")
    h('pathsem_nh_open_ = pathsem_nh_f_.ropen("%s")' % stdlib)
    assert h.pathsem_nh_open_ == 1.0, stdlib
    h("pathsem_nh_f_.close()")
