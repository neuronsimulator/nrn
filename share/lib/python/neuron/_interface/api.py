"""ctypes bindings to libnrniv for NEURON 9+."""
import ctypes
import importlib.util
import itertools
import math
import os
import site
import sys
import threading
import warnings
from pathlib import Path

# The package, for what it defines after its submodules load (n, the model
# epoch, the callback stash): read as _PACKAGE.<name> at call time, which costs
# a fraction of a function-level import. Never copy a name another module rebinds.
_PACKAGE = sys.modules[__package__]

_LIBNRNIV_ENV = "NRN_LIBNRNIV"
_LIBMODLREG_ENV = "NRN_LIBMODLREG"

# Imported as neuron._interface, this package lives inside NEURON's own Python
# package: it must use that build's libraries (never another installed
# neuron) and reproduce the startup the legacy PyInit_hoc performed.
_VENDORED = __package__ != "myneuron"
# Python-visible module of HOC classes. neuron.hoc.Vector.__module__ is "hoc".
HOC_CLASS_MODULE = "hoc" if _VENDORED else "myneuron"
NRN_CLASS_MODULE = "nrn" if _VENDORED else "myneuron"  # nrn.Section, nrn.Segment
_HERE = Path(__file__).resolve().parent
_LIBNRNIV_NAMES = ("libnrniv.so", "libnrniv.dylib", "nrniv.dll", "libnrniv.dll")


def _vendored_lib_dirs():
    # Wheel: neuron/.data/lib, on Windows .data/bin. Build tree and CMake install:
    # <prefix>/lib/python/neuron/_interface -> <prefix>/lib. Windows: <prefix>/bin.
    parents = list(_HERE.parents)  # a shallow --target install has only three
    return [
        parents[0] / ".data" / "lib",
        parents[0] / ".data" / "bin",
        parents[2],
        *(prefix / "bin" for prefix in parents[3:4]),
    ]


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


def _nrniv_candidates_from_neuron_wheel(own_only=False):
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
    if not own_only:
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


def _load_cdll_or_raise(*, label, envvar, candidates, mode=None, loader=ctypes.PyDLL):
    """Load a shared library.

    Tries ``$envvar``, then ``candidates``, then ``ctypes.util.find_library``.
    Raises ImportError listing the paths tried. PyDLL: see the
    comment above libnrniv's load.
    """
    tried = []
    env_path = os.environ.get(envvar)
    if env_path:
        # An override that does not load is an error, not a reason to take
        # another library.
        try:
            if mode is None:
                return loader(env_path)
            return loader(env_path, mode=mode)
        except OSError as exc:
            raise ImportError(f"{envvar}={env_path!r} could not be loaded: {exc}")

    for p in _iter_existing(candidates):
        tried.append(p)
        try:
            if mode is None:
                return loader(p)
            return loader(p, mode=mode)
        except OSError:
            continue

    # As a last resort try find_library (often returns a soname)
    from ctypes.util import find_library

    libname = find_library(label)
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

# libnrniv leaves modl_reg to the program. RTLD_GLOBAL: libnrniv finds it. On
# Windows it does not call it.
_rtld_global = getattr(ctypes, "RTLD_GLOBAL", None)
libmodlreg = (
    None
    if sys.platform == "win32"
    else _load_cdll_or_raise(
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
)

# libnrniv must load as PyDLL so the GIL stays held around every call: NEURON
# has one interpreter per process, and nothing in it is thread safe. With the
# legacy module in the process, CDLL crashed on every mod file with USEION
# (its nrnpy_reg_mech calls Python without taking the GIL).
#
# On macOS, RTLD_GLOBAL, as the legacy module gets it there (dlopen's default).
# With RTLD_LOCAL a mechanism library from nrnivmodl binds its own weak copy of
# typeid(Point_process*), and on x86_64 macOS net_send and every other
# Point_process handle conversion threw "cannot be converted to
# data_handle<Point_process>" (gap-76). Not on Linux: libnrniv carries its own
# readline and termcap symbols, and a global libnrniv made Python's readline
# bind to them (write_history_file segfaulted). Linux's libstdc++ compares
# type_info by name, so the separate copy does no harm there, and headless
# legacy NEURON keeps libnrniv local too.
def _libnrniv_dlopen_mode(platform=sys.platform):
    return _rtld_global if platform == "darwin" else None


libnrniv = _load_cdll_or_raise(
    label="nrniv",
    envvar=_LIBNRNIV_ENV,
    candidates=_nrniv_candidates_from_neuron_wheel(),
    mode=_libnrniv_dlopen_mode(),
)


_own_nrniv = {
    str(path.resolve()) for path in _nrniv_candidates_from_neuron_wheel(own_only=True)
}
if str(Path(libnrniv._name).resolve()) not in _own_nrniv:
    warnings.warn(
        f"loaded libnrniv from {libnrniv._name!r}, not from the neuron package "
        f"({_LIBNRNIV_ENV} or a fallback search); it must be the core this "
        "release pins",
        RuntimeWarning,
        stacklevel=2,
    )


def _bind(name, restype, *argtypes):
    """Return libnrniv's function *name* with its C signature set."""
    function = getattr(libnrniv, name)
    function.argtypes = list(argtypes)
    function.restype = restype
    return function


# KSChan's count of its symbol renames and frees, or None on a core without
# it. The symbol caches kept by name follow it (sections.forget_moved_symbols).
try:
    _kschan_symbol_changes = _bind("nrn_kschan_symbol_changes", ctypes.c_size_t)
except AttributeError:
    _kschan_symbol_changes = None


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
        (
            f"{libnrniv._name!r} lacks the C API this interface needs"
            if _VENDORED
            else "myneuron requires neuron-nightly with the completed NEURON C API. "
            "Install or upgrade it with `pip install --upgrade neuron-nightly`. "
            f"Loaded {libnrniv._name!r}"
        )
        + f"; missing symbols: {', '.join(_missing_nightly_api)}"
    )

# Error buffer for nothrow C API calls
_ERR_BUF_SIZE = 1024
# _ERR_BUF() is create_string_buffer(_ERR_BUF_SIZE) without the function call.
_ERR_BUF = ctypes.c_char * _ERR_BUF_SIZE


def _check_nrn_error(ret, err_buf):
    """Raise the pending Python cause, otherwise the nothrow C API error."""
    if ret != 0:
        # Callers have already rolled back arguments restored by nothrow.
        # Public component hooks abort in core, without a return slot to pop.
        raise_callback_error()
        msg = err_buf.value.decode("utf-8", errors="replace")
        raise RuntimeError(
            f"hocobj_call error: {msg}" if msg else "hocobj_call error (no message)"
        )


#
# initialize NEURON
#
# argv:
#   "-nogui"    Opt-in via NRN_NOGUI=1. Real NEURON's PyInit_hoc never
#               passes it, and it forces Graph.view_count() to 0 even on a
#               live display. The test suite opts in so PlotShape/plotting
#               tests are order-independent without a DISPLAY. See
#               docs/ARCHITECTURE.md#nrn_init.
#   "-nopython" Python is running: NEURON must not look for one to start.

_argv_list = [b"NEURON"]
if _VENDORED:
    from ._startup import startup_options

    _argv_list += [arg.encode() for arg in startup_options()]
if os.environ.get("NRN_NOGUI", "0") == "1":
    _argv_list.append(b"-nogui")
_argv_list.append(b"-nopython")
_argc = len(_argv_list)
argv = (ctypes.c_char_p * (_argc + 1))(*_argv_list, None)

nrn_init = _bind(
    "nrn_init", ctypes.c_int, ctypes.c_int, ctypes.POINTER(ctypes.c_char_p)
)

# NEURON calls Python itself; see "Python objects in HOC" below.
CALLS_PYTHON = hasattr(libnrniv, "nrn_python_hook_set")
if not CALLS_PYTHON:
    try:
        from . import _released_base
    except ModuleNotFoundError:
        raise ImportError(
            f"{libnrniv._name} is not the NEURON this package belongs to: it has no "
            "nrn_python_hook_set"
        ) from None

# Does nothing under nrniv, where NEURON is already running.
ret = nrn_init(_argc, argv)
if ret:
    raise RuntimeError("nrn_init failed with return code %d" % ret)


# --- Python objects in HOC ---
#
# NEURON calls Python itself (nrn_python_hook_set and the functions next to it),
# in every launch mode. Cores without them, the released wheels, are for
# standalone development: _released_core.py gives them ctypes callbacks, and
# _released_base.py the rest. They are not exported to NEURON.

# The model epoch. Python-side invalidation only clears the wrapper that made a
# change, but HOC-side changes and other wrappers of the same Section need a
# shared signal. NEURON ends the epoch (puts None) when it calls Python, whose
# code may find a model that HOC has edited, and HOC calls made from Python end
# it, except the read-only ones (utils.READ_ONLY_*). A cache about the model
# keeps the epoch it was filled in (_model_epoch, which starts one if need be)
# and is good while that is EPOCH[0]. structure_change_cnt is not sufficient: it
# does not move on insert, and some interior deletes evade allsec()'s head-link
# check. Over-invalidation (a harmless `n("print x")` also ends the epoch) is
# intentional.
# The core's fast path holds this list (never rebind it, only set EPOCH[0]).
EPOCH = [1]
_epochs = itertools.count(2)


def _model_epoch():
    epoch = EPOCH[0]
    if epoch is None:
        epoch = EPOCH[0] = next(_epochs)
    return epoch


def _bump_model_epoch():
    EPOCH[0] = None


if CALLS_PYTHON:
    nrn_python_hook_set = _bind(
        "nrn_python_hook_set", ctypes.c_int, ctypes.c_char_p, ctypes.py_object
    )
    # A PythonObject with one reference, the caller's.
    _new_python_object = _bind(
        "nrn_python_object_new", ctypes.c_void_p, ctypes.py_object
    )
    # The Python value of a HOC object.
    _python_object_get = _bind(
        "nrn_python_object_get", ctypes.py_object, ctypes.c_void_p
    )
    _python_error_take = _bind("nrn_python_error_take", ctypes.py_object)
    # The base classes of the wrappers: core reads and makes instances itself.
    _python_type = _bind("nrn_python_type", ctypes.py_object, ctypes.c_char_p)
    OBJECT_BASE = _python_type(b"object")
    SECTION_BASE = _python_type(b"section")
    SEGMENT_BASE = _python_type(b"segment")
    # Fast-path base classes (prototype): None on a core without them.
    MECHANISM_BASE = _python_type(b"mechanism")
    VECTOR_BASE = _python_type(b"vector")
    # NEURON is at its top level whenever it runs Python, so this is always HOC's
    # own top-level symbol.
    _top_level_symbol = _bind("nrn_symbol", ctypes.c_void_p, ctypes.c_char_p)

    def _call_at_top_level(target, args):
        return target(*args)

    # HOC's string assignment; None frees the string.
    _str_assign = _bind(
        "nrn_str_assign", None, ctypes.POINTER(ctypes.c_void_p), ctypes.c_char_p
    )
else:
    nrn_python_hook_set = None
    OBJECT_BASE = _released_base.OBJECT_BASE
    SECTION_BASE = _released_base.SECTION_BASE
    SEGMENT_BASE = _released_base.SEGMENT_BASE
    MECHANISM_BASE = VECTOR_BASE = None
    _call_at_top_level = _released_base.call_at_top_level
    _top_level_symbol = _released_base.top_level_symbol
    _str_assign = _released_base.str_assign

    def _new_python_object(value):
        from ._released_core import new_python_object

        return new_python_object(value)

    def _python_object_get(obj):
        from ._released_core import python_object_get

        return python_object_get(obj)


def owns_python_objects():
    """False in a process where the legacy neuron module came first (tests)."""
    if CALLS_PYTHON:
        return True
    from ._released_core import _cfunctype_methods_installed

    return _cfunctype_methods_installed


# Exceptions of ctypes callbacks, which wait for the call that ran the callback
# to return. Stays empty if NEURON calls Python itself.
PENDING_CALLBACKS = []


def raise_callback_error():
    """Raise the Python exception that caused the HOC error, if one did."""
    if CALLS_PYTHON:
        exc = _python_error_take()
        if exc is not None:
            try:
                raise exc
            finally:
                # Or its traceback and this frame keep each other, and with them
                # what the callback's frames refer to (sections) until a garbage
                # collection, which can come while core's data is frozen.
                del exc
        return
    from ._released_core import raise_pending_callback_exc

    raise_pending_callback_exc()


# nrn#3889 (NEURON 9.1): every dimension of an array symbol. Optional; without
# it objref arrays are treated as one-dimensional.
_nrn_symbol_array_dims = getattr(libnrniv, "nrn_symbol_array_dims", None)
if _nrn_symbol_array_dims is not None:
    _nrn_symbol_array_dims.argtypes = [
        ctypes.c_void_p,
        ctypes.POINTER(ctypes.c_int),
        ctypes.c_int,
    ]
    _nrn_symbol_array_dims.restype = ctypes.c_int


# nrn#3900 (pending): a pointer to one element of an object's array property,
# with every index (Matrix.x takes two). Optional; without it an element of
# Matrix.x cannot be passed as a pointer.
_nrn_property_element_ptr = getattr(libnrniv, "nrn_property_element_ptr", None)
if _nrn_property_element_ptr is not None:
    _nrn_property_element_ptr.argtypes = [
        ctypes.c_void_p,
        ctypes.c_char_p,
        ctypes.POINTER(ctypes.c_int),
        ctypes.c_int,
        ctypes.POINTER(ctypes.POINTER(ctypes.c_double)),
        ctypes.c_char_p,
        ctypes.c_size_t,
    ]
    _nrn_property_element_ptr.restype = ctypes.c_int

# nrn#3888 (pending): the Object bound to an objref member of a HOC template
# object, public or not, as NEURON's Python module reads it (borrowed; NULL
# when nil or not an objref member). Optional; without it members are read
# through HOC, which allows only public ones.
_nrn_property_object_get = getattr(libnrniv, "nrn_property_object_get", None)
if _nrn_property_object_get is not None:
    _nrn_property_object_get.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
    _nrn_property_object_get.restype = ctypes.c_void_p

_Dims8 = ctypes.c_int * 8


def symbol_array_dims(sym):
    """All dimensions of array symbol ``sym``, outermost first; only the first
    on a core without nrn_symbol_array_dims."""
    if _nrn_symbol_array_dims is not None:
        dims = _Dims8()
        ndim = _nrn_symbol_array_dims(sym, dims, 8)
        if ndim > 0:
            return tuple(dims[: min(ndim, 8)])
    return (int(_nrn_symbol_array_length(sym)),)


def _unwrap_pythonobject(obj):
    """What a PythonObject holds; None for a nil object."""
    if not obj:
        return None
    if _nrn_class_name(obj) != b"PythonObject":
        raise TypeError("expected a HOC PythonObject")
    return _python_object_get(obj)


def _owned_pythonobject_to_python(obj):
    """Consume an owned PythonObject reference."""
    try:
        return _unwrap_pythonobject(obj)
    finally:
        if obj:
            _nrn_object_unref(obj)


def _owned_hoc_object_to_python(obj):
    # Component RHS/arguments and public object returns must share ownership
    # cleanup, especially when PythonObject conversion raises.

    return _PACKAGE.utils._wrap_owned_object(obj)


# Private prototypes (not ctypes.pythonapi's shared ones). PySequence_Check
# tells a numpy scalar, which is indexable, from an array; no Python-level
# test does.
_PyNumber_Check = ctypes.PYFUNCTYPE(ctypes.c_int, ctypes.py_object)(
    ("PyNumber_Check", ctypes.pythonapi)
)
_PySequence_Check = ctypes.PYFUNCTYPE(ctypes.c_int, ctypes.py_object)(
    ("PySequence_Check", ctypes.pythonapi)
)
_Py_TPFLAGS_HEAPTYPE = 1 << 9


def _is_hoc_number(value):
    """Whether HOC takes *value* as a number, as NEURON does.

    That is what PyNumber_Check accepts, unless it is a sequence or float()
    fails: a numpy scalar (numpy.bool_ too) or a Decimal is a number; complex, a
    numpy array (even 0-d), str and None are not. Nor, on a released core, is an
    instance of a class defined in Python: NEURON 9.0 rejects every type with a
    sequence methods table, which each such class has. Later ones ask
    PySequence_Check and accept a class with __float__ (Fraction too).
    """
    if not CALLS_PYTHON and type(value).__flags__ & _Py_TPFLAGS_HEAPTYPE:
        return False
    if not _PyNumber_Check(value) or _PySequence_Check(value):
        return False
    try:
        float(value)
    except Exception:
        return False
    return True


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
                _bump_model_epoch()
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

if CALLS_PYTHON:
    _nrn_hoc_call_nothrow = _bind("nrn_hoc_call_nothrow", ctypes.c_int, ctypes.c_char_p)
else:
    _nrn_hoc_call_nothrow = _released_base.hoc_call_nothrow


# One lock for every multi-step HOC interaction. A call from Python is several
# C calls (push the arguments, call, pop the result) on NEURON's single
# interpreter stack and parser buffer, and the GIL can switch threads between
# them; NEURON's own module makes each call one C call under the GIL. The GUI
# event thread (doNotify every 0.1 s) and any user thread would otherwise pop
# another thread's values or overwrite its statement mid-parse (gap-72).
# Reentrant: HOC calls back into Python, which calls HOC again.
HOC_LOCK = threading.RLock()


# Nesting of hoc_locked calls: 1 in a call from user code, more in a call a
# Python callback makes from inside HOC.
_hoc_call_depth = 0


def hoc_call_depth():
    return _hoc_call_depth


_launched_by_python = [None]


def in_hoc_call():
    """True in a callback core makes on this thread. Always under nrniv, where
    HOC calls Python without a call from here."""
    if _launched_by_python[0] is None:
        _launched_by_python[0] = _PACKAGE._n.nrnversion(9) == "2"
    if not _launched_by_python[0]:
        return True
    if not _hoc_call_depth or not HOC_LOCK.acquire(blocking=False):
        return False  # no call, or another thread's
    HOC_LOCK.release()
    return True


def hoc_locked(func):
    """Run ``func`` holding HOC_LOCK."""

    def locked(*args, **kwargs):
        global _hoc_call_depth
        with HOC_LOCK:
            _hoc_call_depth += 1
            try:
                return func(*args, **kwargs)
            finally:
                _hoc_call_depth -= 1

    locked.__name__ = func.__name__
    locked.__qualname__ = func.__qualname__
    locked.__doc__ = func.__doc__
    locked.__wrapped__ = func
    return locked


@hoc_locked
def _nrn_hoc_call(cmd):
    return _nrn_hoc_call_nothrow(cmd)


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

# The Object a top-level objref holds, borrowed (no reference taken); NULL when
# the objref is nil or the symbol is not a top-level objref.
_nrn_symbol_object_get = _bind("nrn_symbol_object_get", Object, Symbol)

# A top-level strdef's value; NULL if the symbol is not a strdef.
_nrn_symbol_str_get = _bind("nrn_symbol_str_get", ctypes.c_char_p, Symbol)

_nrn_register_function = _bind(
    "nrn_register_function", None, ctypes.c_void_p, ctypes.c_char_p, ctypes.c_int
)

_nrn_gargstr = _bind("nrn_gargstr", ctypes.c_char_p, ctypes.c_int)
_nrn_objgetarg = _bind("nrn_objgetarg", ctypes.POINTER(Object), ctypes.c_int)
_nrn_is_object_arg = _bind("nrn_is_object_arg", ctypes.c_bool, ctypes.c_int)
_nrn_is_str_arg = _bind("nrn_is_str_arg", ctypes.c_bool, ctypes.c_int)

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


def _nrn_object_new(sym, narg, result=None):
    """Construct a HOC object without allowing C++ exceptions across ctypes.

    A caller that must release the object when an interrupt arrives as this
    returns passes the ``Object`` out-parameter and reads it afterwards.
    """
    result = Object() if result is None else result
    err = _ERR_BUF()
    rc = _nrn_object_new_nothrow(sym, narg, ctypes.byref(result), err, _ERR_BUF_SIZE)
    if rc:
        msg = err.value.decode("utf-8", errors="replace")
        raise RuntimeError(
            f"hocobj_call error: {msg}" if msg else "HOC object construction failed"
        )
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


def hoc_variable(name, ctype):
    """A core variable that HOC also knows, as a ctypes object."""
    pointer = _nrn_symbol_dataptr(_nrn_symbol(name.encode()))
    return ctypes.cast(pointer, ctypes.POINTER(ctype)).contents


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
# The legacy module flushed it after every function/method call and when a
# wrapper was released. Without the flush the deferred object outlives its
# owner: a NetCon destroyed after its source double-frees in PreSyn::~PreSyn.
# A core that calls Python itself flushes it in the C API's calls.
# The result of a method that returns a number: -1 if it fails, else 0 for a
# double, 1 for an int, 2 for a bool. NEURON's integer and boolean methods of
# built-in classes say what they return.
if CALLS_PYTHON:
    _hoc_unref_defer = None
    _nrn_method_call_number = _bind(
        "nrn_method_call_number",
        ctypes.c_int,
        Object,
        Symbol,
        ctypes.c_int,
        ctypes.POINTER(ctypes.c_double),
        ctypes.c_char_p,
        ctypes.c_size_t,
    )
    # A StringFunctions alias of an object: its symbol, or NULL. Runs no HOC.
    _nrn_object_alias = _bind(
        "nrn_object_alias", ctypes.c_void_p, Object, ctypes.c_char_p
    )
    # Returns the value before. A HOC error on several ranks calls MPI_Abort if
    # it is not 0.
    _nrn_mpiabort_on_error = _bind("nrn_mpiabort_on_error", ctypes.c_int, ctypes.c_int)
    _structure_change_count = _bind("nrn_structure_change_count", ctypes.c_int)
    _mechanism_type_count = _bind("nrn_mechanism_type_count", ctypes.c_int)
    _section_list_version = None
else:
    _hoc_unref_defer = _released_base.hoc_unref_defer
    _nrn_method_call_number = _released_base.method_call_number
    _nrn_object_alias = _released_base.object_alias
    _nrn_mpiabort_on_error = _released_base.mpiabort_on_error
    _structure_change_count = _released_base.structure_change_count
    _mechanism_type_count = _released_base.mechanism_type_count
    _section_list_version = _released_base.section_list_version


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


def _object_ptr_push(obj_ref, count):
    count[0] += 1  # see NrnRef._push
    _nrn_object_ptr_push(obj_ref)


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

# A node's mechanisms in the order of its property list, the order NEURON's
# `for mech in seg` follows. Optional: the pinned core lacks it; without it a
# segment yields its mechanisms in registration order.
_nrn_segment_mechanisms = getattr(libnrniv, "nrn_segment_mechanisms", None)
if _nrn_segment_mechanisms is not None:
    _nrn_segment_mechanisms.argtypes = [
        Section,
        ctypes.c_double,
        ctypes.POINTER(Symbol),
        ctypes.c_int,
    ]
    _nrn_segment_mechanisms.restype = ctypes.c_int

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


def _not_a_number(error_type, text):
    """The error for a value that is no number: NEURON raises ValueError("bad
    value") for a range variable, and TypeError with Python's text for a
    point-process property, so the caller picks the type."""
    return error_type(f"bad value: {text}" if error_type is ValueError else text)


def _refused_double(exc, error_type):
    """_not_a_number for a value ctypes refused as a double argument."""
    return _not_a_number(error_type, exc.args[0].rpartition(": ")[2])


def _as_double(value):
    """``value`` as a double; a str, None or other non-number is a TypeError
    ("must be real number, not str"), as for a NEURON global."""
    return ctypes.c_double(value).value


def _to_double(value, error_type):
    """``value`` as a double, any failure reported by _not_a_number.

    Conversion can run user code (``__float__``) that deletes the section or
    uninserts the mechanism the write is about to use. Callers convert before
    their liveness checks, and skip this call for a plain float or int, which
    cannot run any.
    """
    try:
        return ctypes.c_double(value).value
    except Exception as exc:
        raise _not_a_number(error_type, str(exc)) from None


def _property_set_checked(obj, name, value):
    # Writes have no NaN sentinel to gate on, so the validity probe runs on
    # every write. Real NEURON raises AttributeError("POINTER is NULL") for
    # an attribute write through an unset/opaque POINTER; a silent write-
    # through corrupts the invalid handle's target.
    if not _nrn_property_data_handle_is_valid(obj, name, 0):
        raise AttributeError("POINTER is NULL")
    try:
        _nrn_property_set(obj, name, value)
    except ctypes.ArgumentError as exc:
        raise _refused_double(exc, TypeError) from None


def _property_array_set_checked(obj, name, index, value):
    if not _nrn_property_data_handle_is_valid(obj, name, index):
        raise AttributeError("POINTER is NULL")
    try:
        _nrn_property_array_set(obj, name, index, value)
    except ctypes.ArgumentError as exc:
        raise _refused_double(exc, TypeError) from None


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


def _property_push_checked(obj, name, count):
    # Pushing an invalid handle hands downstream consumers (Vector.record)
    # a pointer that SIGSEGVs at finitialize; refuse it like real refuses
    # the record argument. *count*: see NrnRef._push.
    if not _nrn_property_data_handle_is_valid(obj, name, 0):
        raise ValueError("Invalid data_handle")
    count[0] += 1
    _nrn_property_push(obj, name)


def _property_array_push_checked(obj, name, index, count):
    if not _nrn_property_data_handle_is_valid(obj, name, index):
        raise ValueError("Invalid data_handle")
    count[0] += 1
    _nrn_property_array_push(obj, name, index)


_nrn_section_is_active = _bind("nrn_section_is_active", ctypes.c_bool, Section)

_nrn_rangevar_set = _bind(
    "nrn_rangevar_set", None, Symbol, Section, ctypes.c_double, ctypes.c_double
)

# Push a pointer to element `index` of a range variable array (nrn#3905,
# merged after the pinned core). nrn_rangevar_push reads the index from the HOC
# stack behind an array-dimension marker the C API cannot push, and throws
# without it. Optional; without it an element reference cannot be passed as a
# pointer.
# The text NEURON prints for a reference, e.g.
# data_handle<double>{Node::field::Voltage row=0/2 val=-65} (gap-65). Optional:
# merged after the pinned core.
_nrn_rangevar_handle_str = getattr(libnrniv, "nrn_rangevar_handle_str", None)
if _nrn_rangevar_handle_str is not None:
    _nrn_rangevar_handle_str.argtypes = [
        Symbol,
        Section,
        ctypes.c_double,
        ctypes.c_int,
        ctypes.c_char_p,
        ctypes.c_size_t,
    ]
    _nrn_rangevar_handle_str.restype = ctypes.c_int

_nrn_rangevar_push_nothrow = getattr(libnrniv, "nrn_rangevar_push_nothrow", None)
if _nrn_rangevar_push_nothrow is not None:
    _nrn_rangevar_push_nothrow.argtypes = [
        Symbol,
        Section,
        ctypes.c_double,
        ctypes.c_int,
        ctypes.c_char_p,
        ctypes.c_size_t,
    ]
    _nrn_rangevar_push_nothrow.restype = ctypes.c_int

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
_diam_changed = hoc_variable("diam_changed", ctypes.c_int)

# WHY ctypes .value: ~94 ns/read vs numpy ~106 ns at 5M reads, and no numpy
# dependency on the hot path. Measurements:
# benchmarks/README.md#historical-implementation-measurements.


def get_topology_version():
    """Return an opaque tuple that changes when topology/diam changes.

    Suitable for staleness checks on cached range-variable pointers.
    Compare two return values with `==` / `!=`; do not interpret the
    components.
    """
    return (_diam_changed.value, _structure_change_count())


# The model epoch in which the section tree was last known to be set up. While it
# lasts no end of a section needs _ensure_topology.
_TREE_SET_UP = [0]
if CALLS_PYTHON:
    _nrn_rangevar_get_nothrow = _bind(
        "nrn_rangevar_get_nothrow",
        ctypes.c_int,
        Symbol,
        Section,
        ctypes.c_double,
        ctypes.c_int,
        ctypes.POINTER(ctypes.c_double),
        ctypes.c_char_p,
        ctypes.c_size_t,
    )

    _nrn_tree_setup = _bind("nrn_tree_setup", ctypes.c_char_p, Section, ctypes.c_double)

    def _ensure_topology(sec_ptr, x):
        """Call it before a raw C call that resolves the end x of a section: that
        sets the tree up (node_exact), which is a HOC error if a loop exists, and
        RuntimeError, with NEURON's message, here. A deleted section is left to the
        caller's liveness check."""
        if _nrn_section_is_active(sec_ptr):
            error = _nrn_tree_setup(sec_ptr, x)
            if error is None:
                _TREE_SET_UP[0] = _model_epoch()
            elif error:
                raise RuntimeError(error.decode("utf-8", errors="replace"))

    _IMEM = [_nrn_symbol(b"i_membrane_"), False, 0]  # and whether it exists, as of

    def _fast_imem_on(sec_ptr):
        if _IMEM[2] != EPOCH[0]:
            epoch = _model_epoch()
            failed = _nrn_rangevar_get_nothrow(_IMEM[0], sec_ptr, 0.5, 0, None, None, 0)
            _IMEM[1:] = not failed, epoch
        return _IMEM[1]

else:
    _ensure_topology = _released_base.ensure_topology
    _fast_imem_on = _released_base.fast_imem_on


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
        # Subtype of TEMPLATE (hocdec.h CPLUSOBJECT): a class implemented in
        # C++, built in or from a MOD file (Vector, List, IClamp, ExpSyn).
        # A HOC template has subtype 0.
        "CPLUSOBJECT",
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
            "CPLUSOBJECT": b"Vector",  # a C++ class
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

        # SECTION follows the `create` keyword (parse.ypp: %token SECTIONKEYWORD
        # SECTION). No symbol has this type until a section exists, and a section
        # would mark the model changed: `import neuron` can follow finitialize (a
        # HOC run through CoreNEURON: "internal structures ... are out of date").
        code = int(_nrn_symbol_type(_nrn_symbol(b"create"))) + 1
        object.__setattr__(self, "SECTION", code)
        self._RAW["SECTION"] = code

        # Obfunc method, read from the template's symbol table.
        if not _nrn_symbol(b"_MNTypeProbe"):  # a re-import: HOC keeps it
            hoc_exec(
                "begintemplate _MNTypeProbe\n"
                "public get_obj\n"
                "obfunc get_obj() { return new Vector(1) }\n"
                "endtemplate _MNTypeProbe\n"
            )
        self._record_class_member(b"_MNTypeProbe", b"get_obj", "OBFUNCTION")

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

    def _record_class_member(self, class_name, member, attr):
        """Record the type of a class's member, found without an instance."""
        iterator = _nrn_symbol_table_iterator_new(
            _nrn_symbol_table(_nrn_symbol(class_name))
        )
        try:
            while not _nrn_symbol_table_iterator_done(iterator):
                sym = _nrn_symbol_table_iterator_next(iterator)
                if _nrn_symbol_name(sym) == member:
                    code = int(_nrn_symbol_type(sym))
                    object.__setattr__(self, attr, code)
                    self._RAW[attr] = code
                    break
        finally:
            _nrn_symbol_table_iterator_free(iterator)

    def probe_section_ref(self):
        """Probe METHOD_SECTIONREF. A SectionRef instance would need a section."""
        self._record_class_member(b"SectionRef", b"sec", "METHOD_SECTIONREF")

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
