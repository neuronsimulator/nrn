"""ctypes bindings to libnrniv for NEURON 9+."""
import collections as _collections
import ctypes
import ctypes.util
import glob
import importlib.util
import math
import operator
import os
import site
import sys
import warnings
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
    this, `load_file("stdrun.hoc")` fails, which breaks nearly every
    model that uses `run()` or `finitialize()` via stdrun. Set only as a
    default; an existing value (system NEURON, custom install) wins.
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
    # Fallback locations: find_spec("neuron") may resolve to a shim package
    # that has no `.data/`.
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
        for name in ("libnrniv.so", "libnrniv.dylib", "nrniv.dll", "libnrniv.dll"):
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
    """Load a shared library.

    Tries ``$envvar``, then ``candidates``, then ``ctypes.util.find_library``.
    Raises ImportError listing the paths tried. ``loader`` is ``ctypes.CDLL``
    or ``ctypes.PyDLL``; libnrniv needs PyDLL (see the comment above its load).
    """
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
        for d in _vendored_lib_dirs()
        for suffix in (".so", ".dylib")
    ]
    if _VENDORED
    else [
        *sorted(_bundled_lib_dir.glob("libmodlreg*.so")),
        *sorted(_bundled_lib_dir.glob("libmodlreg*.dylib")),
        _local_lib_dir / "libmodlreg.so",
        _local_lib_dir / "libmodlreg.dylib",
    ],
    mode=_rtld_global,
)

# libnrniv must load as PyDLL so the GIL stays held around every call. HOC
# calls such as nrn_load_dll reach libnrnpython's nrnpy_reg_mech, which calls
# PyDict_GetItemString without taking the GIL. Every mod file with USEION
# (ion_reg -> register_mech) takes this path; CDLL = process crash:
#   nrnoc/init.cpp:737            register_mech -> nrnpy_reg_mech_p_(mechtype)
#   nrnpython/nrnpy_nrn.cpp:3092  nrnpy_reg_mech_p_ = nrnpy_reg_mech
#   nrnpython/nrnpy_nrn.cpp:3125  nrnpy_reg_mech -> PyDict_GetItemString
libnrniv = _load_cdll_or_raise(
    label="nrniv",
    envvar=_LIBNRNIV_ENV,
    candidates=_nrniv_candidates_from_neuron_wheel(),
    loader=ctypes.PyDLL,
)


def _bind(name, restype, *argtypes):
    """Return libnrniv's function *name* with its C signature set."""
    function = getattr(libnrniv, name)
    function.argtypes = list(argtypes)
    function.restype = restype
    return function


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
        raise RuntimeError(
            f"NEURON error: {msg}" if msg else "NEURON error (no message)"
        )


#
# initialize NEURON
#
# argv:
#   "-nogui"    Opt-in via MYNEURON_NOGUI=1. Real NEURON's PyInit_hoc never
#               passes it, and it forces Graph.view_count() to 0 even on a
#               live display. The test suite opts in so PlotShape/plotting
#               tests are order-independent without a DISPLAY. See
#               docs/ARCHITECTURE.md#nrn_init.
#   "-nopython" Passed when the methods table is already populated (avoids a
#               duplicate class2oc("PythonObject", ...)) and for Option B
#               (libnrniv registers its stub PythonObject before myneuron
#               installs callbacks). Only MYNEURON_USE_CFUNCTYPE=0 drops it,
#               so libnrnpython can populate the methods table.

_methods_populated = False
try:
    _methods_sym = ctypes.c_void_p.in_dll(libnrniv, "_ZN6neuron6python7methodsE")
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

    major, minor = sys.version_info[:2]
    return major * 100 + minor if minor >= 10 else major * 10 + minor


def _libnrnpython_present_next_to_libnrniv():
    lib_dir = os.path.dirname(libnrniv._name or "")
    if not lib_dir:
        return False
    for pat in ("libnrnpython*.so", "libnrnpython*.dylib"):
        if glob.glob(os.path.join(lib_dir, pat)):
            return True
    return False


# --- Methods-table population strategy ----------------------------
# Three ways to fill neuron::python::methods (Options A/B/C are defined here;
# tests and docs/KNOWN_GAPS.md use these letters):
#
#   A. Already populated: keep the existing pointers (`import neuron` ran
#      first, or libnrnpython auto-loaded in a prior process).
#   B. CFUNCTYPE (default when methods is empty): libnrnpython is not loaded.
#      With `-nopython`, libnrniv's nrnpython_reg() (src/nrniv/nrnpy.cpp:253)
#      registers the stub `class2oc("PythonObject", stub_cons, stub_destruct,
#      ...)`. After `nrn_init` we look up the stub symbol, set
#      `nrnpy_pyobj_sym_`, and install Python-implemented function pointers
#      into `methods` via `ctypes.CFUNCTYPE`.
#   C. libnrnpython load (opt-out via `MYNEURON_USE_CFUNCTYPE=0`): drop
#      `-nopython`, pre-set `nrn_is_python_extension`, and let libnrniv's
#      `nrnpython_reg -> load_nrnpython -> nrnpython_reg_real` chain populate
#      all 29 pointers from libnrnpython's C++ code.
#
# In B, _install_cfunctype_methods replaces the stub template's no-op
# destructor with a registry cleanup callback. A and C must never use ctypes
# payloads: their C++ destructor requires `new Py2Nrn`.
_use_cfunctype = os.environ.get("MYNEURON_USE_CFUNCTYPE", "1") != "0"
_libnrnpython_available = _libnrnpython_present_next_to_libnrniv()

# C only when CFUNCTYPE is disabled and a libnrnpython sibling exists.
_use_libnrnpython_load = (
    not _methods_populated and not _use_cfunctype and _libnrnpython_available
)

# See the argv notes above.
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
    # A and B pass -nopython.
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

nrn_init = _bind(
    "nrn_init", ctypes.c_int, ctypes.c_int, ctypes.POINTER(ctypes.c_char_p)
)

ret = nrn_init(_argc, argv)
if ret:
    raise RuntimeError("nrn_init failed with return code %d" % ret)


# --- Option B: CFUNCTYPE methods table ----------------------------
#
# Private layouts we depend on. tests/regression/test_cfunctype_methods_layout.py
# asserts these against NEURON 9 at runtime; if a NEURON bump shifts them,
# update the constants and re-run it:
#   Object.u offset           : 8     (oc/hocdec.h:173)
#   Py2Nrn.po_ offset         : 8     (nrnpython/nrnpy_p2h.cpp:27)
#   sizeof(Py2Nrn)            : 16
#   impl_ptrs.hoccommand_exec : field 11 -> byte offset 88
#                               (nrniv/nrnpy.h:25-54)
#   impl_ptrs.pysame          : field 25 -> byte offset 200
#   Object.ctemplate          : offset 16  (oc/hocdec.h:173)
#   cTemplate.sym             : offset 0   (oc/hocdec.h:146)
_OBJECT_U_OFFSET = 8
_OBJECT_CTEMPLATE_OFFSET = 16
_SYMBOL_U_OFFSET = 16
_CTEMPLATE_DESTRUCTOR_OFFSET = 80
_PY2NRN_PO_OFFSET = 8
_PY2NRN_SIZEOF = 16
_METHODS_HOCCOMMAND_EXEC_OFFSET = 88
# hpoasgn: HOC writing a Python object's component (`$o1.x = v`). Field 15 of
# impl_ptrs (nrn/src/nrniv/nrnpy.h). The struct is a flat array of 8-byte
# function pointers, so offset = field_index * 8 = 120.
_METHODS_HPOASGN_OFFSET = 120
_METHODS_PYSAME_OFFSET = 200
# py2n_component: HOC reading a Python object's component (`x = pyobj.attr`).
# Field 26, right after pysame (field 25 = 200). A probe at offset 208 fires
# from hoc_object_component with nindex=0/isfunc=0 for `$o1.x`; 216 leaves the
# live slot NULL and SIGSEGVs.
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
_hoc_top_level_symlist = ctypes.c_void_p.in_dll(libnrniv, "hoc_top_level_symlist")


def _call_at_top_level(target, args):
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


# hoc_nrnpython (field 13, after hoccommand_exec_strret at 96): the builtin
# nrnpython() calls it. Inside a template HOC resolves the builtin rather than
# a top-level registration (ModelView's `nrnpython("import neuron")`).
_METHODS_HOC_NRNPYTHON_OFFSET = 104


def _install_methods_slot(offset, callback):
    """Fill one empty methods-table slot; a native provider's stays untouched."""
    try:
        methods = ctypes.c_void_p.in_dll(libnrniv, "_ZN6neuron6python7methodsE")
    except ValueError:
        return False
    slot = ctypes.c_void_p.from_address(ctypes.addressof(methods) + offset)
    if slot.value:
        return False
    slot.value = ctypes.cast(callback, ctypes.c_void_p).value
    return True


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
    # Option B keeps this ctypes allocation in _PY2NRN_REGISTRY; its template
    # destructor removes the entry.
    _fields_ = [
        ("type_", ctypes.c_int),
        ("_pad", ctypes.c_int),
        ("po_", ctypes.c_void_p),
    ]


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


# Present on the pinned dev115; absent on the dev114 lane, which falls back to
# the legacy methods-table slots. Native providers keep their own hooks and
# ownership.
_nrn_template_set_component_hooks = getattr(
    libnrniv, "nrn_template_set_component_hooks", None
)
if _nrn_template_set_component_hooks is not None:
    _nrn_template_set_component_hooks.argtypes = [
        ctypes.c_void_p,
        ctypes.c_void_p,
        ctypes.c_void_p,
        ctypes.c_char_p,
        ctypes.c_size_t,
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
    return (
        ctypes.c_void_p.from_address(this_ptr + _PY2NRN_PO_OFFSET).value
        if this_ptr
        else None
    )


def _unwrap_pythonobject(obj):
    """Return the Python payload borrowed from a live PythonObject."""
    if not obj:
        return None
    if _nrn_class_name(obj) != b"PythonObject":
        raise TypeError("expected a HOC PythonObject")
    po_handle = _pythonobject_payload_ptr(obj)
    if not po_handle:
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


# Bounded rotating keepalive for strings pushed back to HOC from a Python
# component read. The pushed char** must stay valid until HOC consumes
# it (within the same expression). Mirrors real NEURON's hoc_temp_charptr,
# which hands out slots from a small rotating pool; a bounded ring keeps the
# buffers alive across the immediate read without growing without bound.
_PY2N_STR_POOL = _collections.deque(maxlen=64)


def _install_cfunctype_methods():
    """Option B: populate the methods table with CFUNCTYPE callbacks.

    Installs the stub template's destructor, hoccommand_exec (offset 88),
    pysame (offset 200) and the PythonObject component read/write hooks
    (public nrn_template_set_component_hooks on dev115; legacy slots 208/120
    on dev114). No-op if hoccommand_exec is already set (Option A or C).
    """
    global _cfunctype_methods_installed, _public_component_hooks_installed
    public_components = _nrn_template_set_component_hooks is not None

    if not _use_cfunctype:
        return

    # If methods.hoccommand_exec is already set by another path, leave it.
    try:
        _methods_sym2 = ctypes.c_void_p.in_dll(libnrniv, "_ZN6neuron6python7methodsE")
    except ValueError:
        return
    _methods_addr = ctypes.addressof(_methods_sym2)
    _hcx_slot = ctypes.c_void_p.from_address(
        _methods_addr + _METHODS_HOCCOMMAND_EXEC_OFFSET
    )
    if _hcx_slot.value:
        return  # already populated

    # Wire the stub PythonObject template into nrnpy_pyobj_sym_ so
    # nrn_object_new_wrap(sym, payload) works later. Public `nrn_symbol` is
    # hoc_lookup (neuronapi.cpp); it is bound locally for bootstrap ordering.
    try:
        _sym_lookup = _bind("nrn_symbol", ctypes.c_void_p, ctypes.c_char_p)
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

    # The stub template's destructor is a no-op. Replace it so the ctypes-owned
    # Py2Nrn payload and its Python object are released when the HOC object's
    # refcount reaches zero. Only done after the methods table was verified
    # empty; A and C keep libnrnpython's C++ destructor.
    _PYOBJECT_DESTRUCTOR_T = ctypes.CFUNCTYPE(None, ctypes.c_void_p)

    @_PYOBJECT_DESTRUCTOR_T
    def _py_pythonobject_destructor(payload):
        entry = _PY2NRN_REGISTRY.pop(payload, None)
        if entry is not None:
            entry[0].po_ = None

    _CFUNCTYPE_KEEPALIVE.append(_py_pythonobject_destructor)
    _ctemplate = ctypes.c_void_p.from_address(_sym + _SYMBOL_U_OFFSET).value
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
        # the registry-owned PyObject*, and call it. Return 1
        # on success so cvodeobj.cpp:511 doesn't `hoc_execerror`.
        if not obj_ptr:
            return 0
        try:
            po_handle = _pythonobject_payload_ptr(obj_ptr)
            if not po_handle:
                return 0
            callback = ctypes.cast(po_handle, ctypes.py_object).value
        except BaseException as exc:
            import sys

            print(
                f"myneuron CFUNCTYPE hoccommand_exec: " f"{type(exc).__name__}: {exc}",
                file=sys.stderr,
            )
            return 0
        try:
            # Same dispatch as NEURON's hoccommand_exec_help1: a
            # (callable, args) tuple calls callable(*args), with a non-tuple
            # args value passed as the single argument.
            if isinstance(callback, tuple):
                args = callback[1] if isinstance(callback[1], tuple) else (callback[1],)
                callback[0](*args)
            else:
                callback()
            return 1
        except BaseException:
            # The core reports the failure from the return value; an
            # exception must not cross ctypes. NEURON prints the same text.
            import sys
            import traceback

            print("NEURON: Python Callback failed [hoccommand_exec]:", file=sys.stderr)
            traceback.print_exc()
            return 0

    _CFUNCTYPE_KEEPALIVE.append(_py_hoccommand_exec)
    _hcx_slot.value = ctypes.cast(_py_hoccommand_exec, ctypes.c_void_p).value

    # methods.pysame: needed by cvodeobj.cpp:540's
    # extra_scatter_gather_remove. Without it, removing a callback
    # dereferences a NULL function pointer. Mirrors `pysame` in
    # nrnpython/nrnpy_p2h.cpp:69 -- ho_eq_po composition collapsed
    # to: same PythonObject template AND same PyObject* handle.
    _PYSAME_T = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p)
    _nrnpy_sym_addr = ctypes.c_void_p.in_dll(libnrniv, "nrnpy_pyobj_sym_").value

    @_PYSAME_T
    def _py_pysame(o1, o2):
        if not o1 or not o2:
            return 0
        try:
            ct1 = ctypes.c_void_p.from_address(o1 + _OBJECT_CTEMPLATE_OFFSET).value
            ct2 = ctypes.c_void_p.from_address(o2 + _OBJECT_CTEMPLATE_OFFSET).value
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
    _pysame_slot = ctypes.c_void_p.from_address(_methods_addr + _METHODS_PYSAME_OFFSET)
    _pysame_slot.value = ctypes.cast(_py_pysame, ctypes.c_void_p).value

    # Public neuronapi stack kinds. On dev114 and dev115, querying the kind of a
    # dimension marker throws. Consume known markers with nrn_int_pop, never stack_type.
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
                raise TypeError(f"cannot return Python {type(result).__name__} to HOC")
            try:
                _nrn_object_push(obj_ptr)
            finally:
                # Transfer the creator-owned reference to the HOC temp stack.
                _nrn_object_unref(obj_ptr)

    _call_python_component = _call_at_top_level

    def _python_component_target(py_obj, name, obj):
        # The top-level PythonObject (no payload) evaluates the name in
        # __main__'s namespace like NEURON's py2n_component (PyRun_String),
        # so builtins such as `list` resolve too. Otherwise `._` is the
        # payload itself.
        if not _pythonobject_payload_ptr(obj):
            import __main__

            try:
                return eval(name, vars(__main__))
            except NameError:
                raise AttributeError(
                    f"module '__main__' has no attribute {name!r}"
                ) from None
        if name == "_":
            return py_obj
        return getattr(py_obj, name)

    def _check_component_dimensions(ndim):
        if ndim != 1:
            raise RuntimeError(
                f"{ndim} dimensional python objects can't be accessed from hoc "
                "with var._[i1][i2]... syntax. Must use var._[i1]._[i2]... "
                "hoc syntax."
            )

    # Component read hook (legacy methods.py2n_component at offset 208):
    # HOC reading a Python object's component, `x = pyobj.attr`. A NULL legacy
    # slot SIGSEGVs inside hoc_object_component. Method arguments are popped by
    # public stack type and reversed into source order. For indexed reads,
    # nindex says a dimension marker precedes the indices; nrn_int_pop
    # consumes it.
    # Error contract: public hooks return an error pointer so core aborts
    # before another HOC statement executes, and dispatch rolls back restored
    # arguments before raising the stashed Python exception. The dev114
    # fallback instead drains the frame and pushes a placeholder.
    _PY2N_COMPONENT_T = ctypes.CFUNCTYPE(
        ctypes.c_void_p if public_components else None,
        ctypes.c_void_p,
        ctypes.c_void_p,
        ctypes.c_int,
        ctypes.c_int,
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
                    raise RuntimeError(
                        "PythonObject dimension marker disagrees with frame"
                    )
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
                        raise OverflowError(
                            "HOC read index is outside the C long range"
                        )
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

    # Assignment hook (legacy methods.hpoasgn at offset 120): HOC writing a
    # Python object's component, `$o1.x = v`. Called from hoc_object_asgn when
    # the left-hand side is a PythonObject. Operand stack on entry, top to
    # bottom: the RHS value, the PythonObject `o`, the attribute-name Symbol,
    # and nindex (0 for `.x`, 1 for a `._[i]` subscript). When indexed, a
    # dimension marker and the indices follow (pushed by hoc_object_component
    # under `isfunc & 2`). Pop in exactly that order, as the real hpoasgn in
    # nrnpy_p2h.cpp does, to keep the stack balanced; then setattr (or
    # __setitem__ with a truncated numeric index) on the live object.
    # Error contract: same as the read hook above.
    # HPOASGN_RHS_* are the possible `type_` values: parse.hpp
    # (NUMBER/STRING/OBJECTVAR) and hocdec.h (OBJECTTMP=8).
    _HPOASGN_RHS_NUMBER = 259
    _HPOASGN_RHS_STRING = 260
    _HPOASGN_RHS_OBJECTVAR = 324
    _HPOASGN_RHS_OBJECTTMP = 8
    _HPOASGN_T = (
        ctypes.CFUNCTYPE(ctypes.c_void_p, ctypes.c_void_p)
        if public_components
        else ctypes.CFUNCTYPE(None, ctypes.c_void_p, ctypes.c_int)
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
                    raise RuntimeError(
                        "PythonObject dimension marker disagrees with frame"
                    )
                _check_component_dimensions(ndim)
                if not numeric_index:
                    raise TypeError(
                        "HOC PythonObject assignment requires a numeric index"
                    )
                # Only one dimension is accepted, so no reversal is needed.
                # Unlike reads, native writes use PyLong_FromDouble, not C long.
                key = int(indices[0])
            if target_obj != o:
                raise RuntimeError("hpoasgn target stack entry did not match")

            py_obj = _unwrap_pythonobject(target_obj)
            raw_name = _nrn_symbol_name(sym)
            name = raw_name.decode("utf-8") if isinstance(raw_name, bytes) else raw_name
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
            _sym,
            ctypes.cast(_py_py2n_component, ctypes.c_void_p),
            ctypes.cast(_py_hpoasgn, ctypes.c_void_p),
            error,
            len(error),
        ):
            raise ImportError(
                f"PythonObject component registration failed: {error.value!r}"
            )
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
    # in NEURON is still the version-safe long-term fix.


def _pop_looked_at_object():
    """Pop the PythonObject that hoc_obj_look_inside_stack left on the stack.

    py2n_component receives an object the interpreter peeked, not popped, and
    must consume it before pushing the component's value (real NEURON uses
    hoc_pop_defer()). The public pop returns an owned reference, so the
    discard helper releases it immediately.
    """
    try:
        _discard_object_stack_value()
    except BaseException:
        pass


_install_cfunctype_methods()


def _py_pyobj_to_hoc_cfunctype(py_callable):
    """Wrap a Python value as a new HOC Object* (Option B only).

    Allocates a Py2Nrn, keeps the value in the payload registry, and passes
    both to `nrn_object_new_wrap`; the object's `u.this_pointer` is the
    payload. Mirrors `nrnpy_pyobject_in_obj` (nrnpython/nrnpy_p2h.cpp:91).
    Returns None under Option A so callers use native nrnpy_po2ho, whose C++
    destructor matches its own payloads.
    """
    if not _cfunctype_methods_installed:
        return None
    try:
        _pyobj_sym_value = ctypes.c_void_p.in_dll(libnrniv, "nrnpy_pyobj_sym_").value
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
# The callback reference MUST be kept alive in module scope. If Python
# collects it, NEURON calls a dangling pointer and segfaults.
_STDOUT_CB_TYPE = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_int, ctypes.c_char_p)

_nrn_stdout_redirect = _bind("nrn_stdout_redirect", None, _STDOUT_CB_TYPE)

_stdout_callback_ref = None  # keep-alive for the active callback


def _default_stdout_sink(stream, msg):
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
# Aliases for opaque C types
Section = ctypes.c_void_p
Symlist = ctypes.c_void_p
Symbol = ctypes.c_void_p
Object = ctypes.c_void_p
SymbolTableIterator = ctypes.c_void_p
SectionListIterator = ctypes.c_void_p
nrn_Item = ctypes.c_void_p
ShapePlotInterface = ctypes.c_void_p

_nrn_section_new = _bind("nrn_section_new", Section, ctypes.c_char_p)

_nrn_hoc_call_raw = _bind("nrn_hoc_call", ctypes.c_int, ctypes.c_char_p)

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


_nrn_double_push = _bind("nrn_double_push", None, ctypes.c_double)

_nrn_global_symbol_table = _bind("nrn_global_symbol_table", Symlist)

_nrn_top_level_symbol_table = _bind("nrn_top_level_symbol_table", Symlist)

# --- Symbol table iterators ---
_nrn_symbol_table_iterator_new = _bind(
    "nrn_symbol_table_iterator_new", SymbolTableIterator, Symlist
)

_nrn_symbol_table_iterator_free = _bind(
    "nrn_symbol_table_iterator_free", None, SymbolTableIterator
)

_nrn_symbol_table_iterator_next = _bind(
    "nrn_symbol_table_iterator_next", Symbol, SymbolTableIterator
)

_nrn_symbol_table_iterator_done = _bind(
    "nrn_symbol_table_iterator_done", ctypes.c_int, SymbolTableIterator
)

_nrn_sectionlist_iterator_new = _bind(
    "nrn_sectionlist_iterator_new", SectionListIterator, nrn_Item
)

_nrn_sectionlist_iterator_free = _bind(
    "nrn_sectionlist_iterator_free", None, SectionListIterator
)

_nrn_sectionlist_iterator_next = _bind(
    "nrn_sectionlist_iterator_next", Section, SectionListIterator
)

_nrn_sectionlist_iterator_done = _bind(
    "nrn_sectionlist_iterator_done", ctypes.c_int, SectionListIterator
)

# --- Symbol metadata ---
_nrn_symbol = _bind("nrn_symbol", Symbol, ctypes.c_char_p)

_nrn_symbol_type = _bind("nrn_symbol_type", ctypes.c_int, Symbol)

_nrn_symbol_subtype = _bind("nrn_symbol_subtype", ctypes.c_int, Symbol)

_nrn_symbol_name = _bind("nrn_symbol_name", ctypes.c_char_p, Symbol)

_nrn_register_function = _bind(
    "nrn_register_function", None, ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int
)

_nrn_gargstr = _bind("nrn_gargstr", ctypes.c_char_p, ctypes.c_int)

# --- Stack operations (push / pop / HOC call) ---
# Read a double argument from the HOC operand stack inside a
# C-registered function callback. Returned pointer is owned by NEURON.
_nrn_getarg = _bind("nrn_getarg", ctypes.POINTER(ctypes.c_double), ctypes.c_int)

_nrn_hoc_ret = _bind("nrn_hoc_ret", None)

_nrn_function_call = _bind(
    "nrn_function_call_nothrow",
    ctypes.c_int,
    Symbol,
    ctypes.c_int,
    ctypes.c_char_p,
    ctypes.c_size_t,
)

_nrn_double_pop = _bind("nrn_double_pop", ctypes.c_double)

# --- Object API ---
_nrn_object_new_nothrow = _bind(
    "nrn_object_new_nothrow",
    ctypes.c_int,
    Symbol,
    ctypes.c_int,
    ctypes.POINTER(Object),
    ctypes.c_char_p,
    ctypes.c_size_t,
)


def _nrn_object_new(sym, narg):
    """Construct a HOC object without allowing C++ exceptions across ctypes."""
    result = Object()
    err = ctypes.create_string_buffer(_ERR_BUF_SIZE)
    rc = _nrn_object_new_nothrow(sym, narg, ctypes.byref(result), err, _ERR_BUF_SIZE)
    if rc:
        msg = err.value.decode("utf-8", errors="replace")
        raise RuntimeError(msg or "HOC object construction failed")
    return result.value


_nrn_object_new_wrap = _bind("nrn_object_new_wrap", Object, Symbol, ctypes.c_void_p)

_nrn_object_ref = _bind("nrn_object_ref", None, Object)

_nrn_object_unref = _bind("nrn_object_unref", None, Object)

_nrn_symbol_table = _bind("nrn_symbol_table", Symlist, Symbol)

_nrn_class_name = _bind("nrn_class_name", ctypes.c_char_p, Object)

_nrn_method_symbol = _bind("nrn_method_symbol", Symbol, Object, ctypes.c_char_p)
_nrn_symbol_dataptr = _bind(
    "nrn_symbol_dataptr", ctypes.POINTER(ctypes.c_double), Symbol
)

# Smallest address treated as a real pointer from nrn_symbol_dataptr. For a
# NOTUSER runtime scalar (`n('x = 42')`) the storage is
# hoc_top_level_data[sym->u.oboff].pval. With nrn#3815 the call returns that
# address; older builds return the raw sym->u.oboff (a small symbol-table
# index, always below this threshold) as a pointer, and dereferencing it
# segfaults. The subtype-0 paths in __init__.py test against this to choose
# direct dereference or the hoc_ac_ trampoline / HOC-assign fallback.
_MIN_VALID_DATAPTR = 0x10000

_nrn_method_call = _bind(
    "nrn_method_call_nothrow",
    ctypes.c_int,
    Object,
    Symbol,
    ctypes.c_int,
    ctypes.c_char_p,
    ctypes.c_size_t,
)

# HOC defers one object unref when popping an OBJECTTMP (hoc_pop_defer).
# Real NEURON flushes it after every function/method call and when a wrapper
# is released (nrnpy_hoc.cpp:264, :847). Without the flush the deferred object
# outlives its owner: a NetCon destroyed after its source double-frees in
# PreSyn::~PreSyn. Not in the public C API; bound by its exported mangled
# name, a no-op if the core does not export it.
try:
    _hoc_unref_defer = _bind("_Z15hoc_unref_deferv", None)
except AttributeError:

    def _hoc_unref_defer():
        pass


_nrn_vector_data = _bind("nrn_vector_data", ctypes.POINTER(ctypes.c_double), Object)
# Despite the name, returns the Vector's size (vector_capacity, ivocvect.cpp).
_nrn_vector_capacity = _bind("nrn_vector_capacity", ctypes.c_int, Object)

_nrn_section_pop = _bind("nrn_section_pop", None)

# nrn_str_pop() returns char** owned by NEURON; _nrn_str_pop() decodes it to str.
_nrn_str_pop0 = _bind("nrn_str_pop", ctypes.POINTER(ctypes.c_char_p))


def _decode_hoc_string(p):
    """Decode after frame accounting; invalid UTF-8 can raise after a raw pop."""
    if not p:
        return ""
    s = p.contents.value
    if s is None:
        return ""
    return s.decode("utf-8")


def _nrn_str_pop():
    """Return a Python string from _nrn_str_pop0() result."""
    return _decode_hoc_string(_nrn_str_pop0())


_nrn_object_pop = _bind("nrn_object_pop", Object)

_nrn_stack_type = _bind("nrn_stack_type", ctypes.c_int)

_nrn_symbol_pop = _bind("nrn_symbol_pop", Symbol)


def _object_pop_safe():
    """Pop an object value, returning an owned Object* or None for nil."""
    return _nrn_object_pop()


def _discard_object_stack_value():
    """Consume one object stack entry without retaining its object."""
    obj = _object_pop_safe()
    if obj:
        _nrn_object_unref(obj)


_nrn_int_pop = _bind("nrn_int_pop", ctypes.c_int)


def _symbol_pop():
    return _nrn_symbol_pop()


_nrn_str_push = _bind("nrn_str_push", None, ctypes.POINTER(ctypes.c_char_p))

_nrn_object_push = _bind("nrn_object_push", None, Object)

_nrn_object_ptr_push = _bind("nrn_object_ptr_push", None, ctypes.POINTER(Object))


def _object_ptr_push(obj_ref):
    _nrn_object_ptr_push(obj_ref)


# --- Python callable -> HOC PythonObject (lazy) ----------------------------
# Real NEURON converts a Python callable to a HOC `Object*` of class
# "PythonObject" via `nrnpy_po2ho` (src/nrnpython/nrnpy_hoc.cpp:598), which
# lives in libnrnpython.cpython-3.X.so (mangled symbol
# `_Z11nrnpy_po2hoP7_object`), not libnrniv. It is safe to call only after
# `nrnpython_reg_real` has run inside libnrnpython: that registration
# installs the "PythonObject" template and fills `neuron::python::methods`
# so `hoccommand_exec` can call back into the wrapped callable. `import
# neuron` does that registration (the bootstrap-order rule in CLAUDE.md).
# Standalone myneuron (no neuron loaded) normally never reaches this path: the
# default CFUNCTYPE provider wraps values itself (_py_pyobj_to_hoc_cfunctype).
# It is the fallback when that provider is not installed. If libnrnpython or
# the symbol cannot be found, this returns None and callers raise TypeError.

_pyobj_to_hoc_object = None  # populated lazily by _ensure_pyobj_to_hoc()


def _ensure_pyobj_to_hoc():
    """Return a ctypes-bound ``nrnpy_po2ho(PyObject*) -> Object*`` or None.

    Returns None if libnrnpython can't be located or loaded.

    Side effect: if nrnpy_pyobj_sym_ is unset, sets it to the stub PythonObject
    symbol (needed when -nopython ran without loading libnrnpython).
    """
    global _pyobj_to_hoc_object
    # TODO(gap-60): discover the native allocator in static Python extensions.
    if _pyobj_to_hoc_object is not None:
        return _pyobj_to_hoc_object

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

    # Under -nopython the stub PythonObject is registered but
    # nrnpy_pyobj_sym_ (normally set by nrnpython_reg_real) stays unset.
    # Calling nrnpython_reg_real here would fail with "PythonObject already
    # being used as a name", so assign the stub symbol ourselves. That is
    # enough for nrnpy_pyobject_in_obj to allocate PythonObject Objects; the
    # stub's p_cons is not called on this path (only HOC's
    # `new PythonObject()` calls it).
    try:
        pyobj_sym = ctypes.c_void_p.in_dll(libnrniv, "nrnpy_pyobj_sym_")
    except ValueError:
        pyobj_sym = None
    if pyobj_sym is not None and not pyobj_sym.value:
        # Same local nrn_symbol binding as in _install_cfunctype_methods.
        try:
            _sym_lookup = _bind("nrn_symbol", ctypes.c_void_p, ctypes.c_char_p)
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


_nrn_double_ptr_push = _bind(
    "nrn_double_ptr_push", None, ctypes.POINTER(ctypes.c_double)
)

_nrn_allsec = _bind("nrn_allsec", nrn_Item)

_nrn_sectionlist_data = _bind("nrn_sectionlist_data", nrn_Item, Object)

# Batched section-list gather (nrn#3834). Call with buf=NULL, maxlen=0 to
# obtain the total live count, then again with a caller-owned buffer.
_nrn_sectionlist_to_array = _bind(
    "nrn_sectionlist_to_array",
    ctypes.c_int,
    nrn_Item,
    ctypes.POINTER(ctypes.c_void_p),
    ctypes.c_int,
)

# --- Section API ---
_nrn_section_push = _bind("nrn_section_push", None, Section)

_nrn_mechanism_insert = _bind("nrn_mechanism_insert", None, Section, Symbol)

_nrn_rangevar_get = _bind(
    "nrn_rangevar_get", ctypes.c_double, Symbol, Section, ctypes.c_double
)

_nrn_section_connect = _bind(
    "nrn_section_connect", None, Section, ctypes.c_double, Section, ctypes.c_double
)

_nrn_section_length_set = _bind(
    "nrn_section_length_set", None, Section, ctypes.c_double
)

_nrn_section_length_get = _bind("nrn_section_length_get", ctypes.c_double, Section)

_nrn_secname = _bind("nrn_secname", ctypes.c_char_p, Section)

_nrn_nseg_get = _bind("nrn_nseg_get", ctypes.c_int, Section)

_nrn_nseg_set = _bind("nrn_nseg_set", None, Section, ctypes.c_int)

_nrn_section_diam_set = _bind(
    "nrn_segment_diam_set", None, Section, ctypes.c_double, ctypes.c_double
)

_nrn_section_diam_get = _bind(
    "nrn_segment_diam_get", ctypes.c_double, Section, ctypes.c_double
)

_nrn_segment_node_index = _bind(
    "nrn_segment_node_index", ctypes.c_int, Section, ctypes.c_double
)

# Section-tree accessors (nrn#3835). Each returns a Section* or NULL.
_nrn_section_parent = _bind("nrn_section_parent", Section, Section)

_nrn_section_trueparent = _bind("nrn_section_trueparent", Section, Section)

_nrn_section_child = _bind("nrn_section_child", Section, Section)

_nrn_section_sibling = _bind("nrn_section_sibling", Section, Section)

_nrn_property_get = _bind("nrn_property_get", ctypes.c_double, Object, ctypes.c_char_p)

_nrn_property_array_get = _bind(
    "nrn_property_array_get", ctypes.c_double, Object, ctypes.c_char_p, ctypes.c_int
)

_nrn_property_set = _bind(
    "nrn_property_set", None, Object, ctypes.c_char_p, ctypes.c_double
)

_nrn_property_array_set = _bind(
    "nrn_property_array_set",
    None,
    Object,
    ctypes.c_char_p,
    ctypes.c_int,
    ctypes.c_double,
)

_nrn_property_data_handle_is_valid = _bind(
    "nrn_property_data_handle_is_valid",
    ctypes.c_bool,
    Object,
    ctypes.c_char_p,
    ctypes.c_int,
)


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


_nrn_section_is_active = _bind("nrn_section_is_active", ctypes.c_bool, Section)

_nrn_rangevar_set = _bind(
    "nrn_rangevar_set", None, Symbol, Section, ctypes.c_double, ctypes.c_double
)

# Both return one owned Object* reference, or NULL for a non-live RANDOM member.
_nrn_segment_nmodlrandom_get = _bind(
    "nrn_segment_nmodlrandom_get", Object, Section, ctypes.c_double, Symbol
)

_nrn_pntproc_nmodlrandom_get = _bind(
    "nrn_pntproc_nmodlrandom_get", Object, Object, Symbol
)

# PlotShape accessors; see plotting._PlotShapePlot._get_plot_data.
_nrn_get_plotshape_interface = _bind(
    "nrn_get_plotshape_interface", ShapePlotInterface, Object
)

_nrn_symbol_is_array = _bind("nrn_symbol_is_array", ctypes.c_bool, Symbol)

_nrn_get_plotshape_section_list = _bind(
    "nrn_get_plotshape_section_list", Object, ShapePlotInterface
)

_nrn_get_plotshape_varname = _bind(
    "nrn_get_plotshape_varname", ctypes.c_char_p, ShapePlotInterface
)

_nrn_get_plotshape_low = _bind(
    "nrn_get_plotshape_low", ctypes.c_float, ShapePlotInterface
)

_nrn_get_plotshape_high = _bind(
    "nrn_get_plotshape_high", ctypes.c_float, ShapePlotInterface
)

_nrn_symbol_array_length = _bind("nrn_symbol_array_length", ctypes.c_int, Symbol)

# --- Ra, rallbranch, ref/unref, distance ---
_nrn_section_Ra_get = _bind("nrn_section_Ra_get", ctypes.c_double, Section)

_nrn_section_Ra_set = _bind("nrn_section_Ra_set", None, Section, ctypes.c_double)

_nrn_section_rallbranch_get = _bind(
    "nrn_section_rallbranch_get", ctypes.c_double, Section
)

_nrn_section_rallbranch_set = _bind(
    "nrn_section_rallbranch_set", None, Section, ctypes.c_double
)

_nrn_rangevar_push = _bind("nrn_rangevar_push", None, Symbol, Section, ctypes.c_double)

_nrn_setpointer_pop = _bind(
    "nrn_setpointer_pop",
    ctypes.c_int,
    Symbol,
    Section,
    ctypes.c_double,
    ctypes.c_char_p,
    ctypes.c_size_t,
)

_nrn_pp_setpointer_pop = _bind(
    "nrn_pp_setpointer_pop",
    ctypes.c_int,
    Object,
    ctypes.c_char_p,
    ctypes.c_char_p,
    ctypes.c_size_t,
)

_nrn_property_push = _bind("nrn_property_push", None, Object, ctypes.c_char_p)

_nrn_property_array_push = _bind(
    "nrn_property_array_push", None, Object, ctypes.c_char_p, ctypes.c_int
)

_nrn_double_ptr_pop = _bind("nrn_double_ptr_pop", ctypes.POINTER(ctypes.c_double))

# Reserved: nrn_symbol_push is bound but not currently used in myneuron.
_nrn_symbol_push = _bind("nrn_symbol_push", None, Symbol)

_nrn_section_ref = _bind("nrn_section_ref", None, Section)

_nrn_section_unref = _bind("nrn_section_unref", None, Section)

_nrn_cas = _bind("nrn_cas", Section)

# Reserved: nrn_prop_exists (check whether an Object has a given property)
# is bound but not currently used in myneuron.
_nrn_prop_exists = _bind("nrn_prop_exists", ctypes.c_bool, Object)

_nrn_distance = _bind(
    "nrn_distance", ctypes.c_double, Section, ctypes.c_double, Section, ctypes.c_double
)


# ---------------------------------------------------------------------------
# C-side staleness counters
#
# NEURON globals that change when topology or geometry changes:
#   diam_changed         cabcode.cpp:55   dirty flag; set on nseg/diam/L
#                                         mutation, cleared at finitialize
#   structure_change_cnt treeset.cpp:66   bumped when NEURON recomputes
#                                         structure (typically at finitialize)
#
# (diam_changed, structure_change_cnt) is the staleness signal for cached
# pointers: if it differs between cache time and read time, the pointer may
# be stale. It catches changes from any source (HOC, real NEURON, models that
# call nrn_section_new directly), not just myneuron's setters.
_diam_changed = ctypes.c_int.in_dll(libnrniv, "diam_changed")
_structure_change_cnt = ctypes.c_int.in_dll(libnrniv, "structure_change_cnt")

# WHY ctypes .value: ~94 ns/read vs numpy ~106 ns at 5M reads, and no numpy
# dependency on the hot path. Measurements:
# benchmarks/README.md#historical-implementation-measurements.


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
# parse.ypp's token values (VAR=263, FUNCTION=270, ...) are parser internals;
# neuronapi.cpp:236: "types are in parse.hpp and are not the same between
# versions, so we really should wrap." So the codes are read at import time
# from symbols whose kind is known:
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

    Dispatch compares against ``TYPES.VAR`` etc., never integer literals, so a
    NEURON that renumbers its token enum does not break it.
    """

    __slots__ = (
        # Top-level codes
        "STRING",  # top-level strdef (260) — also object STRDEF members
        "VAR",  # global double (t, dt, celsius); also USERPROPERTY (L, nseg)
        "BLTIN",  # math built-in (sin, cos, exp)
        "FUNCTION",  # HOC func/method returning double (270); distinct from FUN_BLTIN (280)
        "FUN_BLTIN",  # C-registered function returning double (finitialize, x3d)
        "PROCEDURE",  # HOC PROC
        "STRINGFUNC",  # function returning string (secname)
        "OBJECTFUNC",  # built-in function returning object (object_pushed)
        "OBJECTVAR",  # top-level objref (324) — `objref foo`
        "MECHANISM",  # density mechanism (hh, pas)
        "TEMPLATE",  # HOC class (IClamp, Vector)
        "RANGEVAR",  # range variable on segment (v, gnabar_hh)
        "RANGEOBJ",  # NMODL RANDOM range object (runtime-probed after MOD load)
        # Template / method codes
        "SECTION",  # template public section (cell.soma)
        "OBFUNCTION",  # template method returning object (HOC obfunc)
        "METHOD_OBFUNC",  # C-side method returning object (Vector.c, List.object)
        "METHOD_STRFUNC",  # method returning string (Vector.label)
        "METHOD_SECTIONREF",  # method returning Section (SectionRef.sec/parent/root)
        # Subtypes of VAR (hocdec.h:83-90 #defines, probed via known symbols)
        "USERINT",  # int scalar VAR (stoprun, hoc_ac_)
        "USERDOUBLE",  # double scalar VAR (t, dt, celsius)
        "USERPROPERTY",  # section-level property (L, nseg, Ra)
        "USERFLOAT",  # float scalar VAR — declared in hocdec.h:86 but not
        # known to be probable from a stock symbol table;
        # left None unless discovery succeeds.
        "_RAW",  # debug: full discovered map
    )

    def __init__(self):
        for name in self.__slots__:
            object.__setattr__(self, name, None)
        object.__setattr__(self, "_RAW", {})

    def discover(self):
        """Probe known top-level symbols to populate type codes."""
        # FUNCTION (270) is probed via Vector.size in discover_template_scoped.
        probes = {
            "VAR": b"t",
            "BLTIN": b"sin",
            "FUN_BLTIN": b"finitialize",
            "STRINGFUNC": b"secname",
            "OBJECTFUNC": b"object_pushed",  # built-in obfunc returning Object
            "MECHANISM": b"hh",
            "TEMPLATE": b"IClamp",
            "RANGEVAR": b"v",
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
            "USERINT": b"stoprun",  # int stop flag
            "USERDOUBLE": b"t",  # double simulation time
            "USERPROPERTY": b"nseg",  # section-level property
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

        # PROCEDURE code. The body must stay empty: `local x` would leave a
        # global `x` that broke user code naming something x (SectionRef.rename).
        hoc_exec("proc _mn_type_probe_proc() { }")
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
            raise RuntimeError(f"RANGEOBJ type code changed from {current} to {code}")
        object.__setattr__(self, "RANGEOBJ", code)
        self._RAW["RANGEOBJ"] = code

    # Names that are allowed to stay None after discovery (documented).
    # USERFLOAT (hocdec.h:86) has no obvious stock-symbol probe; no
    # dispatch path currently depends on it.
    _OPTIONAL = frozenset({"_RAW", "USERFLOAT", "RANGEOBJ"})

    def validate(self):
        """Return True if every required type code was discovered."""
        missing = [
            n
            for n in self.__slots__
            if n not in self._OPTIONAL and getattr(self, n) is None
        ]
        if missing:
            warnings.warn(
                "myneuron: could not discover type codes for: "
                + ", ".join(missing)
                + ". Some dispatch paths may not work."
            )
            return False
        return True

    def __repr__(self):
        pairs = [(n, getattr(self, n)) for n in self.__slots__ if n != "_RAW"]
        return "TypeCodes(" + ", ".join(f"{n}={v}" for n, v in pairs) + ")"


# Global singleton, populated by discover() at module import time below.
TYPES = TypeCodes()
TYPES.discover()
# Top-level codes are discovered here. Template-scoped ones are filled by
# __init__.py after the singleton exists; TYPES.SECTION etc. are None until then.
