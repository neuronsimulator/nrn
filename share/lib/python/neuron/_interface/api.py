"""ctypes bindings to libnrniv for NEURON 9+."""
import ctypes
import ctypes.util
import math
import operator
import os
from pathlib import Path

_LIBNRNIV_ENV = "MYNEURON_LIBNRNIV"
_LIBMODLREG_ENV = "MYNEURON_LIBMODLREG"

# Imported as neuron._interface, this package lives inside NEURON's own Python
# package: it must use that build's libraries (never another installed
# neuron) and reproduce the startup the legacy PyInit_hoc performed.
_VENDORED = __package__ != "myneuron"
_HERE = Path(__file__).resolve().parent
_LIBNRNIV_NAMES = ("libnrniv.so", "libnrniv.dylib", "nrniv.dll", "libnrniv.dll")


def _vendored_lib_dirs():
    # Wheel: neuron/.data/lib. Build tree and CMake install:
    # <prefix>/lib/python/neuron/_interface -> <prefix>/lib.
    return [_HERE.parent / ".data" / "lib", _HERE.parents[2]]


def _iter_existing(paths):
    for p in paths:
        if not p:
            continue
        try:
            path = Path(p)
        except TypeError:
            continue
        if path.exists():
            yield str(path)


def _neuron_pkg_path():
    """Locate the neuron package directory without executing its __init__.

    `importlib.util.find_spec("neuron")` returns the spec without
    running `neuron/__init__.py`. That matters because eager
    `import neuron`: (a) calls `nrn_init` a second time after myneuron
    has already called it, which segfaults; and (b) loads
    libnrnpython.so and populates `neuron::python::methods`, papering
    over standalone-mode bugs we'd rather see fail loudly. The spec
    lookup is the equivalent of real NEURON's `nrn_dll()` filesystem-
    walk strategy (share/lib/python/neuron/__init__.py:584) without
    needing the `hoc` extension or `n.neuronhome()` to be live.
    """
    if _VENDORED:
        return _HERE.parent
    import importlib.util

    try:
        spec = importlib.util.find_spec("neuron")
    except (ImportError, ValueError):
        return None
    if spec is None or not spec.submodule_search_locations:
        return None
    return Path(next(iter(spec.submodule_search_locations))).resolve()


def _ensure_neuronhome():
    """Set NEURONHOME from the neuron wheel layout if not already set.

    Mirrors neuron/__init__.py:135-137. libnrniv reads NEURONHOME at
    init time to find stdrun.hoc and the standard HOC library. Without
    this, `load_file("stdrun.hoc")` fails — which breaks essentially
    every model that uses `run()` or `finitialize()` via stdrun. We
    set it as a default only; an existing value (system NEURON, custom
    install) wins.
    """
    if os.environ.get("NEURONHOME") or os.environ.get("HOC_LIBRARY_PATH"):
        return
    base = _neuron_pkg_path()
    if base is None:
        return
    nrn_share = base / ".data" / "share" / "nrn"
    if nrn_share.is_dir():
        os.environ["NEURONHOME"] = str(nrn_share)


def _nrniv_candidates_from_neuron_wheel():
    # NEURON's manylinux/macOS wheels ship libnrniv inside
    # site-packages/neuron/.data/lib/ (auditwheel layout); older wheels
    # put it directly in site-packages/neuron/. Both are valid sources
    # because we still want users who only `pip install neuron` (not a
    # system NEURON build) to work out of the box.
    if _VENDORED:
        return [d / name for d in _vendored_lib_dirs() for name in _LIBNRNIV_NAMES]
    base = _neuron_pkg_path()
    candidates = []
    if base is not None:
        for libdir in (base / ".data" / "lib", base):
            for name in (
                "libnrniv.so",
                "libnrniv.dylib",
                "nrniv.dll",
                "libnrniv.dll",
            ):
                candidates.append(libdir / name)
    # Append fallback locations for the case where `find_spec("neuron")`
    # points at something other than the pip-installed wheel — e.g. when
    # `neuron` has been shimmed by an integration harness (the
    # nrn-myneuron experiment) and its package dir has no `.data/`.
    candidates.extend(_extra_libnrniv_candidates())
    return candidates


def _extra_libnrniv_candidates():
    """Fallback search paths for libnrniv when the wheel layout misses.

    Looks (in order):
    1. The wheel layout under every site-packages dir on sys.path
       (covers the case where `find_spec("neuron")` resolves to a
       shim package rather than the pip-installed wheel).
    2. `importlib.metadata.distribution('neuron')` files — the wheel
       metadata survives even when ``__init__.py`` is replaced.
    3. Common system install prefixes used by source builds.
    """
    import site
    out = []
    site_dirs = []
    try:
        site_dirs.extend(site.getsitepackages())
    except (AttributeError, OSError):
        pass
    try:
        user_sp = site.getusersitepackages()
        if isinstance(user_sp, str):
            site_dirs.append(user_sp)
    except (AttributeError, OSError):
        pass
    for sp in site_dirs:
        if not isinstance(sp, str):
            continue
        for name in ("libnrniv.so", "libnrniv.dylib", "nrniv.dll",
                     "libnrniv.dll"):
            out.append(Path(sp) / "neuron" / ".data" / "lib" / name)

    try:
        import importlib.metadata as md
        dist = md.distribution("neuron")
        for f in dist.files or []:
            if "libnrniv" in str(f):
                p = Path(str(dist.locate_file(f)))
                if p not in out:
                    out.append(p)
    except (ImportError, md.PackageNotFoundError):  # noqa: F821
        pass
    except Exception:
        pass

    for prefix in ("/usr/local", "/opt/nrn", os.path.expanduser("~/.local")):
        for name in ("libnrniv.so", "libnrniv.dylib"):
            out.append(Path(prefix) / "lib" / name)
    return out


_ensure_neuronhome()


def _load_cdll_or_raise(*, label, envvar, candidates, mode=None, loader=ctypes.CDLL):
    """Load a shared library. ``loader`` may be ``ctypes.CDLL`` (releases GIL
    around each call — fine for most NEURON C API functions) or ``ctypes.PyDLL``
    (holds GIL — required for libnrniv because HOC calls like ``nrn_load_dll``
    ultimately invoke libnrnpython's ``nrnpy_reg_mech``, which does
    ``PyDict_GetItemString`` without acquiring the GIL itself; dropping the
    GIL around the HOC call segfaults on any mod file with a ``USEION``
    declaration)."""
    tried = []
    env_path = os.environ.get(envvar)
    if env_path:
        candidates = [env_path, *candidates]

    for p in _iter_existing(candidates):
        tried.append(p)
        try:
            if mode is None:
                return loader(p)
            return loader(p, mode=mode)
        except OSError:
            continue

    # As a last resort try find_library (often returns a soname)
    libname = ctypes.util.find_library(label)
    if libname:
        tried.append(libname)
        try:
            if mode is None:
                return loader(libname)
            return loader(libname, mode=mode)
        except OSError:
            pass

    tried_msg = "\n".join(f"- {t}" for t in tried) if tried else "- (no candidates)"
    raise ImportError(
        f"Could not load {label}.\n"
        f"Set {envvar} to an absolute path to the library file.\n"
        f"Tried:\n{tried_msg}"
    )


_repo_root = Path(__file__).resolve().parents[1]
_local_lib_dir = _repo_root / "lib"
_bundled_lib_dir = Path(__file__).resolve().parent / "_native"

# Load libmodlreg with RTLD_GLOBAL so its symbols are available to other libraries.
_rtld_global = getattr(ctypes, "RTLD_GLOBAL", None)
libmodlreg = _load_cdll_or_raise(
    label="modlreg",
    envvar=_LIBMODLREG_ENV,
    candidates=[
        d / f"libnrn_modlreg_stub{suffix}"
        for d in _vendored_lib_dirs() for suffix in (".so", ".dylib")
    ] if _VENDORED else [
        *sorted(_bundled_lib_dir.glob("libmodlreg*.so")),
        *sorted(_bundled_lib_dir.glob("libmodlreg*.dylib")),
        _local_lib_dir / "libmodlreg.so",
        _local_lib_dir / "libmodlreg.dylib",
    ],
    mode=_rtld_global,
)

# Load libnrniv (NEURON core library). Use ctypes.PyDLL so the GIL is held
# around each libnrniv call. This is necessary because libnrniv functions
# (most notably nrn_hoc_call executing ``nrn_load_dll(...)``) can call back
# into libnrnpython's ``nrnpy_reg_mech``, which does ``PyDict_GetItemString``
# without acquiring the GIL itself. A CDLL load would drop the GIL around
# each call and segfault the first time a USEION mod file registers.
#
# The callback chain is documented:
#   nrnoc/init.cpp:737  register_mech → nrnpy_reg_mech_p_(mechtype)
#   nrnpython/nrnpy_nrn.cpp:3092  nrnpy_reg_mech_p_ = nrnpy_reg_mech
#   nrnpython/nrnpy_nrn.cpp:3125  nrnpy_reg_mech → PyDict_GetItemString
# So every mod file with USEION (ion_reg → register_mech) traverses this
# path. CDLL = process crash.
libnrniv = _load_cdll_or_raise(
    label="nrniv",
    envvar=_LIBNRNIV_ENV,
    candidates=_nrniv_candidates_from_neuron_wheel(),
    loader=ctypes.PyDLL,
)

_REQUIRED_NIGHTLY_API = (
    "nrn_segment_node_index",
    "nrn_object_new_wrap",
    "nrn_object_new_nothrow",
    "nrn_symbol_object_get",
    "nrn_symbol_object_set",
    "nrn_symbol_str_get",
    "nrn_symbol_str_set",
    "nrn_object_ptr_push",
    "nrn_section_parent",
    "nrn_section_trueparent",
    "nrn_section_child",
    "nrn_section_sibling",
    "nrn_setpointer_pop",
    "nrn_pp_setpointer_pop",
    "nrn_symbol_pop",
    "nrn_sectionlist_to_array",
    # The dev114 baseline includes nrn#3857's ndim-aware nrn_int_pop. This
    # adjacent public capability rejects older cores BEFORE installing callbacks;
    # a late callback gate cannot safely drain their incompatible ndim marker.
    "nrn_property_data_handle_is_valid",
    "nrn_segment_nmodlrandom_get",
    "nrn_pntproc_nmodlrandom_get",
)
_missing_nightly_api = [
    name for name in _REQUIRED_NIGHTLY_API if not hasattr(libnrniv, name)
]
if _missing_nightly_api:
    raise ImportError(
        "myneuron requires neuron-nightly with the completed NEURON C API. "
        "Install or upgrade it with "
        "`pip install --upgrade neuron-nightly`. "
        f"Loaded {libnrniv._name!r}; missing symbols: "
        f"{', '.join(_missing_nightly_api)}"
    )

# Error buffer for nothrow C API calls
_ERR_BUF_SIZE = 1024


def _check_nrn_error(ret, err_buf):
    """Raise the pending Python cause, otherwise the nothrow C API error."""
    if ret != 0:
        # Callers have already rolled back arguments restored by nothrow.
        # Public component hooks abort in core, without a return slot to pop.
        from . import _raise_pending_callback_exc
        _raise_pending_callback_exc()
        msg = err_buf.value.decode("utf-8", errors="replace").strip()
        raise RuntimeError(f"NEURON error: {msg}" if msg else "NEURON error (no message)")


#
# initialize NEURON
#
# argv reasoning:
#   "-nogui"    → default-OFF (opt-IN via MYNEURON_NOGUI=1), matching
#                 real NEURON's PyInit_hoc which never passes `-nogui`.
#                 The flag forces Graph.view_count() to 0 even on a
#                 live display — strictly worse than real NEURON on GUI
#                 machines. See docs/ARCHITECTURE.md § argv strategy
#                 for the -nogui / Graph / display interaction.
#   "-nopython" → pass for an already-populated methods table and for the
#                 default standalone CFUNCTYPE provider. The former avoids a
#                 duplicate `class2oc("PythonObject", ...)` registration; the
#                 latter lets libnrniv register its stub PythonObject before
#                 myneuron installs Python callbacks. Only the explicit
#                 `MYNEURON_USE_CFUNCTYPE=0` fallback drops this flag so
#                 libnrnpython can populate the methods table.

_methods_populated = False
try:
    _methods_sym = ctypes.c_void_p.in_dll(
        libnrniv, "_ZN6neuron6python7methodsE"
    )
    _methods_first_ptr = (ctypes.c_void_p * 1).from_address(
        ctypes.addressof(_methods_sym)
    )[0]
    _methods_populated = _methods_first_ptr is not None
except ValueError:
    pass


def _python_version_code():
    # libnrniv's nrnpy.cpp:259-262 encodes the Python version the same
    # way neuron's PyInit_hoc does (inithoc.cpp:356-360): minor < 10
    # uses MAJOR*10+MINOR, minor >= 10 uses MAJOR*100+MINOR. The encoded
    # value drives `libnrnpython<MAJOR>.<MINOR>.so` selection.
    import sys
    major, minor = sys.version_info[:2]
    return major * 100 + minor if minor >= 10 else major * 10 + minor


def _libnrnpython_present_next_to_libnrniv():
    import glob
    lib_dir = os.path.dirname(libnrniv._name or "")
    if not lib_dir:
        return False
    for pat in ("libnrnpython*.so", "libnrnpython*.dylib"):
        if glob.glob(os.path.join(lib_dir, pat)):
            return True
    return False


# --- Methods-table population strategy ----------------------------
# Three pathways exist for filling neuron::python::methods:
#
#   A. already-populated → keep existing pointers (`import neuron`
#      ran first, or libnrnpython auto-load ran in a prior process)
#   B. CFUNCTYPE path (default when methods is empty) → don't load
#      libnrnpython. Keep `-nopython`; libnrniv's `nrnpython_reg()`
#      (src/nrniv/nrnpy.cpp:253) falls through to the stub
#      `class2oc("PythonObject", stub_cons, stub_destruct, ...)`
#      registration. After `nrn_init` we look up the stub symbol,
#      set `nrnpy_pyobj_sym_`, and install Python-implemented
#      function pointers into `methods` via `ctypes.CFUNCTYPE`. No
#      libnrnpython is loaded; the HOC core calls into Python
#      directly via the C function pointers.
#   C. libnrnpython-load path (opt-out fallback via
#      `MYNEURON_USE_CFUNCTYPE=0`) → drop `-nopython`, pre-set
#      `nrn_is_python_extension`, let libnrniv's standard
#      `nrnpython_reg → load_nrnpython → nrnpython_reg_real` chain
#      populate all 29 pointers via libnrnpython's C++ code.
#
# Option B avoids loading libnrnpython entirely. After nrn_init registers its
# no-op PythonObject stub, _install_cfunctype_methods replaces that template's
# destructor with the matched registry cleanup callback below. Option A/C must
# never use ctypes payloads: their real C++ destructor requires `new Py2Nrn`.
_use_cfunctype = os.environ.get("MYNEURON_USE_CFUNCTYPE", "1") != "0"
_libnrnpython_available = _libnrnpython_present_next_to_libnrniv()

# Option C activates only when CFUNCTYPE is explicitly disabled AND
# the libnrnpython sibling exists.
_use_libnrnpython_load = (
    not _methods_populated
    and not _use_cfunctype
    and _libnrnpython_available
)

# MYNEURON_NOGUI=1 opts IN to `-nogui` (default OFF, matching real
# NEURON which never passes the flag — see argv reasoning above). The
# opt-in is what the test suite uses to preserve no-DISPLAY-friendly
# behavior for PlotShape/plotting test-order isolation.
_pass_nogui = os.environ.get("MYNEURON_NOGUI", "0") == "1"
_extra_argv = []
if _pass_nogui:
    _extra_argv.append(b"-nogui")

if _use_libnrnpython_load:
    try:
        _ipe = ctypes.c_int.in_dll(libnrniv, "nrn_is_python_extension")
        if _ipe.value == 0:
            _ipe.value = _python_version_code()
    except ValueError:
        pass
    _nopython_argv = []
else:
    # Option A or B: always pass -nopython. For A, libnrnpython's
    # functions are already in methods; we leave them alone. For B,
    # the stub PythonObject template gets registered and we install
    # CFUNCTYPE pointers after nrn_init.
    _nopython_argv = [b"-nopython"]

if _VENDORED:
    from ._startup import startup_options
    _startup_argv = [arg.encode() for arg in startup_options()]
else:
    _startup_argv = []
_argv_list = [b"NEURON"] + _startup_argv + _extra_argv + _nopython_argv
_argc = len(_argv_list)
argv = (ctypes.c_char_p * (_argc + 1))()
for _i, _s in enumerate(_argv_list):
    argv[_i] = _s
argv[_argc] = None

nrn_init = libnrniv.nrn_init
nrn_init.argtypes = [ctypes.c_int, ctypes.POINTER(ctypes.c_char_p)]
nrn_init.restype = ctypes.c_int

ret = nrn_init(_argc, argv)
if ret:
    raise RuntimeError("nrn_init failed with return code %d" % ret)


# --- Option B: CFUNCTYPE-based methods table population -----------
#
# When `_use_cfunctype` and methods is still empty (i.e. neither
# Option A nor the libnrnpython-load path ran), install Python-backed
# callbacks directly into `neuron::python::methods`. The HOC core
# calls them through C function pointers and never knows the work is
# happening in Python.
#
# Layout we depend on (verified by
# tests/regression/test_cfunctype_methods_layout.py):
#   Object.u offset           : 8     (oc/hocdec.h:173)
#   Py2Nrn.po_ offset         : 8     (nrnpython/nrnpy_p2h.cpp:27)
#   sizeof(Py2Nrn)            : 16
#   impl_ptrs.hoccommand_exec : field 11 -> byte offset 88
#                               (nrniv/nrnpy.h:25-54)
#   impl_ptrs.pysame          : field 25 -> byte offset 200
#   Object.ctemplate          : offset 16  (oc/hocdec.h:173)
#   cTemplate.sym             : offset 0   (oc/hocdec.h:146)
# Verified against NEURON 9 struct layouts. test_cfunctype_methods_layout.py
# asserts these at runtime. A NEURON major-version bump may shift them;
# update these constants and re-run the test.
_OBJECT_U_OFFSET = 8
_OBJECT_CTEMPLATE_OFFSET = 16
_SYMBOL_U_OFFSET = 16
_CTEMPLATE_DESTRUCTOR_OFFSET = 80
_PY2NRN_PO_OFFSET = 8
_PY2NRN_SIZEOF = 16
_METHODS_HOCCOMMAND_EXEC_OFFSET = 88
# hpoasgn — HOC writing a Python object's component (`$o1.x = v`). Field 15 of
# the impl_ptrs struct (nrn/src/nrniv/nrnpy.h) -> offset 15*8 = 120. The struct
# is a flat array of 8-byte fn pointers, so offset = field_index*8; cross-checked
# against hoccommand_exec=88, pysame=200, py2n_component=208 below.
_METHODS_HPOASGN_OFFSET = 120
_METHODS_PYSAME_OFFSET = 200
# py2n_component — HOC reading a Python object's component (`x = pyobj.attr`).
# A probe callback at offset 208 fires from hoc_object_component with
# nindex=0/isfunc=0 for `$o1.x`; using 216 instead leaves the live slot NULL
# and causes SIGSEGV. 208 = 0-indexed field 26, right after pysame
# (field 25 = 200), consistent with the verified field order.
_METHODS_PY2N_COMPONENT_OFFSET = 208

# HocTopContextManager's five globals. Python method calls can re-enter HOC;
# while called from a template, the nested dispatch must temporarily see the
# top-level object data and symbol table or it dereferences the outer template
# context and can segfault. Keep this exact save/set/restore sequence aligned
# with nrnpython/hoccontext.h until a public context API replaces it.
_hoc_thisobject = ctypes.c_void_p.in_dll(libnrniv, "hoc_thisobject")
_hoc_objectdata = ctypes.c_void_p.in_dll(libnrniv, "hoc_objectdata")
_hoc_symlist = ctypes.c_void_p.in_dll(libnrniv, "hoc_symlist")
_hoc_top_level_data = ctypes.c_void_p.in_dll(libnrniv, "hoc_top_level_data")
_hoc_top_level_symlist = ctypes.c_void_p.in_dll(
    libnrniv, "hoc_top_level_symlist"
)

# --- Method-return typing (int / bool vs float) ---
# A built-in class method declared to return an int or bool sets the global
# hoc_return_type_code just before returning (HocReturnType in oc/code.h:
# 0 = floating, 1 = integer, 2 = boolean). nrnpy_hoc.cpp's fcall/component reads
# it after a bound-object method call and pops int()/bool() accordingly; Object
# .method_func (object.py) mirrors that. The hint is only meaningful for
# built-in classes (ctemplate->id <= hoc_max_builtin_class_id): a user template
# method whose body invokes such a built-in would otherwise leave a stale hint,
# so the read is gated on it. Both symbols export unmangled from libnrniv
# (verified via nm -D); a public accessor would remove even this global read.
_hoc_return_type_code = ctypes.c_int.in_dll(libnrniv, "hoc_return_type_code")
_hoc_max_builtin_class_id = ctypes.c_int.in_dll(libnrniv, "hoc_max_builtin_class_id")
_CTEMPLATE_ID_OFFSET = 56  # cTemplate.id (oc/hocdec.h:156); Object.ctemplate at +16


def _object_is_builtin_class(obj_int):
    """True if a raw HOC obj*'s class is a built-in (registered before any user
    template). Only built-in class methods set hoc_return_type_code, so the type
    hint is trusted only for these (mirrors nrnpy_hoc.cpp:541). Called only when
    a hint actually fired, so the extra struct reads stay off the common path."""
    ctemplate = ctypes.cast(
        obj_int + _OBJECT_CTEMPLATE_OFFSET, ctypes.POINTER(ctypes.c_void_p)
    ).contents.value
    class_id = ctypes.cast(
        ctemplate + _CTEMPLATE_ID_OFFSET, ctypes.POINTER(ctypes.c_int)
    ).contents.value
    return class_id <= _hoc_max_builtin_class_id.value


class _Py2Nrn(ctypes.Structure):
    # Mirrors `struct Py2Nrn` in nrnpython/nrnpy_p2h.cpp:27. Layout:
    #   int type_      // 4 bytes + 4 padding
    #   PyObject* po_  // 8 bytes, total = 16
    # The native C++ destructor owns native allocations. Option B instead
    # stores this ctypes allocation in _PY2NRN_REGISTRY and installs a matched
    # template destructor that removes the registry entry.
    _fields_ = [("type_", ctypes.c_int),
                ("_pad", ctypes.c_int),
                ("po_", ctypes.c_void_p)]


# Module-level keepalive: every CFUNCTYPE callback must outlive the HOC core.
# PythonObject payloads are instead owned by the keyed registry until the
# matching stub-template destructor removes them.
_CFUNCTYPE_KEEPALIVE = []
_PY2NRN_REGISTRY = {}
_cfunctype_methods_installed = False
_public_component_hooks_installed = False
_component_error = ctypes.create_string_buffer(1024)


def _component_error_pointer(exc):
    # Core copies this message after ctypes returns; temporary bytes would die
    # too early. Even a broken exception __str__ must not escape CFUNCTYPE.
    try:
        message = str(exc).encode("utf-8", errors="replace")
        message = message[:1023].decode("utf-8", errors="ignore").encode("utf-8")
    except BaseException:
        message = b"Python component callback failed"
    _component_error.value = message or b"Python component callback failed"
    return ctypes.addressof(_component_error)

# Optional on dev114: native providers keep their own hooks and ownership.
_nrn_template_set_component_hooks = getattr(
    libnrniv, "nrn_template_set_component_hooks", None
)
if _nrn_template_set_component_hooks is not None:
    _nrn_template_set_component_hooks.argtypes = [
        ctypes.c_void_p, ctypes.c_void_p, ctypes.c_void_p,
        ctypes.c_char_p, ctypes.c_size_t,
    ]
    _nrn_template_set_component_hooks.restype = ctypes.c_bool


def _pythonobject_payload_ptr(obj):
    """Borrow the pinned Py2Nrn payload pointer; caller verifies the HOC class."""
    if not obj:
        return None
    this_ptr = ctypes.c_void_p.from_address(obj + _OBJECT_U_OFFSET).value
    # Native Py2Nrn (src/nrnpython/nrnpy_p2h.cpp:27, dev114) and our ctypes
    # payload share this pinned layout, including on providers A/C.
    # nrnpy_hoc2pyobject (:77-89) maps NULL po_ to __main__; the standalone
    # stub instead has a NULL this_pointer. Neither denotes a dead payload.
    return (ctypes.c_void_p.from_address(this_ptr + _PY2NRN_PO_OFFSET).value
            if this_ptr else None)


def _unwrap_pythonobject(obj):
    """Return the Python payload borrowed from a live PythonObject."""
    if not obj:
        return None
    if _nrn_class_name(obj) != b"PythonObject":
        raise TypeError("expected a HOC PythonObject")
    po_handle = _pythonobject_payload_ptr(obj)
    if not po_handle:
        import sys

        return sys.modules["__main__"]
    return ctypes.cast(po_handle, ctypes.py_object).value


def _owned_pythonobject_to_python(obj):
    """Consume an owned PythonObject reference with either active provider."""
    try:
        return _unwrap_pythonobject(obj)
    finally:
        if obj:
            _nrn_object_unref(obj)


def _owned_hoc_object_to_python(obj):
    # Component RHS/arguments and public object returns must share ownership
    # cleanup, especially when PythonObject conversion raises.
    from .utils import _wrap_owned_object

    return _wrap_owned_object(obj)

import collections as _collections
# Bounded rotating keepalive for strings pushed back to HOC from a Python
# component read. The pushed char** must stay valid until HOC consumes
# it (within the same expression). Mirrors the real impl's hoc_temp_charptr,
# which hands out slots from a small rotating pool; a bounded ring keeps the
# buffers alive across the immediate read without growing without bound.
_PY2N_STR_POOL = _collections.deque(maxlen=64)


def _install_cfunctype_methods():
    """Install hoccommand_exec (field 11, offset 88) and pysame (field 25,
    offset 200) into neuron::python::methods via CFUNCTYPE.

    Idempotent: exits immediately if hoccommand_exec is already set
    (Options A or C ran first). When neither ran, installs Python-backed
    C function pointers so HOC's callback table is populated without
    loading libnrnpython.
    """
    global _cfunctype_methods_installed, _public_component_hooks_installed
    public_components = _nrn_template_set_component_hooks is not None

    if not _use_cfunctype:
        return

    # If methods.hoccommand_exec is already set by another path, leave it.
    try:
        _methods_sym2 = ctypes.c_void_p.in_dll(
            libnrniv, "_ZN6neuron6python7methodsE"
        )
    except ValueError:
        return
    _methods_addr = ctypes.addressof(_methods_sym2)
    _hcx_slot = ctypes.c_void_p.from_address(
        _methods_addr + _METHODS_HOCCOMMAND_EXEC_OFFSET
    )
    if _hcx_slot.value:
        return  # already populated

    # Look up the stub PythonObject template and wire it into
    # nrnpy_pyobj_sym_ so nrn_object_new_wrap(sym, payload) works later.
    # Public `nrn_symbol` is literally `hoc_lookup` (neuronapi.cpp), so it
    # replaces the C++-mangled `_Z10hoc_lookupPKc` with no behavior change.
    # Bound locally to stay ordering-safe inside the methods-table bootstrap.
    try:
        _sym_lookup = libnrniv.nrn_symbol
        _sym_lookup.argtypes = [ctypes.c_char_p]
        _sym_lookup.restype = ctypes.c_void_p
        _sym = _sym_lookup(b"PythonObject")
    except (AttributeError, OSError):
        _sym = None
    if not _sym:
        return  # No PythonObject template — falls back to "TypeError" at push time.
    try:
        _pyobj_sym = ctypes.c_void_p.in_dll(libnrniv, "nrnpy_pyobj_sym_")
        _pyobj_sym.value = _sym
    except ValueError:
        return

    # Option B uses libnrniv's stub PythonObject template, whose destructor is
    # deliberately a no-op. Replace that one slot with a matched callback so a
    # ctypes-owned Py2Nrn payload and its Python object are released when HOC's
    # object refcount reaches zero. This patch is only made after proving the
    # methods table was empty; Option A/C retain libnrnpython's C++ destructor.
    _PYOBJECT_DESTRUCTOR_T = ctypes.CFUNCTYPE(None, ctypes.c_void_p)

    @_PYOBJECT_DESTRUCTOR_T
    def _py_pythonobject_destructor(payload):
        entry = _PY2NRN_REGISTRY.pop(payload, None)
        if entry is not None:
            entry[0].po_ = None

    _CFUNCTYPE_KEEPALIVE.append(_py_pythonobject_destructor)
    _ctemplate = ctypes.c_void_p.from_address(
        _sym + _SYMBOL_U_OFFSET
    ).value
    if not _ctemplate:
        return
    _destructor_slot = ctypes.c_void_p.from_address(
        _ctemplate + _CTEMPLATE_DESTRUCTOR_OFFSET
    )
    _destructor_slot.value = ctypes.cast(
        _py_pythonobject_destructor, ctypes.c_void_p
    ).value

    _HOCCOMMAND_EXEC_T = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p)

    @_HOCCOMMAND_EXEC_T
    def _py_hoccommand_exec(obj_ptr):
        # Called by NEURON's C core when an Object wrapping a Python
        # callable needs to fire. Read Object->u.this_pointer at
        # offset 8 (oc/hocdec.h:173 layout), cast to Py2Nrn*, recover
        # the registry-owned PyObject*, call it with no args. Return 1
        # on success so cvodeobj.cpp:511 doesn't `hoc_execerror`.
        if not obj_ptr:
            return 0
        try:
            po_handle = _pythonobject_payload_ptr(obj_ptr)
            if not po_handle:
                return 0
            # Invoke via the CPython C API directly. PyObject_CallObject
            # with NULL args means "call with empty tuple"; returns
            # NULL on exception (which we map to return 0).
            _PyCO = ctypes.pythonapi.PyObject_CallObject
            _PyCO.argtypes = [ctypes.c_void_p, ctypes.c_void_p]
            _PyCO.restype = ctypes.c_void_p
            result = _PyCO(po_handle, None)
            if result is None:
                # Python exception inside the callback.
                ctypes.pythonapi.PyErr_Clear()
                return 0
            # PyObject_CallObject returns a new reference -- DECREF it.
            ctypes.pythonapi.Py_DecRef.argtypes = [ctypes.c_void_p]
            ctypes.pythonapi.Py_DecRef.restype = None
            ctypes.pythonapi.Py_DecRef(result)
            return 1
        except BaseException as exc:
            import sys
            print(
                f"myneuron CFUNCTYPE hoccommand_exec: "
                f"{type(exc).__name__}: {exc}",
                file=sys.stderr,
            )
            return 0

    _CFUNCTYPE_KEEPALIVE.append(_py_hoccommand_exec)
    _hcx_slot.value = ctypes.cast(_py_hoccommand_exec, ctypes.c_void_p).value

    # methods.pysame: needed by cvodeobj.cpp:540's
    # extra_scatter_gather_remove. Without it, removing a callback
    # dereferences a NULL function pointer. Mirrors `pysame` in
    # nrnpython/nrnpy_p2h.cpp:69 -- ho_eq_po composition collapsed
    # to: same PythonObject template AND same PyObject* handle.
    _PYSAME_T = ctypes.CFUNCTYPE(
        ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p
    )
    _nrnpy_sym_addr = ctypes.c_void_p.in_dll(
        libnrniv, "nrnpy_pyobj_sym_"
    ).value

    @_PYSAME_T
    def _py_pysame(o1, o2):
        if not o1 or not o2:
            return 0
        try:
            ct1 = ctypes.c_void_p.from_address(
                o1 + _OBJECT_CTEMPLATE_OFFSET
            ).value
            ct2 = ctypes.c_void_p.from_address(
                o2 + _OBJECT_CTEMPLATE_OFFSET
            ).value
            if not ct1 or not ct2:
                return 0
            # cTemplate.sym is at offset 0.
            sym1 = ctypes.c_void_p.from_address(ct1).value
            sym2 = ctypes.c_void_p.from_address(ct2).value
            if sym1 != _nrnpy_sym_addr or sym2 != _nrnpy_sym_addr:
                return 0
            po1 = _pythonobject_payload_ptr(o1)
            po2 = _pythonobject_payload_ptr(o2)
            return 1 if (po1 and po1 == po2) else 0
        except BaseException:
            return 0

    _CFUNCTYPE_KEEPALIVE.append(_py_pysame)
    _pysame_slot = ctypes.c_void_p.from_address(
        _methods_addr + _METHODS_PYSAME_OFFSET
    )
    _pysame_slot.value = ctypes.cast(_py_pysame, ctypes.c_void_p).value

    # Public neuronapi stack kinds. On dev114, querying the kind of a dimension
    # marker throws. Consume known markers with nrn_int_pop, never stack_type.
    _STACK_IS_STR = 1
    _STACK_IS_VAR = 2
    _STACK_IS_NUM = 3
    _STACK_IS_OBJVAR = 4
    _STACK_IS_OBJTMP = 5
    _STACK_IS_INT = 6

    def _pop_hoc_python_value(*, discard=False, on_pop):
        stack_type = _nrn_stack_type()
        if stack_type == _STACK_IS_STR:
            value = _nrn_str_pop0()
        elif stack_type == _STACK_IS_VAR:
            value = _nrn_double_ptr_pop()
        elif stack_type == _STACK_IS_NUM:
            value = _nrn_double_pop()
        elif stack_type in (_STACK_IS_OBJVAR, _STACK_IS_OBJTMP):
            value = _object_pop_safe()
        elif stack_type == _STACK_IS_INT:
            value = _nrn_int_pop()
        else:
            raise TypeError(f"unsupported HOC argument stack type {stack_type}")
        # Advance frame accounting only after a pop succeeds, but BEFORE
        # converting an owned object or dereferencing a possibly null pointer.
        on_pop()
        if stack_type == _STACK_IS_STR:
            return None if discard else _decode_hoc_string(value)
        if stack_type == _STACK_IS_VAR:
            if not value:
                raise ReferenceError("HOC argument points to invalid storage")
            return float(value[0])
        if stack_type in (_STACK_IS_OBJVAR, _STACK_IS_OBJTMP):
            if discard:
                if value:
                    _nrn_object_unref(value)
                return None
            return _owned_hoc_object_to_python(value)
        return value

    def _push_py2n_result(result):
        """Push exactly one Python result using py2n_component semantics."""
        from .object import Object as _Object

        if isinstance(result, bool):
            _nrn_double_push(float(result))
        elif isinstance(result, (int, float)):
            _nrn_double_push(float(result))
        elif result is None:
            # nrnpy_po2ho(None) is a nil HOC Object, not numeric zero. The
            # distinction is observable in objref versus numeric contexts.
            _nrn_object_push(None)
        elif isinstance(result, (str, bytes)):
            encoded = result.encode("utf-8") if isinstance(result, str) else result
            c_buf = ctypes.c_char_p(encoded)
            _PY2N_STR_POOL.append(c_buf)
            _nrn_str_push(ctypes.pointer(c_buf))
        elif isinstance(result, _Object):
            _nrn_object_push(result._obj)
        else:
            obj_ptr = _py_pyobj_to_hoc_cfunctype(result)
            if obj_ptr is None:
                po2ho = _ensure_pyobj_to_hoc()
                obj_ptr = po2ho(result) if po2ho is not None else None
            if not obj_ptr:
                raise TypeError(
                    f"cannot return Python {type(result).__name__} to HOC"
                )
            try:
                _nrn_object_push(obj_ptr)
            finally:
                # Transfer the creator-owned reference to the HOC temp stack.
                _nrn_object_unref(obj_ptr)

    def _call_python_component(target, args):
        """Call with HOC temporarily switched to its top-level context."""
        if not _hoc_thisobject.value:
            return target(*args)
        saved_object = _hoc_thisobject.value
        saved_data = _hoc_objectdata.value
        saved_symlist = _hoc_symlist.value
        _hoc_thisobject.value = None
        _hoc_objectdata.value = _hoc_top_level_data.value
        _hoc_symlist.value = _hoc_top_level_symlist.value
        try:
            return target(*args)
        finally:
            _hoc_thisobject.value = saved_object
            _hoc_objectdata.value = saved_data
            _hoc_symlist.value = saved_symlist

    def _python_component_target(py_obj, name, obj):
        # HOC's `._` denotes the payload itself, except on the top-level
        # PythonObject proxy, where names are looked up in __main__.
        if name == "_":
            if _pythonobject_payload_ptr(obj):
                return py_obj
        return getattr(py_obj, name)

    def _check_component_dimensions(ndim):
        if ndim != 1:
            raise RuntimeError(
                f"{ndim} dimensional python objects can't be accessed from hoc "
                "with var._[i1][i2]... syntax. Must use var._[i1]._[i2]... "
                "hoc syntax."
            )

    # Public component hook (legacy methods.py2n_component at offset 208):
    # HOC reading a Python object's component, `x = pyobj.attr`. A NULL
    # legacy slot causes SIGSEGV inside hoc_object_component. Recover the
    # live Python object, getattr by the HOC Symbol name, and push attributes
    # or method results through the complete value matrix. Method arguments
    # are popped by their public stack type and reversed into source order.
    # Public hooks return an error pointer so core aborts before another HOC
    # statement executes; dispatch rolls back restored arguments before raising
    # the stashed Python exception. The dev114 fallback instead drains and
    # supplies a placeholder until dispatch returns. For indexed reads, nindex tells
    # us that a dimension marker precedes the indices; nrn_int_pop consumes it.
    _PY2N_COMPONENT_T = ctypes.CFUNCTYPE(
        ctypes.c_void_p if public_components else None,
        ctypes.c_void_p, ctypes.c_void_p, ctypes.c_int, ctypes.c_int
    )

    @_PY2N_COMPONENT_T
    def _py_py2n_component(ho, sym, nindex, isfunc):
        object_popped = False
        remaining_args = nindex
        marker_pending = bool(nindex and not isfunc)

        def argument_popped():
            nonlocal remaining_args
            remaining_args -= 1

        try:
            if not ho:
                raise TypeError("component target is not a live PythonObject")
            if marker_pending:
                ndim = _nrn_int_pop()
                marker_pending = False
            args = []
            numeric_index = False
            while remaining_args:
                if not isfunc and remaining_args == 1:
                    numeric_index = _nrn_stack_type() == _STACK_IS_NUM
                args.append(_pop_hoc_python_value(on_pop=argument_popped))
            args.reverse()
            py_obj = _unwrap_pythonobject(ho)
            raw_name = _nrn_symbol_name(sym)
            name = raw_name.decode("utf-8") if isinstance(raw_name, bytes) else raw_name
            if nindex and not isfunc:
                if ndim != nindex:
                    raise RuntimeError("PythonObject dimension marker disagrees with frame")
                _check_component_dimensions(ndim)
            # Attribute getters can re-enter HOC too, before __getitem__ or
            # a method starts. They need the same template-context boundary.
            target = _call_python_component(
                _python_component_target, (py_obj, name, ho)
            )
            if isfunc:
                result = _call_python_component(target, args)
            elif nindex:
                # Native HOC numeric subscripts truncate toward zero, not
                # Python's usual rejection of float sequence indices. Its
                # double-to-C-long cast has no portable out-of-range result;
                # reject those inputs instead of reproducing undefined behavior.
                key = int(args[0]) if numeric_index else args[0]
                if numeric_index:
                    limit = 1 << (ctypes.sizeof(ctypes.c_long) * 8 - 1)
                    if not -limit <= key < limit:
                        raise OverflowError("HOC read index is outside the C long range")
                result = _call_python_component(operator.getitem, (target, key))
            else:
                result = target

            _pop_looked_at_object()
            object_popped = True
            _push_py2n_result(result)
        except BaseException as exc:
            from . import _stash_callback_exc
            _stash_callback_exc(exc)
            if public_components:
                # Return across ctypes first; core unwinds the interrupted
                # frame. No placeholder or subsequent HOC statement executes.
                # The frame may be only partly consumed; OcJump restores it.
                return _component_error_pointer(exc)
            try:
                if marker_pending:
                    _nrn_int_pop()
                while remaining_args:
                    # Cleanup must not repeat a failing object conversion.
                    _pop_hoc_python_value(discard=True, on_pop=argument_popped)
                if not object_popped:
                    _pop_looked_at_object()
            except BaseException:
                pass
            # A failed best-effort drain must not skip the required return slot.
            try:
                _nrn_double_push(0.0)
            except BaseException:
                pass

    _CFUNCTYPE_KEEPALIVE.append(_py_py2n_component)

    # Public assignment hook (legacy methods.hpoasgn at offset 120):
    # HOC writing a Python object's component, `$o1.x = v`.
    # Reached from hoc_object_asgn when the
    # left-hand side is a PythonObject. On entry the operand stack is, top to
    # bottom: the RHS value, the PythonObject `o`, the attribute-name Symbol,
    # and nindex (0 for `.x`, 1 for a `._[i]` subscript), followed when indexed
    # by a dimension marker and the indices — pushed by
    # hoc_object_component under `isfunc & 2`. We pop them in that exact order
    # (matching the real hpoasgn in nrnpy_p2h.cpp) so the stack stays balanced,
    # then PyObject_SetAttrString on the recovered live object.
    #   * number / string / object / nil RHS -> setattr with the Python value
    #   * subscript -> numeric index truncation and Python __setitem__
    # HPOASGN_RHS_* mirror parse.hpp (NUMBER/STRING/OBJECTVAR) and hocdec.h
    # (OBJECTTMP=8) — the possible types of `type_`.
    _HPOASGN_RHS_NUMBER = 259
    _HPOASGN_RHS_STRING = 260
    _HPOASGN_RHS_OBJECTVAR = 324
    _HPOASGN_RHS_OBJECTTMP = 8
    _HPOASGN_T = (
        ctypes.CFUNCTYPE(ctypes.c_void_p, ctypes.c_void_p)
        if public_components else
        ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.c_int)
    )

    @_HPOASGN_T
    def _py_hpoasgn(o, type_=None):
        from . import _stash_callback_exc
        rhs_obj = None
        target_obj = None
        remaining_indices = 0

        def index_popped():
            nonlocal remaining_indices
            remaining_indices -= 1

        try:
            if public_components:
                type_ = {
                    _STACK_IS_NUM: _HPOASGN_RHS_NUMBER,
                    _STACK_IS_STR: _HPOASGN_RHS_STRING,
                    _STACK_IS_OBJVAR: _HPOASGN_RHS_OBJECTVAR,
                    _STACK_IS_OBJTMP: _HPOASGN_RHS_OBJECTTMP,
                }.get(_nrn_stack_type())
            # Pop all fixed entries before converting values or calling user
            # code. A conversion/setter exception must not strand metadata.
            if type_ == _HPOASGN_RHS_NUMBER:
                value = _nrn_double_pop()
            elif type_ == _HPOASGN_RHS_STRING:
                value = _nrn_str_pop0()
            elif type_ in (_HPOASGN_RHS_OBJECTVAR, _HPOASGN_RHS_OBJECTTMP):
                rhs_obj = _object_pop_safe()
                value = None  # converted after the fixed metadata is drained
            else:
                raise TypeError(
                    f"unsupported PythonObject assignment stack type {type_}"
                )

            target_obj = _object_pop_safe()
            sym = _symbol_pop()
            nindex = _nrn_int_pop()

            if nindex:
                ndim = _nrn_int_pop()
                remaining_indices = nindex
                # Native assignment expects numbers. Check before popping so
                # an object key cannot throw C++ across the callback boundary.
                numeric_index = _nrn_stack_type() == _STACK_IS_NUM
                indices = []
                while remaining_indices:
                    indices.append(_pop_hoc_python_value(on_pop=index_popped))
                if ndim != nindex:
                    raise RuntimeError("PythonObject dimension marker disagrees with frame")
                _check_component_dimensions(ndim)
                if not numeric_index:
                    raise TypeError("HOC PythonObject assignment requires a numeric index")
                # Only one dimension is accepted, so no reversal is needed.
                # Unlike reads, native writes use PyLong_FromDouble, not C long.
                key = int(indices[0])
            if target_obj != o:
                raise RuntimeError("hpoasgn target stack entry did not match")

            py_obj = _unwrap_pythonobject(target_obj)
            raw_name = _nrn_symbol_name(sym)
            name = (
                raw_name.decode("utf-8") if isinstance(raw_name, bytes)
                else raw_name
            )
            if type_ in (_HPOASGN_RHS_OBJECTVAR, _HPOASGN_RHS_OBJECTTMP):
                owned_rhs = rhs_obj
                rhs_obj = None
                value = _owned_hoc_object_to_python(owned_rhs)
            elif type_ == _HPOASGN_RHS_STRING:
                value = _decode_hoc_string(value)
            if nindex:
                target = _call_python_component(
                    _python_component_target, (py_obj, name, target_obj)
                )
                _call_python_component(operator.setitem, (target, key, value))
            else:
                _call_python_component(setattr, (py_obj, name, value))
        except BaseException as exc:
            _stash_callback_exc(exc)
            if public_components:
                return _component_error_pointer(exc)
            try:
                while remaining_indices:
                    _pop_hoc_python_value(discard=True, on_pop=index_popped)
            except BaseException:
                pass
        finally:
            if rhs_obj:
                _nrn_object_unref(rhs_obj)
            if target_obj:
                _nrn_object_unref(target_obj)

    _CFUNCTYPE_KEEPALIVE.append(_py_hpoasgn)
    if public_components:
        error = ctypes.create_string_buffer(_ERR_BUF_SIZE)
        if not _nrn_template_set_component_hooks(
            _sym, ctypes.cast(_py_py2n_component, ctypes.c_void_p),
            ctypes.cast(_py_hpoasgn, ctypes.c_void_p), error, len(error),
        ):
            raise ImportError(f"PythonObject component registration failed: {error.value!r}")
        _public_component_hooks_installed = True
    else:
        # dev114 compatibility only: newer cores never touch these slots.
        ctypes.c_void_p.from_address(
            _methods_addr + _METHODS_PY2N_COMPONENT_OFFSET
        ).value = ctypes.cast(_py_py2n_component, ctypes.c_void_p).value
        ctypes.c_void_p.from_address(
            _methods_addr + _METHODS_HPOASGN_OFFSET
        ).value = ctypes.cast(_py_hpoasgn, ctypes.c_void_p).value

    _cfunctype_methods_installed = True

    # TODO(gap-41): retire the remaining private host-context/payload layouts.
    # Component reentry currently mirrors
    # HocTopContextManager via its exported globals; a public context boundary
    # is still the version-safe upstream endgame.


def _pop_looked_at_object():
    """Pop the PythonObject that hoc_obj_look_inside_stack left on the stack.

    py2n_component is handed an object the interpreter *peeked* (did not pop),
    and is expected to consume it before pushing the component's value — the
    real impl does this with hoc_pop_defer(). The public pop returns an owned
    reference, so the discard helper immediately releases it.
    """
    try:
        _discard_object_stack_value()
    except BaseException:
        pass


_install_cfunctype_methods()


def _py_pyobj_to_hoc_cfunctype(py_callable):
    """Wrap a Python value as a HOC Object* (Option B).

    Allocate a Py2Nrn, retain the value in the payload registry, and hand both to
    `nrn_object_new_wrap` — returns a fresh HOC Object whose `u.this_pointer`
    is our Py2Nrn payload.

    WHY: Mirrors `nrnpy_pyobject_in_obj` in
    nrnpython/nrnpy_p2h.cpp:91 without loading libnrnpython.

    This allocator is valid only when Option B installed the matching ctypes
    destructor. With neuron-first Option A, return None so callers use native
    nrnpy_po2ho and its C++ `delete Py2Nrn` destructor instead.
    """
    if not _cfunctype_methods_installed:
        return None
    try:
        _pyobj_sym_value = ctypes.c_void_p.in_dll(
            libnrniv, "nrnpy_pyobj_sym_"
        ).value
    except ValueError:
        return None
    if not _pyobj_sym_value:
        return None

    py2nrn = _Py2Nrn()
    py2nrn.type_ = 1  # mirror nrnpy_pyobject_in_obj setting type_=1
    # The address of the PyObject as seen by C is id(py_callable).
    py2nrn.po_ = id(py_callable)

    # The registry owns both the ctypes payload and one strong Python reference.
    # The Option-B destructor removes this exact entry at HOC object zero-ref.
    payload = ctypes.addressof(py2nrn)
    _PY2NRN_REGISTRY[payload] = (py2nrn, py_callable)

    obj = _nrn_object_new_wrap(_pyobj_sym_value, payload)
    if not obj:
        _PY2NRN_REGISTRY.pop(payload, None)
        raise RuntimeError("failed to create HOC PythonObject wrapper")
    # nrn_object_new_wrap returns the object at refcount 0; take one ref before
    # storing it (matches nrnpy_pyobject_in_obj's hoc_obj_ref(on)). Uses the
    # public `nrn_object_ref` (bound at module level) in place of the
    # C++-mangled `_Z11hoc_obj_refP6Object`.
    try:
        _nrn_object_ref(obj)
    except BaseException:
        _PY2NRN_REGISTRY.pop(payload, None)
        raise
    return obj


# nrn_stdout_redirect installs a callback that NEURON invokes for every
# printf / hoc print. Signature: int (*)(int stream, char* msg) where
# stream is 1 for stdout, 2 for stderr.
#
# The callback reference MUST be kept alive in module scope — if Python
# GCs it, NEURON will call a dangling pointer and segfault.
_STDOUT_CB_TYPE = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_int, ctypes.c_char_p)

_nrn_stdout_redirect = libnrniv.nrn_stdout_redirect
_nrn_stdout_redirect.argtypes = [_STDOUT_CB_TYPE]
_nrn_stdout_redirect.restype = None

_stdout_callback_ref = None  # keep-alive for the active callback


def _default_stdout_sink(stream, msg):
    import sys
    try:
        text = msg.decode("utf-8", errors="replace") if msg else ""
    except Exception:
        return 0
    target = sys.stderr if stream == 2 else sys.stdout
    try:
        target.write(text)
        target.flush()
    except Exception:
        pass
    return 0


def install_stdout_redirect(sink=None):
    """Route NEURON's printf/print output through a Python callable.

    sink(stream:int, msg:str) is called for every chunk; stream is 1 for
    stdout, 2 for stderr. Defaults to writing to sys.stdout / sys.stderr.
    Passing None restores the default sink. Calling again replaces the
    previous callback.
    """
    global _stdout_callback_ref

    if sink is None:
        py_sink = _default_stdout_sink
    else:
        def py_sink(stream, msg):
            try:
                text = msg.decode("utf-8", errors="replace") if msg else ""
                sink(int(stream), text)
            except Exception:
                pass
            return 0

    _stdout_callback_ref = _STDOUT_CB_TYPE(py_sink)
    _nrn_stdout_redirect(_stdout_callback_ref)


# Install the default sink at import time so HOC output participates in
# normal Python stdout (Jupyter, captured-output testing, etc.).
install_stdout_redirect()

# --- C API bindings (ctypes signatures for libnrniv functions) ---
#
# Define aliases for nonstandard data types
Section = ctypes.c_void_p
Symlist = ctypes.c_void_p
Symbol = ctypes.c_void_p
Object = ctypes.c_void_p
SymbolTableIterator = ctypes.c_void_p
SectionListIterator = ctypes.c_void_p
nrn_Item = ctypes.c_void_p
ShapePlotInterface = ctypes.c_void_p

# Load functions from libnrniv
_nrn_section_new = libnrniv.nrn_section_new
_nrn_section_new.argtypes = [ctypes.c_char_p]
_nrn_section_new.restype = Section

_nrn_hoc_call_raw = libnrniv.nrn_hoc_call
_nrn_hoc_call_raw.argtypes = [ctypes.c_char_p]
_nrn_hoc_call_raw.restype = ctypes.c_int

# TODO(gap-61): retire this private OcJump entry when public raw-HOC calls
# restore the operand stack on failure. Its bool result is opposite the C API.
_hoc_valid_stmt = getattr(libnrniv, "_Z14hoc_valid_stmtPKcP6Object", None)
if _hoc_valid_stmt is not None:
    _hoc_valid_stmt.argtypes = [ctypes.c_char_p, ctypes.c_void_p]
    _hoc_valid_stmt.restype = ctypes.c_bool


def _nrn_hoc_call(cmd):
    if _hoc_valid_stmt is not None:
        return 0 if _hoc_valid_stmt(cmd, None) else 1
    return _nrn_hoc_call_raw(cmd)

_nrn_double_push = libnrniv.nrn_double_push
_nrn_double_push.argtypes = [ctypes.c_double]
_nrn_double_push.restype = None

_nrn_global_symbol_table = libnrniv.nrn_global_symbol_table
_nrn_global_symbol_table.argtypes = []
_nrn_global_symbol_table.restype = Symlist

_nrn_top_level_symbol_table = libnrniv.nrn_top_level_symbol_table
_nrn_top_level_symbol_table.argtypes = []
_nrn_top_level_symbol_table.restype = Symlist

# --- Symbol table iterators ---
_nrn_symbol_table_iterator_new = libnrniv.nrn_symbol_table_iterator_new
_nrn_symbol_table_iterator_new.argtypes = [Symlist]
_nrn_symbol_table_iterator_new.restype = SymbolTableIterator

_nrn_symbol_table_iterator_free = libnrniv.nrn_symbol_table_iterator_free
_nrn_symbol_table_iterator_free.argtypes = [SymbolTableIterator]
_nrn_symbol_table_iterator_free.restype = None

_nrn_symbol_table_iterator_next = libnrniv.nrn_symbol_table_iterator_next
_nrn_symbol_table_iterator_next.argtypes = [SymbolTableIterator]
_nrn_symbol_table_iterator_next.restype = Symbol

_nrn_symbol_table_iterator_done = libnrniv.nrn_symbol_table_iterator_done
_nrn_symbol_table_iterator_done.argtypes = [SymbolTableIterator]
_nrn_symbol_table_iterator_done.restype = ctypes.c_int

_nrn_sectionlist_iterator_new = libnrniv.nrn_sectionlist_iterator_new
_nrn_sectionlist_iterator_new.argtypes = [nrn_Item]
_nrn_sectionlist_iterator_new.restype = SectionListIterator

_nrn_sectionlist_iterator_free = libnrniv.nrn_sectionlist_iterator_free
_nrn_sectionlist_iterator_free.argtypes = [SectionListIterator]
_nrn_sectionlist_iterator_free.restype = None

_nrn_sectionlist_iterator_next = libnrniv.nrn_sectionlist_iterator_next
_nrn_sectionlist_iterator_next.argtypes = [SectionListIterator]
_nrn_sectionlist_iterator_next.restype = Section

_nrn_sectionlist_iterator_done = libnrniv.nrn_sectionlist_iterator_done
_nrn_sectionlist_iterator_done.argtypes = [SectionListIterator]
_nrn_sectionlist_iterator_done.restype = ctypes.c_int

# --- Symbol metadata ---
_nrn_symbol = libnrniv.nrn_symbol
_nrn_symbol.argtypes = [ctypes.c_char_p]
_nrn_symbol.restype = Symbol

_nrn_symbol_type = libnrniv.nrn_symbol_type
_nrn_symbol_type.argtypes = [Symbol]
_nrn_symbol_type.restype = ctypes.c_int

_nrn_symbol_subtype = libnrniv.nrn_symbol_subtype
_nrn_symbol_subtype.argtypes = [Symbol]
_nrn_symbol_subtype.restype = ctypes.c_int

_nrn_symbol_name = libnrniv.nrn_symbol_name
_nrn_symbol_name.argtypes = [Symbol]
_nrn_symbol_name.restype = ctypes.c_char_p

_nrn_register_function = libnrniv.nrn_register_function
_nrn_register_function.argtypes = [ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int]
_nrn_register_function.restype = None

_nrn_gargstr = libnrniv.nrn_gargstr
_nrn_gargstr.argtypes = [ctypes.c_int]
_nrn_gargstr.restype = ctypes.c_char_p

# --- Stack operations (push / pop / HOC call) ---
# Read a double argument from the HOC operand stack inside a
# C-registered function callback. Returned pointer is owned by NEURON.
_nrn_getarg = libnrniv.nrn_getarg
_nrn_getarg.argtypes = [ctypes.c_int]
_nrn_getarg.restype = ctypes.POINTER(ctypes.c_double)

_nrn_hoc_ret = libnrniv.nrn_hoc_ret
_nrn_hoc_ret.argtypes = []
_nrn_hoc_ret.restype = None

_nrn_function_call = libnrniv.nrn_function_call_nothrow
_nrn_function_call.argtypes = [Symbol, ctypes.c_int, ctypes.c_char_p, ctypes.c_size_t]
_nrn_function_call.restype = ctypes.c_int

_nrn_double_pop = libnrniv.nrn_double_pop
_nrn_double_pop.argtypes = []
_nrn_double_pop.restype = ctypes.c_double

# --- Object API ---
_nrn_object_new_nothrow = libnrniv.nrn_object_new_nothrow
_nrn_object_new_nothrow.argtypes = [
    Symbol,
    ctypes.c_int,
    ctypes.POINTER(Object),
    ctypes.c_char_p,
    ctypes.c_size_t,
]
_nrn_object_new_nothrow.restype = ctypes.c_int


def _nrn_object_new(sym, narg):
    """Construct a HOC object without allowing C++ exceptions across ctypes."""
    result = Object()
    err = ctypes.create_string_buffer(_ERR_BUF_SIZE)
    rc = _nrn_object_new_nothrow(
        sym, narg, ctypes.byref(result), err, _ERR_BUF_SIZE
    )
    if rc:
        msg = err.value.decode("utf-8", errors="replace")
        raise RuntimeError(msg or "HOC object construction failed")
    return result.value


_nrn_object_new_wrap = libnrniv.nrn_object_new_wrap
_nrn_object_new_wrap.argtypes = [Symbol, ctypes.c_void_p]
_nrn_object_new_wrap.restype = Object

_nrn_object_ref = libnrniv.nrn_object_ref
_nrn_object_ref.argtypes = [Object]
_nrn_object_ref.restype = None

_nrn_object_unref = libnrniv.nrn_object_unref
_nrn_object_unref.argtypes = [Object]
_nrn_object_unref.restype = None

_nrn_symbol_table = libnrniv.nrn_symbol_table
_nrn_symbol_table.argtypes = [Symbol]
_nrn_symbol_table.restype = Symlist

_nrn_class_name = libnrniv.nrn_class_name
_nrn_class_name.argtypes = [Object]
_nrn_class_name.restype = ctypes.c_char_p

_nrn_method_symbol = libnrniv.nrn_method_symbol
_nrn_method_symbol.argtypes = [Object, ctypes.c_char_p]
_nrn_method_symbol.restype = Symbol
_nrn_symbol_dataptr = libnrniv.nrn_symbol_dataptr
_nrn_symbol_dataptr.argtypes = [Symbol]
_nrn_symbol_dataptr.restype = ctypes.POINTER(ctypes.c_double)

# Smallest address treated as a real data pointer from nrn_symbol_dataptr.
# For a NOTUSER runtime scalar (`n('x = 42')`) the correct storage is
# hoc_top_level_data[sym->u.oboff].pval. A libnrniv carrying the upstream fix
# (nrn#3815) returns that heap/data-segment address; older builds return the
# raw sym->u.oboff — a small integer (symbol-table index, always well under
# this threshold) reinterpreted as a pointer, whose dereference would segfault.
# The __init__.py subtype-0 paths guard on this to pick the direct-deref fast
# path vs. the hoc_ac_ trampoline / HOC-assign fallback, without ever
# dereferencing a bogus pointer.
_MIN_VALID_DATAPTR = 0x10000

_nrn_method_call = libnrniv.nrn_method_call_nothrow
_nrn_method_call.argtypes = [Object, Symbol, ctypes.c_int, ctypes.c_char_p, ctypes.c_size_t]
_nrn_method_call.restype = ctypes.c_int

_nrn_vector_data = libnrniv.nrn_vector_data
_nrn_vector_data.argtypes = [Object]
_nrn_vector_data.restype = ctypes.POINTER(ctypes.c_double)

_nrn_section_pop = libnrniv.nrn_section_pop
_nrn_section_pop.argtypes = []
_nrn_section_pop.restype = None

# nrn_str_pop() returns char** owned by NEURON; _nrn_str_pop() decodes it to str.
_nrn_str_pop0 = libnrniv.nrn_str_pop
_nrn_str_pop0.argtypes = []
_nrn_str_pop0.restype = ctypes.POINTER(ctypes.c_char_p)

def _decode_hoc_string(p):
    """Decode after frame accounting; invalid UTF-8 can raise after a raw pop."""
    if not p:
        return ''
    s = p.contents.value
    if s is None:
        return ''
    return s.decode("utf-8")


def _nrn_str_pop():
    """Return a Python string from _nrn_str_pop0() result."""
    return _decode_hoc_string(_nrn_str_pop0())

_nrn_object_pop = libnrniv.nrn_object_pop
_nrn_object_pop.argtypes = []
_nrn_object_pop.restype = Object

_nrn_stack_type = libnrniv.nrn_stack_type
_nrn_stack_type.argtypes = []
_nrn_stack_type.restype = ctypes.c_int

_nrn_symbol_pop = libnrniv.nrn_symbol_pop
_nrn_symbol_pop.argtypes = []
_nrn_symbol_pop.restype = Symbol


def _object_pop_safe():
    """Pop an object value, returning an owned Object* or None for nil."""
    return _nrn_object_pop()


def _discard_object_stack_value():
    """Consume one object stack entry without retaining its object."""
    obj = _object_pop_safe()
    if obj:
        _nrn_object_unref(obj)

_nrn_int_pop = libnrniv.nrn_int_pop
_nrn_int_pop.argtypes = []
_nrn_int_pop.restype = ctypes.c_int

def _symbol_pop():
    return _nrn_symbol_pop()

_nrn_str_push = libnrniv.nrn_str_push
_nrn_str_push.argtypes = [ctypes.POINTER(ctypes.c_char_p)]
_nrn_str_push.restype = None

_nrn_object_push = libnrniv.nrn_object_push
_nrn_object_push.argtypes = [Object]
_nrn_object_push.restype = None

_nrn_object_ptr_push = libnrniv.nrn_object_ptr_push
_nrn_object_ptr_push.argtypes = [ctypes.POINTER(Object)]
_nrn_object_ptr_push.restype = None


def _object_ptr_push(obj_ref):
    _nrn_object_ptr_push(obj_ref)


# --- Python callable -> HOC PythonObject (lazy) ----------------------------
# Real NEURON converts a Python callable to a HOC `Object*` of class
# "PythonObject" via `nrnpy_po2ho` (src/nrnpython/nrnpy_hoc.cpp:598).
# That function lives in libnrnpython.cpython-3.X.so (mangled symbol
# `_Z11nrnpy_po2hoP7_object`), not libnrniv. It's only safe to call once
# `nrnpython_reg_real` has run inside libnrnpython — that registration
# installs the "PythonObject" template and populates
# `neuron::python::methods` so `hoccommand_exec` can later call back into
# the wrapped callable. We rely on neuron's own `import neuron` to do
# that registration (the bootstrap-order rule in CLAUDE.md); in pure
# standalone (no neuron loaded), the symbol still exists but the
# template isn't registered, so the path raises a clear TypeError.

_pyobj_to_hoc_object = None  # populated lazily by _ensure_pyobj_to_hoc()


def _ensure_pyobj_to_hoc():
    """Return a ctypes-bound ``nrnpy_po2ho(PyObject*) -> Object*`` or None.

    Returns None if libnrnpython can't be located or loaded.

    Side effect: if nrnpy_pyobj_sym_ is unset, looks up the stub
    PythonObject symbol and sets it — required when -nopython ran
    without libnrnpython loading (Option B / standalone mode).
    """
    global _pyobj_to_hoc_object
    # TODO(gap-60): discover the native allocator in static Python extensions.
    if _pyobj_to_hoc_object is not None:
        return _pyobj_to_hoc_object
    import glob
    import os
    # Search next to libnrniv first (covers the neuron wheel layout).
    lib_dir = os.path.dirname(libnrniv._name or "")
    patterns = [
        os.path.join(lib_dir, "libnrnpython*.so"),
        os.path.join(lib_dir, "libnrnpython*.dylib"),
    ]
    candidate = None
    for pat in patterns:
        matches = sorted(glob.glob(pat))
        if matches:
            candidate = matches[0]
            break
    if candidate is None:
        return None
    try:
        # RTLD_GLOBAL so libnrnpython sees libnrniv's already-loaded
        # symbols (hoc_obj_ref, nrnpy_pyobj_sym_, etc.) without re-dlopen.
        lib = ctypes.PyDLL(candidate, mode=ctypes.RTLD_GLOBAL)
    except OSError:
        return None

    # Ensure libnrnpython's "PythonObject" symbol pointer is set.
    # `nrnpy_pyobj_sym_` (a libnrniv-owned symbol) is normally set by
    # libnrnpython's `nrnpython_reg_real`. When myneuron starts with
    # `-nopython`, libnrniv falls through to a stub `class2oc` for
    # PythonObject but never sets `nrnpy_pyobj_sym_`. Re-calling
    # `nrnpython_reg_real` here would hit
    # "PythonObject already being used as a name" because the stub
    # registration owns the symbol. Instead, do the one assignment
    # that matters: look up the already-registered PythonObject and
    # store it in `nrnpy_pyobj_sym_` ourselves. This lets
    # `nrnpy_pyobject_in_obj` allocate Object* of class PythonObject
    # (the stub p_cons isn't called from this path — only HOC's
    # `new PythonObject()` would trigger it).
    try:
        pyobj_sym = ctypes.c_void_p.in_dll(libnrniv, "nrnpy_pyobj_sym_")
    except ValueError:
        pyobj_sym = None
    if pyobj_sym is not None and not pyobj_sym.value:
        # Public `nrn_symbol` == `hoc_lookup` (neuronapi.cpp); bound locally to
        # stay ordering-safe here in the bootstrap path.
        try:
            _sym_lookup = libnrniv.nrn_symbol
            _sym_lookup.argtypes = [ctypes.c_char_p]
            _sym_lookup.restype = ctypes.c_void_p
            sym = _sym_lookup(b"PythonObject")
        except (AttributeError, OSError):
            sym = None
        if sym:
            pyobj_sym.value = sym

    sym_name = "_Z11nrnpy_po2hoP7_object"  # nrnpy_po2ho(PyObject*)
    try:
        fn = getattr(lib, sym_name)
    except AttributeError:
        return None
    fn.argtypes = [ctypes.py_object]
    fn.restype = Object
    _pyobj_to_hoc_object = fn
    return fn


_nrn_double_ptr_push = libnrniv.nrn_double_ptr_push
_nrn_double_ptr_push.argtypes = [ctypes.POINTER(ctypes.c_double)]
_nrn_double_ptr_push.restype = None

_nrn_allsec = libnrniv.nrn_allsec
_nrn_allsec.argtypes = []
_nrn_allsec.restype = nrn_Item

_nrn_sectionlist_data = libnrniv.nrn_sectionlist_data
_nrn_sectionlist_data.argtypes = [Object]
_nrn_sectionlist_data.restype = nrn_Item

# Batched section-list gather (nrn#3834). Call with buf=NULL, maxlen=0 to
# obtain the total live count, then again with a caller-owned buffer.
_nrn_sectionlist_to_array = libnrniv.nrn_sectionlist_to_array
_nrn_sectionlist_to_array.argtypes = [
    nrn_Item,
    ctypes.POINTER(ctypes.c_void_p),
    ctypes.c_int,
]
_nrn_sectionlist_to_array.restype = ctypes.c_int

# --- Section API ---
_nrn_section_push = libnrniv.nrn_section_push
_nrn_section_push.argtypes = [Section]
_nrn_section_push.restype = None

_nrn_mechanism_insert = libnrniv.nrn_mechanism_insert
_nrn_mechanism_insert.argtypes = [Section, Symbol]
_nrn_mechanism_insert.restype = None

_nrn_rangevar_get = libnrniv.nrn_rangevar_get
_nrn_rangevar_get.argtypes = [Symbol, Section, ctypes.c_double]
_nrn_rangevar_get.restype = ctypes.c_double

_nrn_section_connect = libnrniv.nrn_section_connect
_nrn_section_connect.argtypes = [Section, ctypes.c_double, Section, ctypes.c_double]
_nrn_section_connect.restype = None

_nrn_section_length_set = libnrniv.nrn_section_length_set
_nrn_section_length_set.argtypes = [Section, ctypes.c_double]
_nrn_section_length_set.restype = None

_nrn_section_length_get = libnrniv.nrn_section_length_get
_nrn_section_length_get.argtypes = [Section]
_nrn_section_length_get.restype = ctypes.c_double

_nrn_secname = libnrniv.nrn_secname
_nrn_secname.argtypes = [Section]
_nrn_secname.restype = ctypes.c_char_p

_nrn_nseg_get = libnrniv.nrn_nseg_get
_nrn_nseg_get.argtypes = [Section]
_nrn_nseg_get.restype = ctypes.c_int

_nrn_nseg_set = libnrniv.nrn_nseg_set
_nrn_nseg_set.argtypes = [Section, ctypes.c_int]
_nrn_nseg_set.restype = None

_nrn_section_diam_set = libnrniv.nrn_segment_diam_set
_nrn_section_diam_set.argtypes = [Section, ctypes.c_double, ctypes.c_double]
_nrn_section_diam_set.restype = None

_nrn_section_diam_get = libnrniv.nrn_segment_diam_get
_nrn_section_diam_get.argtypes = [Section, ctypes.c_double]
_nrn_section_diam_get.restype = ctypes.c_double

_nrn_segment_node_index = libnrniv.nrn_segment_node_index
_nrn_segment_node_index.argtypes = [Section, ctypes.c_double]
_nrn_segment_node_index.restype = ctypes.c_int

# Section-tree accessors (nrn#3835). Each returns a Section* or NULL.
_nrn_section_parent = libnrniv.nrn_section_parent
_nrn_section_parent.argtypes = [Section]
_nrn_section_parent.restype = Section

_nrn_section_trueparent = libnrniv.nrn_section_trueparent
_nrn_section_trueparent.argtypes = [Section]
_nrn_section_trueparent.restype = Section

_nrn_section_child = libnrniv.nrn_section_child
_nrn_section_child.argtypes = [Section]
_nrn_section_child.restype = Section

_nrn_section_sibling = libnrniv.nrn_section_sibling
_nrn_section_sibling.argtypes = [Section]
_nrn_section_sibling.restype = Section

_nrn_property_get = libnrniv.nrn_property_get
_nrn_property_get.argtypes = [Object, ctypes.c_char_p]
_nrn_property_get.restype = ctypes.c_double

_nrn_property_array_get = libnrniv.nrn_property_array_get
_nrn_property_array_get.argtypes = [Object, ctypes.c_char_p, ctypes.c_int]
_nrn_property_array_get.restype = ctypes.c_double

_nrn_property_set = libnrniv.nrn_property_set
_nrn_property_set.argtypes = [Object, ctypes.c_char_p, ctypes.c_double]
_nrn_property_set.restype = None

_nrn_property_array_set = libnrniv.nrn_property_array_set
_nrn_property_array_set.argtypes = [
    Object,
    ctypes.c_char_p,
    ctypes.c_int,
    ctypes.c_double,
]
_nrn_property_array_set.restype = None

_nrn_property_data_handle_is_valid = libnrniv.nrn_property_data_handle_is_valid
_nrn_property_data_handle_is_valid.argtypes = [Object, ctypes.c_char_p, ctypes.c_int]
_nrn_property_data_handle_is_valid.restype = ctypes.c_bool


def _property_get_checked(obj, name):
    value = _nrn_property_get(obj, name)
    # Invalid point-process handles and valid NaN values share the C sentinel.
    # Probe validity only for NaN so ordinary property reads need one crossing.
    if math.isnan(value) and not _nrn_property_data_handle_is_valid(obj, name, 0):
        raise ValueError("Invalid data_handle")
    return value


def _property_array_get_checked(obj, name, index):
    value = _nrn_property_array_get(obj, name, index)
    if math.isnan(value) and not _nrn_property_data_handle_is_valid(obj, name, index):
        raise ValueError("Invalid data_handle")
    return value


def _property_set_checked(obj, name, value):
    # Writes have no NaN sentinel to gate on, so the validity probe runs on
    # every write. Real NEURON raises AttributeError("POINTER is NULL") for
    # an attribute write through an unset/opaque POINTER; a silent write-
    # through corrupts the invalid handle's target.
    if not _nrn_property_data_handle_is_valid(obj, name, 0):
        raise AttributeError("POINTER is NULL")
    _nrn_property_set(obj, name, value)


def _property_array_set_checked(obj, name, index, value):
    if not _nrn_property_data_handle_is_valid(obj, name, index):
        raise AttributeError("POINTER is NULL")
    _nrn_property_array_set(obj, name, index, value)


def _ref_property_set_checked(obj, name, value):
    # The h.ref()/._ref_ path mirrors real's ValueError instead of the
    # attribute path's AttributeError.
    if not _nrn_property_data_handle_is_valid(obj, name, 0):
        raise ValueError("Invalid data_handle")
    _nrn_property_set(obj, name, value)


def _ref_property_array_set_checked(obj, name, index, value):
    if not _nrn_property_data_handle_is_valid(obj, name, index):
        raise ValueError("Invalid data_handle")
    _nrn_property_array_set(obj, name, index, value)


def _property_push_checked(obj, name):
    # Pushing an invalid handle hands downstream consumers (Vector.record)
    # a pointer that SIGSEGVs at finitialize; refuse it like real refuses
    # the record argument.
    if not _nrn_property_data_handle_is_valid(obj, name, 0):
        raise ValueError("Invalid data_handle")
    _nrn_property_push(obj, name)


def _property_array_push_checked(obj, name, index):
    if not _nrn_property_data_handle_is_valid(obj, name, index):
        raise ValueError("Invalid data_handle")
    _nrn_property_array_push(obj, name, index)


_nrn_section_is_active = libnrniv.nrn_section_is_active
_nrn_section_is_active.argtypes = [Section]
_nrn_section_is_active.restype = ctypes.c_bool

_nrn_rangevar_set = libnrniv.nrn_rangevar_set
_nrn_rangevar_set.argtypes = [Symbol, Section, ctypes.c_double, ctypes.c_double]
_nrn_rangevar_set.restype = None

# Both return one owned Object* reference, or NULL for a non-live RANDOM member.
_nrn_segment_nmodlrandom_get = libnrniv.nrn_segment_nmodlrandom_get
_nrn_segment_nmodlrandom_get.argtypes = [Section, ctypes.c_double, Symbol]
_nrn_segment_nmodlrandom_get.restype = Object

_nrn_pntproc_nmodlrandom_get = libnrniv.nrn_pntproc_nmodlrandom_get
_nrn_pntproc_nmodlrandom_get.argtypes = [Object, Symbol]
_nrn_pntproc_nmodlrandom_get.restype = Object

# PlotShape bindings: used by plotting._PlotShapePlot._get_plot_data().
# We can't use upstream NEURON's neuron.gui2.utilities.get_plotshape_data —
# it expects a Python-wrapped HOC object (PyObject capsule), but myneuron
# stores raw uint64 C pointers, so the capsule conversion misinterprets them.
_nrn_get_plotshape_interface = libnrniv.nrn_get_plotshape_interface
_nrn_get_plotshape_interface.argtypes = [Object]
_nrn_get_plotshape_interface.restype = ShapePlotInterface

_nrn_symbol_is_array = libnrniv.nrn_symbol_is_array
_nrn_symbol_is_array.argtypes = [Symbol]
_nrn_symbol_is_array.restype = ctypes.c_bool

_nrn_get_plotshape_section_list = libnrniv.nrn_get_plotshape_section_list
_nrn_get_plotshape_section_list.argtypes = [ShapePlotInterface]
_nrn_get_plotshape_section_list.restype = Object

_nrn_get_plotshape_varname = libnrniv.nrn_get_plotshape_varname
_nrn_get_plotshape_varname.argtypes = [ShapePlotInterface]
_nrn_get_plotshape_varname.restype = ctypes.c_char_p

_nrn_get_plotshape_low = libnrniv.nrn_get_plotshape_low
_nrn_get_plotshape_low.argtypes = [ShapePlotInterface]
_nrn_get_plotshape_low.restype = ctypes.c_float

_nrn_get_plotshape_high = libnrniv.nrn_get_plotshape_high
_nrn_get_plotshape_high.argtypes = [ShapePlotInterface]
_nrn_get_plotshape_high.restype = ctypes.c_float

_nrn_symbol_array_length = libnrniv.nrn_symbol_array_length
_nrn_symbol_array_length.argtypes = [Symbol]
_nrn_symbol_array_length.restype = ctypes.c_int

# --- Ra, rallbranch, ref/unref, distance ---
_nrn_section_Ra_get = libnrniv.nrn_section_Ra_get
_nrn_section_Ra_get.argtypes = [Section]
_nrn_section_Ra_get.restype = ctypes.c_double

_nrn_section_Ra_set = libnrniv.nrn_section_Ra_set
_nrn_section_Ra_set.argtypes = [Section, ctypes.c_double]
_nrn_section_Ra_set.restype = None

_nrn_section_rallbranch_get = libnrniv.nrn_section_rallbranch_get
_nrn_section_rallbranch_get.argtypes = [Section]
_nrn_section_rallbranch_get.restype = ctypes.c_double

_nrn_section_rallbranch_set = libnrniv.nrn_section_rallbranch_set
_nrn_section_rallbranch_set.argtypes = [Section, ctypes.c_double]
_nrn_section_rallbranch_set.restype = None

_nrn_rangevar_push = libnrniv.nrn_rangevar_push
_nrn_rangevar_push.argtypes = [Symbol, Section, ctypes.c_double]
_nrn_rangevar_push.restype = None

_nrn_setpointer_pop = libnrniv.nrn_setpointer_pop
_nrn_setpointer_pop.argtypes = [
    Symbol,
    Section,
    ctypes.c_double,
    ctypes.c_char_p,
    ctypes.c_size_t,
]
_nrn_setpointer_pop.restype = ctypes.c_int

_nrn_pp_setpointer_pop = libnrniv.nrn_pp_setpointer_pop
_nrn_pp_setpointer_pop.argtypes = [
    Object,
    ctypes.c_char_p,
    ctypes.c_char_p,
    ctypes.c_size_t,
]
_nrn_pp_setpointer_pop.restype = ctypes.c_int

_nrn_property_push = libnrniv.nrn_property_push
_nrn_property_push.argtypes = [Object, ctypes.c_char_p]
_nrn_property_push.restype = None

_nrn_property_array_push = libnrniv.nrn_property_array_push
_nrn_property_array_push.argtypes = [Object, ctypes.c_char_p, ctypes.c_int]
_nrn_property_array_push.restype = None

_nrn_double_ptr_pop = libnrniv.nrn_double_ptr_pop
_nrn_double_ptr_pop.argtypes = []
_nrn_double_ptr_pop.restype = ctypes.POINTER(ctypes.c_double)

# Reserved: nrn_symbol_push is bound but not currently used in myneuron.
_nrn_symbol_push = libnrniv.nrn_symbol_push
_nrn_symbol_push.argtypes = [Symbol]
_nrn_symbol_push.restype = None

_nrn_section_ref = libnrniv.nrn_section_ref
_nrn_section_ref.argtypes = [Section]
_nrn_section_ref.restype = None

_nrn_section_unref = libnrniv.nrn_section_unref
_nrn_section_unref.argtypes = [Section]
_nrn_section_unref.restype = None

_nrn_cas = libnrniv.nrn_cas
_nrn_cas.argtypes = []
_nrn_cas.restype = Section

# Reserved: nrn_prop_exists (check whether an Object has a given property)
# is bound but not currently used in myneuron.
_nrn_prop_exists = libnrniv.nrn_prop_exists
_nrn_prop_exists.argtypes = [Object]
_nrn_prop_exists.restype = ctypes.c_bool

_nrn_distance = libnrniv.nrn_distance
_nrn_distance.argtypes = [Section, ctypes.c_double, Section, ctypes.c_double]
_nrn_distance.restype = ctypes.c_double


# ---------------------------------------------------------------------------
# C-side staleness counters
#
# NEURON's internal globals that flip when topology / geometry changes:
#   diam_changed         (cabcode.cpp:55)        — dirty flag, set on
#                                                  nseg/diam/L mutation,
#                                                  cleared at finitialize.
#   structure_change_cnt (treeset.cpp:66)        — bumped when NEURON
#                                                  recomputes structure
#                                                  (typically at finitialize).
#   nrn_shape_changed_   (treeset.cpp:37)        — bumped when geometry
#                                                  (3D points) changes.
#
# The pair (diam_changed, structure_change_cnt) is a reliable staleness
# signal for cached pointers: if it changes between cache-time and
# read-time, the pointer may be stale. Catches changes from ANY source
# (HOC, real NEURON's Python extension, models that call nrn_section_new
# directly), not just myneuron's setters.
_diam_changed = ctypes.c_int.in_dll(libnrniv, "diam_changed")
_structure_change_cnt = ctypes.c_int.in_dll(libnrniv, "structure_change_cnt")
_nrn_shape_changed = ctypes.c_int.in_dll(libnrniv, "nrn_shape_changed_")

# Dated context: benchmarks/README.md#historical-implementation-measurements.
# WHY ctypes.c_int.value, not numpy.frombuffer: the per-yield staleness
# check in allsec() was benchmarked at 5M reads: ctypes .value ~94 ns/read
# vs numpy [0] ~106 ns/read.
# numpy is ~12% slower here and would add a hard numpy dependency
# to the hot path. Keeping ctypes.


def get_topology_version():
    """Return an opaque tuple that changes when topology/diam changes.

    Suitable for staleness checks on cached range-variable pointers.
    Compare two return values with `==` / `!=`; do not interpret the
    components.
    """
    return (_diam_changed.value, _structure_change_cnt.value)


# ---------------------------------------------------------------------------
# Type code discovery
#
# parse.ypp's token enum values (VAR=263, FUNCTION=270, ...) are an
# implementation detail of NEURON's parser. neuronapi.cpp:236 notes:
# "types are in parse.hpp and are not the same between versions, so we
# really should wrap." If a future NEURON renumbers, all hardcoded
# `if sym_type == 263` checks would silently break.
#
# Instead, we probe known-stable symbols at import time and let NEURON
# tell us the codes:
#   t           → VAR             (global double)
#   sin         → BLTIN           (math built-in)
#   finitialize → FUN_BLTIN       (function returning double)
#   secname     → STRINGFUNC      (function returning string)
#   hh          → MECHANISM       (density mechanism)
#   IClamp      → TEMPLATE        (HOC class)
#   v           → RANGEVAR        (range variable on segment)
#
# Template-scoped codes (SECTION, OBFUNCTION) are discovered after
# NEURON is initialized by spinning up a probe template.


class TypeCodes:
    """Type codes discovered at runtime by probing known symbols.

    Replaces hardcoded parse.ypp constants throughout the dispatch chain
    so myneuron survives if NEURON renumbers its token enum.

    Use ``TYPES.VAR`` (etc.) in `if sym_type == TYPES.VAR` checks.
    """

    __slots__ = (
        # Top-level codes
        "STRING",       # top-level strdef (260) — also object STRDEF members
        "VAR",          # global double (t, dt, celsius); also USERPROPERTY (L, nseg)
        "BLTIN",        # math built-in (sin, cos, exp)
        "FUNCTION",     # HOC func/method returning double (270); distinct from FUN_BLTIN (280)
        "FUN_BLTIN",    # C-registered function returning double (finitialize, x3d)
        "PROCEDURE",    # HOC PROC
        "STRINGFUNC",   # function returning string (secname)
        "OBJECTFUNC",   # built-in function returning object (object_pushed)
        "OBJECTVAR",    # top-level objref (324) — `objref foo`
        "MECHANISM",    # density mechanism (hh, pas)
        "TEMPLATE",     # HOC class (IClamp, Vector)
        "RANGEVAR",     # range variable on segment (v, gnabar_hh)
        "RANGEOBJ",     # NMODL RANDOM range object (runtime-probed after MOD load)
        # Template / method codes
        "SECTION",      # template public section (cell.soma)
        "OBFUNCTION",   # template method returning object (HOC obfunc)
        "METHOD_OBFUNC",    # C-side method returning object (Vector.c, List.object)
        "METHOD_STRFUNC",   # method returning string (Vector.label)
        "METHOD_SECTIONREF",  # method returning Section (SectionRef.sec/parent/root)
        # Subtypes of VAR (hocdec.h:83-90 #defines, probed via known symbols)
        "USERINT",      # int scalar VAR (stoprun, hoc_ac_)
        "USERDOUBLE",   # double scalar VAR (t, dt, celsius)
        "USERPROPERTY", # section-level property (L, nseg, Ra)
        "USERFLOAT",    # float scalar VAR — declared in hocdec.h:86 but not
                        # known to be probable from a stock symbol table;
                        # left None unless discovery succeeds.
        "_RAW",         # debug: full discovered map
    )

    def __init__(self):
        for name in self.__slots__:
            object.__setattr__(self, name, None)
        object.__setattr__(self, "_RAW", {})

    def discover(self):
        """Probe known top-level symbols to populate type codes."""
        # x3d / finitialize → FUN_BLTIN (280, C-registered)
        # FUNCTION (270, HOC `func`-style returning double) is the same
        # code used for method-returns-double; we probe it via Vector.size
        # in discover_template_scoped.
        probes = {
            "VAR":        b"t",
            "BLTIN":      b"sin",
            "FUN_BLTIN":  b"finitialize",
            "STRINGFUNC": b"secname",
            "OBJECTFUNC": b"object_pushed",  # built-in obfunc returning Object
            "MECHANISM":  b"hh",
            "TEMPLATE":   b"IClamp",
            "RANGEVAR":   b"v",
        }
        for attr, sym_name in probes.items():
            sym = _nrn_symbol(sym_name)
            if sym:
                code = int(_nrn_symbol_type(sym))
                object.__setattr__(self, attr, code)
                self._RAW[attr] = code

        # VAR subtypes (hocdec.h:83-90). Stable across NEURON versions
        # but probed so we never embed the integer literal in dispatch
        # code. Expected: USERINT=1, USERDOUBLE=2, USERPROPERTY=3.
        # USERFLOAT (4) has no obvious stock probe and is left None.
        subtype_probes = {
            "USERINT":      b"stoprun",  # int stop flag
            "USERDOUBLE":   b"t",        # double simulation time
            "USERPROPERTY": b"nseg",     # section-level property
        }
        for attr, sym_name in subtype_probes.items():
            sym = _nrn_symbol(sym_name)
            if sym:
                st = int(_nrn_symbol_subtype(sym))
                object.__setattr__(self, attr, st)
                self._RAW[attr] = st

    def discover_template_scoped(self, hoc_exec):
        """Discover template-scoped and method-level codes.

        Also discovers top-level codes that need an active HOC session
        (PROCEDURE — no built-in matches at startup until stdrun loads).

        ``hoc_exec`` is a callable that runs a HOC string (e.g. ``n``).
        """
        # TODO(gap-46): _mn_type_probe_proc, _mn_type_probe_str, and
        # _MNTypeProbe are permanently visible in the HOC namespace after this
        # runs. No C API exists to delete HOC symbols, so they leak into the
        # global symbol table (harmless, but shows up in a full name dump).

        # Define a top-level proc to discover the PROCEDURE code.
        hoc_exec("proc _mn_type_probe_proc() { local x }")
        sym = _nrn_symbol(b"_mn_type_probe_proc")
        if sym:
            code = int(_nrn_symbol_type(sym))
            object.__setattr__(self, "PROCEDURE", code)
            self._RAW["PROCEDURE"] = code

        # OBJECTVAR (324): `hoc_obj_` is a built-in objref always present.
        sym = _nrn_symbol(b"hoc_obj_")
        if sym:
            code = int(_nrn_symbol_type(sym))
            object.__setattr__(self, "OBJECTVAR", code)
            self._RAW["OBJECTVAR"] = code

        # STRING (260): define a throwaway strdef to probe.
        hoc_exec("strdef _mn_type_probe_str")
        sym = _nrn_symbol(b"_mn_type_probe_str")
        if sym:
            code = int(_nrn_symbol_type(sym))
            object.__setattr__(self, "STRING", code)
            self._RAW["STRING"] = code

        # Probe template-scoped section + obfunc method.
        hoc_exec(
            "begintemplate _MNTypeProbe\n"
            "public soma, get_obj\n"
            "create soma\n"
            "obfunc get_obj() { return new Vector(1) }\n"
            "proc init() { soma { L=10 } }\n"
            "endtemplate _MNTypeProbe\n"
        )
        # Instantiate via the C API directly so we don't depend on Python
        # dispatch (which itself reads TYPES — bootstrapping concern).
        sym = _nrn_symbol(b"_MNTypeProbe")
        obj = _nrn_object_new(sym, 0) if sym else None
        if obj:
            try:
                for member, attr in (
                    (b"soma", "SECTION"),
                    (b"get_obj", "OBFUNCTION"),
                ):
                    msym = _nrn_method_symbol(obj, member)
                    if msym:
                        code = int(_nrn_symbol_type(msym))
                        object.__setattr__(self, attr, code)
                        self._RAW[attr] = code
            finally:
                try:
                    _nrn_object_unref(obj)
                except Exception:
                    pass

        # Probe method-level codes via known built-in classes.
        # Vector.size  → HOC FUNCTION code (270), shared with top-level
        #                user-defined `func` and template `func` methods.
        # Vector.c     → C-side method returning object   (329)
        # Vector.label → method returning string          (330)
        vec_sym = _nrn_symbol(b"Vector")
        vec_obj = _nrn_object_new(vec_sym, 0) if vec_sym else None
        if vec_obj:
            try:
                for member, attr in (
                    (b"size", "FUNCTION"),
                    (b"c", "METHOD_OBFUNC"),
                    (b"label", "METHOD_STRFUNC"),
                ):
                    msym = _nrn_method_symbol(vec_obj, member)
                    if msym:
                        code = int(_nrn_symbol_type(msym))
                        object.__setattr__(self, attr, code)
                        self._RAW[attr] = code
            finally:
                try:
                    _nrn_object_unref(vec_obj)
                except Exception:
                    pass

    def probe_section_ref(self, section_ref_obj):
        """Probe METHOD_SECTIONREF using a live SectionRef instance.

        Called from __init__.py after the NEURON singleton can mint a
        real SectionRef Object wrapper.
        """
        msym = _nrn_method_symbol(section_ref_obj, b"sec")
        if msym:
            code = int(_nrn_symbol_type(msym))
            object.__setattr__(self, "METHOD_SECTIONREF", code)
            self._RAW["METHOD_SECTIONREF"] = code

    def record_rangeobj(self, code):
        """Record RANGEOBJ after a public wrapper validates a live symbol.

        Stock NEURON has no RANGEOBJ symbol before a RANDOM-bearing MOD file is
        loaded, so import-time probing is impossible. The fail-closed public
        wrappers validate the symbol first; only then is its runtime type code
        safe to cache here.
        """
        code = int(code)
        current = self.RANGEOBJ
        if current is not None and current != code:
            raise RuntimeError(
                f"RANGEOBJ type code changed from {current} to {code}"
            )
        object.__setattr__(self, "RANGEOBJ", code)
        self._RAW["RANGEOBJ"] = code

    # Names that are allowed to stay None after discovery (documented).
    # USERFLOAT (hocdec.h:86) has no obvious stock-symbol probe; no
    # dispatch path currently depends on it.
    _OPTIONAL = frozenset({"_RAW", "USERFLOAT", "RANGEOBJ"})

    def validate(self):
        """Return True if every required type code was discovered."""
        missing = [n for n in self.__slots__
                   if n not in self._OPTIONAL and getattr(self, n) is None]
        if missing:
            import warnings
            warnings.warn(
                "myneuron: could not discover type codes for: "
                + ", ".join(missing)
                + ". Some dispatch paths may not work."
            )
            return False
        return True

    def __repr__(self):
        pairs = [(n, getattr(self, n)) for n in self.__slots__
                 if n != "_RAW"]
        return "TypeCodes(" + ", ".join(f"{n}={v}" for n, v in pairs) + ")"


# Global singleton, populated by discover() at module import time below.
TYPES = TypeCodes()
TYPES.discover()
# Template-scoped discovery happens in __init__.py after the NEURON
# singleton is constructed (avoids circular import). For now seed the
# expected codes with what `discover()` finds; SECTION/OBFUNCTION are
# filled later. Callers that read TYPES.SECTION before init finishes
# get None — but that path doesn't run during normal startup.
