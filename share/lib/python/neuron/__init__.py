"""

neuron
======

For empirically-based simulations of neurons and networks of neurons in Python.

This is the top-level module of the official python interface to
the NEURON simulation environment (https://www.neuronsimulator.org).

Documentation is available in the docstrings.

For a list of available names, try dir(neuron).

Example:

$ ipython
In [1]: import neuron
NEURON -- VERSION 6.2 2008-08-22
Duke, Yale, and the BlueBrain Project -- Copyright 1984-2007
See http://neuron.yale.edu/credits.html


In [2]: neuron.h ?






Important names and sub-packages
---------------------

For help on these useful functions, see their docstrings:

  load_mechanisms


neuron.n

   The top-level NEURON interface. Synonymous with neuron.h
   except the type and __repr__ are different.

   Execute Hoc commands by calling n with a string argument:

   >>> n('objref myobj')
   >>> n('myobj = new Vector(10)')

   All Hoc defined variables are accessible by attribute access to n.

   Example:

   >>> print(n.myobj.x[9])

   Hoc Classes are also defined, for example:

   >>> v = n.Vector([1,2,3])
   >>> soma = n.Section("soma")

   More help is available for the respective class by looking in the object docstring:

   >>> help(n.Vector)



neuron.gui

   Import this package if you are using NEURON as an extension to Python,
   and you would like to use the NEURON GUI.

   If you are using NEURON with embedded python, "nrniv -python",
   use rather "nrngui -python" if you would like to use the NEURON GUI.

$Id: __init__.py,v 1.1 2008/05/26 11:39:44 hines Exp hines $

"""

## With Python launched under Linux, shared libraries are apparently imported
## using RTLD_LOCAL. For --with-paranrn=dynamic, this caused a failure when
## libnrnmpi.so is dynamically loaded because nrnmpi_myid (and other global
## variables in src/nrnmpi/nrnmpi_def_cinc) were not resolved --- even though
## all those variables are defined in src/oc/nrnmpi_dynam.c and that
## does a dlopen("libnrnmpi.so", RTLD_NOW | RTLD_GLOBAL) .
## In this case setting the dlopenflags below fixes the problem. But it
## seems that DLFCN is often not available.
## This situation is conceptually puzzling because there
## never seems to be a problem dynamically loading libnrnmech.so, though it
## obviously makes use of many names in the rest of NEURON. Anyway,
## we make the following available in case it is ever needed at least to
## verify that some import problem is traceable to this issue.
## The problem can be resolved in two ways. 1) see src/nrnmpi/nrnmpi_dynam.c
## which promotes liboc.so and libnrniv.so to RTLD_GLOBAL  (commented out).
## 2) The better way of specifying those libraries to libnrnmpi_la_LIBADD
## in src/nrnmpi/Makefile.am . This latter also explains why libnrnmech.so
## does not have this problem.
# try:
#  import sys
#  import DLFCN
#  sys.setdlopenflags(DLFCN.RTLD_NOW | DLFCN.RTLD_GLOBAL)
# except:
#  pass

import atexit as _atexit
import ctypes
import sys
import os
import sysconfig
import warnings
import weakref

from ._config_params import (
    supported_python_versions,
    limited_api,
    python_dynamic,
    mechanism_prefix,
    mechanism_suffix,
)

current_version = "{}.{}".format(*sys.version_info[:2])
if sys.implementation.name != "cpython":
    raise ImportError(
        "This NEURON installation supports CPython only (this interpreter is {}).".format(
            sys.implementation.name
        )
    )
# Cross-version import is safe only when libnrniv is not linked to one libpython.
if limited_api and python_dynamic:
    if sys.version_info[0] != 3 or sys.version_info[1] < 12:
        raise ImportError(
            "Python {} is not supported by this NEURON installation (requires CPython 3.12 "
            "or a newer 3.x minor, with the GIL).".format(current_version)
        )
elif current_version not in supported_python_versions:
    raise ImportError(
        "Python {} is not supported by this NEURON installation (supported: {}). "
        "Either re-build NEURON with support for this version, use a supported version "
        "of Python, or try using nrniv -python so that NEURON can suggest a compatible "
        "version for you.".format(current_version, " ".join(supported_python_versions))
    )
# Py_GIL_DISABLED is the interpreter build. 1 and "1" are free-threaded.
# 0, "0", and None are a GIL build: the string "0" is truthy. Enabling the
# GIL at runtime does not make a free-threaded build acceptable.
if sysconfig.get_config_var("Py_GIL_DISABLED") in (1, "1"):
    raise ImportError(
        "This NEURON installation does not support a free-threaded CPython build "
        "(Py_GIL_DISABLED)."
    )

# With pip we need to rewrite the NEURONHOME
nrn_path = os.path.abspath(os.path.join(os.path.dirname(__file__), ".data/share/nrn"))
if os.path.isdir(nrn_path):
    os.environ["NEURONHOME"] = nrn_path

# Python 3.8+ on Windows does not search PATH for DLL dependencies of a .pyd
# (https://docs.python.org/3/library/os.html#os.add_dll_directory). nrniv.dll is
# in the wheel's .data/bin, cmake prefix/bin, or setup.exe NEURONHOME/bin.
# Keep the cookies: discarding the return value calls RemoveDllDirectory.
_nt_dll_dir_handles = []
if os.name == "nt" and hasattr(os, "add_dll_directory"):
    _dll_dirs = [
        os.path.join(os.path.dirname(os.path.abspath(__file__)), ".data", "bin"),
        os.path.join(os.path.dirname(os.path.abspath(__file__)), ".data", "lib"),
    ]
    _home = os.environ.get("NEURONHOME")
    if _home:
        _dll_dirs.append(os.path.join(os.path.abspath(_home), "bin"))
        _dll_dirs.append(os.path.abspath(os.path.join(_home, "..", "..", "bin")))
    _seen = set()
    for _dll_dir in _dll_dirs:
        _dll_dir = os.path.normpath(_dll_dir)
        if _dll_dir in _seen or not os.path.isdir(_dll_dir):
            continue
        _seen.add(_dll_dir)
        _nt_dll_dir_handles.append(os.add_dll_directory(_dll_dir))
        os.environ["PATH"] = _dll_dir + os.pathsep + os.environ.get("PATH", "")
    del _dll_dirs, _home, _seen, _dll_dir

# On OSX, dlopen might fail if not using full library path
try:
    from sys import platform

    if platform == "darwin":
        from ctypes.util import find_library

        mpi_library_path = find_library("mpi")
        if mpi_library_path and "MPI_LIB_NRN_PATH" not in os.environ:
            os.environ["MPI_LIB_NRN_PATH"] = mpi_library_path
except:
    pass

# `from neuron import h` is served by the pure-Python interface in _interface,
# which drives libnrniv through NEURON's C API. It applies the startup options
# (neuron_options, NEURON_MODULE_OPTIONS, the <arch>/libnrnmech auto-load). Under
# nrniv NEURON is already running, and the interface attaches to it.
from . import _interface
from ._interface import _facades

h = _interface.h
n = _interface.n
hoc, nrn, _neuron_section = _facades.install(_interface)
embedded = h.nrnversion(9) == "1"  # nrniv started Python

# Python 3.13+'s pyrepl (UnixConsole.prepare) clears OPOST, so the kernel
# no longer maps NL to CRNL. GUI callbacks run on a background thread
# while that mode is in effect, and print() then moves down without
# returning to column 0 (CPython #129385). Emulate ONLCR only when the
# kernel is not already doing it; never rewrite piped/MPI stdout.
# WeakSet, not id(): a closed stream's id can be reused by the next pty.
_raw_tty_newline_patched = weakref.WeakSet()


def _kernel_maps_nl_to_crnl(fd):
    """True if this tty will translate NL to CRNL on output."""
    try:
        import termios

        oflag = termios.tcgetattr(fd)[1]
        if not (oflag & termios.OPOST):
            return False
        onlcr = getattr(termios, "ONLCR", 0)
        return bool(not onlcr or (oflag & onlcr))
    except Exception:
        return True


def _with_explicit_cr(text):
    if not text or "\n" not in text:
        return text
    return text.replace("\r\n", "\n").replace("\n", "\r\n")


def _can_track_stream(stream):
    """True when the stream can live in the WeakSet.

    Membership hashes the stream and weakref.ref requires a slot for it.
    A custom stream that defines __eq__ without __hash__, or a slots object
    with no __weakref__, cannot be tracked. Leave it unpatched: wrapping
    write without remembering the stream would wrap it again on the next call.
    """
    try:
        hash(stream)
        weakref.ref(stream)
    except TypeError:
        return False
    return True


def install_raw_tty_newline_fix(streams=None):
    """Make print() return to column 0 when the tty is in pyrepl/raw mode.

    Safe to call more than once. Other GUIs that print from a background
    thread (for example neurongui) can call this instead of wrapping
    sys.stdout themselves.
    """
    if streams is None:
        streams = (sys.stdout, sys.stderr)
    for stream in streams:
        if not _can_track_stream(stream):
            continue
        try:
            if stream in _raw_tty_newline_patched:
                continue
        except TypeError:
            continue
        try:
            fd = stream.fileno()
        except Exception:
            continue
        orig_write = stream.write

        def write(s, _fd=fd, _orig=orig_write):
            if s and "\n" in s and not _kernel_maps_nl_to_crnl(_fd):
                s = _with_explicit_cr(s)
            return _orig(s)

        try:
            stream.write = write
        except Exception:
            continue
        try:
            _raw_tty_newline_patched.add(stream)
        except TypeError:
            try:
                stream.write = orig_write
            except Exception:
                pass


version = n.nrnversion(5)
__version__ = version
_userrxd = False

# Initialise neuron.config.arguments
from neuron import config

config._parse_arguments(h)


def _check_for_intel_openmp() -> None:
    """Check if Intel's OpenMP runtime has already been loaded.

    This does not interact well with the NVIDIA OpenMP runtime in CoreNEURON GPU
    builds. See
    https://forums.developer.nvidia.com/t/nvc-openacc-runtime-segfaults-if-intel-mkl-numpy-is-already-loaded/212739
    for more information.
    """
    import ctypes
    from neuron.config import arguments

    # These checks are only relevant for shared library builds with CoreNEURON GPU support enabled.
    if (
        not arguments["NRN_ENABLE_CORENEURON"]
        or not arguments["CORENRN_ENABLE_GPU"]
        or not arguments["CORENRN_ENABLE_SHARED"]
    ):
        return

    current_exe = ctypes.CDLL(None)
    try:
        # Picked quasi-randomly from `nm libiomp5.so`
        current_exe["_You_must_link_with_Intel_OpenMP_library"]
    except:
        # No Intel symbol found, all good
        pass
    else:
        # Intel symbol was found: danger! danger!
        raise Exception(
            "Intel OpenMP runtime detected. Try importing NEURON before Intel MKL and/or Numpy"
        )

    # Try to load the CoreNEURON shared library so that the various NVIDIA
    # runtime libraries also get loaded, before we import numpy lower down this
    # file.
    loaded_coreneuron = bool(n.coreneuron_handle())
    if not loaded_coreneuron:
        warnings.warn(
            "Failed to pre-load CoreNEURON when importing NEURON. "
            "Try running from the same directory where you ran "
            "nrnivmodl, or setting CORENEURONLIB. If you import "
            "something (e.g. Numpy with Intel MKL) that brings in "
            "an incompatible OpenMP runtime before launching a "
            "CoreNEURON GPU simulation then you may encounter "
            "errors."
        )


_check_for_intel_openmp()


# As a workaround to importing doc at neuron import time
# (which leads to chicken and egg issues on some platforms)
# define a dummy help function which imports doc,
# calls the real help function, and reassigns neuron.help to doc.help
# (thus replacing the dummy)
def help(request: object = None) -> None:
    global help
    from neuron import doc

    doc.help(request)
    help = doc.help


try:
    import pydoc

    pydoc.help = help
except:
    pass

# Global test-suite function


def test(exitOnError: bool = True) -> bool:
    """Runs a global battery of unit tests on the neuron module."""
    import neuron.tests
    import unittest

    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(neuron.tests.suite()).wasSuccessful()
    if exitOnError and result is False:
        sys.exit(1)
    return result


def test_rxd(exitOnError: bool = True) -> bool:
    """Runs a tests on the rxd and crxd modules."""
    import neuron.tests
    import unittest

    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(neuron.tests.test_rxd.suite()).wasSuccessful()
    if exitOnError and result is False:
        sys.exit(1)
    return result


# ------------------------------------------------------------------------------
# class factory for subclassing n.anyclass
# n.anyclass methods may be overridden. If so the base method can be called
# using the idiom self.basemethod = self.baseattr('methodname')
# ------------------------------------------------------------------------------

from neuron.hclass3 import HocBaseObject, hclass

# global list of paths already loaded by load_mechanisms
nrn_dll_loaded = []


def load_mechanisms(path: str, warn_if_already_loaded: bool = True) -> bool:
    """
    load_mechanisms(path)

    Search for and load NMODL mechanisms from the path given.

    This function will not load a mechanism path twice.

    The path should specify the directory in which nrnivmodl or mknrndll was run,
    and in which the directory 'i686' (or 'x86_64' or 'powerpc' depending on your platform)
    was created"""

    import platform

    global nrn_dll_loaded
    if path in nrn_dll_loaded:
        if warn_if_already_loaded:
            print(f"Mechanisms already loaded from path: {path}.  Aborting.")
        return True

    # in case NEURON is assuming a different architecture to Python,
    # we try multiple possibilities

    libname = f"{mechanism_prefix}nrnmech{mechanism_suffix}"
    arch_list = [platform.machine(), "i686", "x86_64", "powerpc", "umac"]

    # windows loads nrnmech.dll
    if n.unix_mac_pc() == 3:
        libname = "nrnmech.dll"
        arch_list = [""]

    for arch in arch_list:
        lib_path = os.path.join(path, arch, libname)
        if os.path.exists(lib_path):
            n.nrn_load_dll(lib_path)
            nrn_dll_loaded.append(path)
            return True
    print(f"NEURON mechanisms not found in {path}.")
    return False


if "NRN_NMODL_PATH" in os.environ:
    # os.pathsep is ':' on Unix and ';' on Windows (':' is a drive letter).
    nrn_nmodl_path = os.environ["NRN_NMODL_PATH"].split(os.pathsep)
    print("Auto-loading mechanisms:")
    print(f"NRN_NMODL_PATH={os.environ['NRN_NMODL_PATH']}")
    for x in nrn_nmodl_path:
        if not x:
            continue
        # print(f"from path {x}:")
        load_mechanisms(x)
        # print "\n"
    print("Done.\n")


# ------------------------------------------------------------------------------
# Python classes and functions without a Hoc equivalent, mainly for internal
# use within this file.
# ------------------------------------------------------------------------------


class HocError(Exception):
    pass


class Wrapper(object):
    """Base class to provide attribute access for HocObjects."""

    def __getattr__(self, name):
        if name == "hoc_obj":
            return self.__dict__["hoc_obj"]
        else:
            try:
                return self.__getattribute__(name)
            except AttributeError:
                return getattr(self.hoc_obj, name)

    def __setattr__(self, name, value):
        try:
            self.hoc_obj.__setattr__(name, value)
        except LookupError:
            object.__setattr__(self, name, value)


def new_point_process(name: str, doc: str = None) -> type:
    """
    Returns a Python-wrapped hoc class where the object needs to be associated
    with a section.

    doc - specify a docstring for the new pointprocess class
    """
    h(f"obfunc new_{name}() {{ return new {name}($1) }}")

    class someclass(Wrapper):
        __doc__ = doc

        def __init__(self, section, position=0.5):
            assert 0 <= position <= 1
            section.push()
            self.__dict__["hoc_obj"] = getattr(n, f"new_{name}")(
                position
            )  # have to put directly in __dict__ to avoid infinite recursion with __getattr__
            n.pop_section()

    someclass.__name__ = name
    return someclass


def new_hoc_class(name, doc=None):
    """
    Returns a Python-wrapped hoc class where the object does not need to be
    associated with a section.

    doc - specify a docstring for the new hoc class
    """
    h(f"obfunc new_{name}() {{ return new {name}() }}")

    class someclass(Wrapper):
        __doc__ = doc

        def __init__(self, **kwargs):
            self.__dict__["hoc_obj"] = getattr(h, f"new_{name}")()
            for k, v in list(kwargs.items()):
                setattr(self.hoc_obj, k, v)

    someclass.__name__ = name
    return someclass


def numpy_element_ref(numpy_array, index):
    """Return a HOC reference into a numpy array.

    Parameters
    ----------
    numpy_array : :class:`numpy.ndarray`
        the numpy array
    index : int
        the index into the numpy array

    .. warning::

        No bounds checking.

    .. warning::

        Assumes a contiguous array of doubles. In particular, be careful when
        using slices. If the array is multi-dimensional,
        the user must figure out the integer index to the desired element.
    """
    # The interface's reference; nrn_hocobj_ptr lived in the compiled extension.
    return _interface.numpy_element_ref(numpy_array, index)


def nrn_dll_sym(name, type=None):
    """return the specified object from the NEURON dlls.

    Parameters
    ----------
    name : string
        the name of the object (function, integer, etc...)
    type : None or ctypes type (e.g. ctypes.c_int)
        the type of the object (if None, assumes function pointer)
    """
    # TODO: this won't work under Windows; will need to search through until
    #       can find the right dll (should we cache the results of the search?)

    if os.name == "nt":
        return nrn_dll_sym_nt(name, type)
    dll = nrn_dll()
    if type is None:
        return dll.__getattr__(name)
    else:
        return type.in_dll(dll, name)


nt_dlls = []


def _nt_neuron_dll_dirs():
    """Directories that may contain nrniv.dll on Windows."""
    here = os.path.dirname(os.path.abspath(__file__))
    dirs = [
        os.path.join(here, ".data", "bin"),
        os.path.join(here, ".data", "lib"),
        here,
    ]
    home = os.environ.get("NEURONHOME")
    if home:
        home = os.path.abspath(home)
        dirs.append(os.path.join(home, "bin"))
        dirs.append(os.path.abspath(os.path.join(home, "..", "..", "bin")))
    try:
        nh = n.neuronhome().replace("/", os.sep)
        dirs.append(os.path.join(nh, "bin"))
        dirs.append(os.path.abspath(os.path.join(nh, "..", "..", "bin")))
    except Exception:
        pass
    out = []
    seen = set()
    for d in dirs:
        d = os.path.normpath(d)
        if d not in seen and os.path.isdir(d):
            seen.add(d)
            out.append(d)
    return out


def nrn_dll_sym_nt(name, type):
    """return the specified object from the NEURON dlls.
    helper for nrn_dll_sym(name, type).

    Windows GetProcAddress does not search a module's dependents (Unix dlsym
    does). ctypes of a basename also skips PATH (Python 3.8+). Load nrniv from
    the wheel .data/bin (or setup.exe NEURONHOME/bin) by full path. The MSVC
    name is nrniv.dll, MinGW's is libnrniv.dll.

    Use PyDLL, not CDLL. Unix nrn_dll() is ctypes.pydll (GIL held). CDLL
    releases the GIL.
    """
    global nt_dlls
    import ctypes
    import glob

    if len(nt_dlls) == 0:
        names = [
            "nrniv.dll",
            "libnrniv.dll",
        ]
        loaded = set()

        def _try_load(path_or_name):
            key = os.path.normcase(os.path.normpath(path_or_name))
            if key in loaded:
                return
            try:
                nt_dlls.append(ctypes.PyDLL(path_or_name))
                loaded.add(key)
            except OSError:
                pass

        for dllname in names:
            _try_load(dllname)
        for d in _nt_neuron_dll_dirs():
            for dllname in names:
                p = os.path.join(d, dllname)
                if os.path.isfile(p):
                    _try_load(p)
            for p in glob.glob(os.path.join(d, "*nrniv*.dll")):
                _try_load(p)
    for dll in nt_dlls:
        if type is None:
            try:
                return dll.__getattr__(name)
            except AttributeError:
                continue
        try:
            return type.in_dll(dll, name)
        except (ValueError, AttributeError):
            continue
    raise Exception("unable to connect to the NEURON library containing " + name)


def nrn_dll(printpath=False):
    """Return a ctypes object corresponding to the NEURON library.

    .. warning::

        This provides access to the C-language internals of NEURON and should
        be used with care.
    """
    if printpath:
        print(_interface.api.libnrniv._name)
    return _interface.api.libnrniv


def _modelview_mechanism_docstrings(dmech, tree):
    if dmech.name not in ("Ra", "capacitance"):
        if hasattr(nrn, dmech.name):  # KSChan not listed (and has no __doc__)
            docs = getattr(h, dmech.name).__doc__
            if docs.strip():
                for line in docs.split("\n"):
                    tree.append(line, dmech.location, 0)


_sec_db = {}


def _declare_contour(secobj, obj, name):
    array, i = _parse_import3d_name(name)
    if obj is None:
        sec = getattr(h, array)[i]
    else:
        sec = getattr(obj, array)[i]
    j = secobj.first
    center_vec = secobj.contourcenter(
        secobj.raw.getrow(0), secobj.raw.getrow(1), secobj.raw.getrow(2)
    )
    x0, y0, z0 = [center_vec.x[i] for i in range(3)]
    # store a couple of points to check if the section has been moved
    pts = [(sec.x3d(i), sec.y3d(i), sec.z3d(i)) for i in [0, sec.n3d() - 1]]
    # (is_stack, x, y, z, xcenter, ycenter, zcenter)
    _sec_db[sec.hoc_internal_name()] = (
        True if secobj.contour_list else False,
        secobj.raw.getrow(0).c(j),
        secobj.raw.getrow(1).c(j),
        secobj.raw.getrow(2).c(j),
        x0,
        y0,
        z0,
        pts,
    )


def _create_all_list(obj):
    # used by import3d
    obj.all = []


def _create_sections_in_obj(obj, name, numsecs):
    # used by import3d to instantiate inside of a Python object
    setattr(
        obj,
        name,
        [n.Section(name=f"{name}[{i}]", cell=obj) for i in range(int(numsecs))],
    )


def _connect_sections_in_obj(obj, childsecname, childx, parentsecname, parentx):
    # used by import3d
    childarray, childi = _parse_import3d_name(childsecname)
    parentarray, parenti = _parse_import3d_name(parentsecname)
    getattr(obj, childarray)[childi].connect(
        getattr(obj, parentarray)[parenti](parentx), childx
    )


def _parse_import3d_name(name):
    if "[" in name:
        import re

        array, i = re.search(r"(.*)\[(\d*)\]", name).groups()
        i = int(i)
    else:
        array = name
        i = 0
    return array, i


def _pt3dstyle_in_obj(obj, name, x, y, z):
    # used by import3d
    array, i = _parse_import3d_name(name)
    n.pt3dstyle(1, x, y, z, sec=getattr(obj, array)[i])


def _pt3dadd_in_obj(obj, name, x, y, z, d):
    array, i = _parse_import3d_name(name)
    n.pt3dadd(x, y, z, d, sec=getattr(obj, array)[i])


def _nmodl():
    try:
        from .nmodl import dsl as nmodl

        return nmodl
    except ModuleNotFoundError:
        raise Exception("Missing nmodl module")


class DensityMechanism:
    def __init__(self, name):
        """Initialize the DensityMechanism.

        Takes the name of a range mechanism; call via e.g. neuron.DensityMechanism('hh')
        """
        self.__name = name
        self.__mt = n.MechanismType(0)
        self.__mt.select(-1)
        self.__mt.select(name)
        if self.__mt.selected() == -1:
            raise Exception("No DensityMechanism: " + name)
        self.__has_nmodl = False
        self.__ast = None
        self.__ions = None
        try:
            _nmodl()

            self.__has_nmodl = True
        except:
            pass

    def __repr__(self):
        return f"neuron.DensityMechanism({self.__name!r})"

    def __dir__(self):
        my_dir = ["code", "file", "insert", "uninsert", "__repr__", "__str__"]
        if self.__has_nmodl:
            my_dir += ["ast", "ions", "ontology_ids"]
        return sorted(my_dir)

    @property
    def ast(self):
        """Abstract Syntax Tree representation.

        Requires the nmodl module, available from: https://github.com/bluebrain/nmodl

        The model is parsed on first access, and the results are cached for quick reaccess
        using the same neuron.DensityMechanism instance.
        """
        if self.__ast is None:
            nmodl = _nmodl()
            driver = nmodl.NmodlDriver()
            self.__ast = driver.parse_string(self.code)
        return self.__ast

    @property
    def code(self):
        """source code"""
        return self.__mt.code()

    @property
    def file(self):
        """source file path"""
        return self.__mt.file()

    @property
    def name(self):
        return self.__name

    def insert(self, secs):
        """insert this mechanism into a section or iterable of sections"""
        if isinstance(secs, nrn.Section):
            secs = [secs]
        for sec in secs:
            sec.insert(self.__name)

    def uninsert(self, secs):
        """uninsert (remove) this mechanism from a section or iterable of sections"""
        if isinstance(secs, nrn.Section):
            secs = [secs]
        for sec in secs:
            sec.uninsert(self.__name)

    @property
    def ions(self):
        """Dictionary of the ions involved in this mechanism"""
        if self.__ions is None:
            nmodl = _nmodl()
            lookup_visitor = nmodl.visitor.AstLookupVisitor()
            ions = lookup_visitor.lookup(self.ast, nmodl.ast.AstNodeType.USEION)
            result = {}
            for ion in ions:
                name = nmodl.to_nmodl(ion.name)
                read = [nmodl.to_nmodl(item) for item in ion.readlist]
                write = [nmodl.to_nmodl(item) for item in ion.writelist]
                if ion.valence:
                    valence = int(nmodl.to_nmodl(ion.valence.value))
                else:
                    valence = None
                ontology_id = None
                try:
                    ontology_id = ion.ontology_id
                except:
                    # older versions of the NMODL library didn't support .ontology_id
                    pass
                result[name] = {
                    "read": read,
                    "write": write,
                    "charge": valence,
                    "ontology_id": nmodl.to_nmodl(ontology_id),
                }
            self.__ions = result
        # return a copy
        return dict(self.__ions)

    @property
    def ontology_ids(self):
        nmodl = _nmodl()
        lookup_visitor = nmodl.visitor.AstLookupVisitor()

        try:
            onts = lookup_visitor.lookup(
                self.ast, nmodl.ast.AstNodeType.ONTOLOGY_STATEMENT
            )
        except AttributeError:
            raise Exception(
                "nmodl module out of date; missing support for ontology declarations"
            )

        return [nmodl.to_nmodl(ont.ontology_id) for ont in onts]


_store_savestates = []
_restore_savestates = []
_id_savestates = []


def register_savestate(id_, store, restore):
    """register routines to be called during SaveState

    id_ -- unique id (consider using a UUID)
    store -- called when saving the state to the object; returns a bytestring
    restore -- called when loading the state from the object; receives a bytestring
    """
    _id_savestates.append(id_)
    _store_savestates.append(store)
    _restore_savestates.append(restore)


def _store_savestate():
    import array
    import itertools

    version = 0
    result = [array.array("Q", [version]).tobytes()]
    for id_, store in zip(_id_savestates, _store_savestates):
        data = store()
        if len(data):
            result.append(
                array.array("Q", [len(id_)]).tobytes()
                + bytes(id_.encode("utf8"))
                + array.array("Q", [len(data)]).tobytes()
                + data
            )
    if len(result) == 1:
        # if no data to save, then don't even bother with a version
        result = []
    return bytearray(itertools.chain.from_iterable(result))


def _restore_savestate(data):
    import array

    # convert from bytearray
    data = bytes(data)
    metadata = array.array("Q")
    metadata.frombytes(data[:8])
    version = metadata[0]
    if version != 0:
        raise Exception("Unsupported SaveState version")
    position = 8
    while position < len(data):
        metadata = array.array("Q")
        metadata.frombytes(data[position : position + 8])
        name_length = metadata[0]
        position += 8
        name = data[position : position + name_length].decode("utf8")
        position += name_length
        metadata = array.array("Q")
        metadata.frombytes(data[position : position + 8])
        data_length = metadata[0]
        position += 8
        my_data = data[position : position + data_length]
        position += data_length
        # lookup the index because not everything that is registered is used
        try:
            index = _id_savestates.index(name)
        except ValueError:
            raise Exception("Undefined SaveState type " + name)
        _restore_savestates[index](my_data)
    if position != len(data):
        raise Exception("SaveState length error")


_mech_classes = {}


def _get_mech_object(name):
    if name in _mech_classes:
        my_class = _mech_classes[name]
    else:
        code = DensityMechanism(name).code
        # docstring is the title and a leading comment, if any
        inside_comment = False
        title = ""
        comment = []
        for line in code.split("\n"):
            line_s = line.strip()
            lower = line_s.lower()
            if inside_comment:
                if lower.startswith("endcomment"):
                    break
                comment.append(line)
            elif lower.startswith("title "):
                title = line_s[6:]
            elif lower.startswith("comment"):
                inside_comment = True
            elif line_s:
                break

        docstring = title + "\n\n"

        docstring += "\n".join(comment)
        docstring = docstring.strip()

        clsdict = {"__doc__": docstring, "title": title}
        my_class = type(name, (DensityMechanism,), clsdict)
        _mech_classes[name] = my_class
    return my_class(name)


# h.<mechanism> objects come from this factory, and SaveState calls the two
# savestate functions through the interface's hooks.
_interface.mechanism.set_density_mechanism_factory(_get_mech_object)
_interface.set_savestate_callbacks(_store_savestate, _restore_savestate)


def _has_scipy():
    """
    to check for scipy:

    has_scipy = 0
    objref p
    if (nrnpython("import neuron")) {
        p = new PythonObject()
        has_scipy = p.neuron._has_scipy()
    }
    """
    try:
        import scipy
    except:
        return 0
    return 1


def _pkl(arg):
    # print('neuron._pkl arg is ', arg)
    return n.Vector(0)


def numpy_from_pointer(cpointer, size):
    import numpy

    buf_from_mem = ctypes.pythonapi.PyMemoryView_FromMemory
    buf_from_mem.restype = ctypes.py_object
    buf_from_mem.argtypes = (ctypes.c_void_p, ctypes.c_int, ctypes.c_int)
    cbuffer = buf_from_mem(cpointer, size * numpy.dtype(float).itemsize, 0x200)
    return numpy.ndarray((size,), float, cbuffer, order="C")


@_atexit.register
def clear_gui_callback():
    nrn_dll_sym("nrnpy_set_gui_callback")(None)


def format_exception(type, value, tb):
    """Single string return wrapper for traceback.format_exception
    used by nrnpyerr_str
    """
    import traceback

    slist = (
        traceback.format_exception_only(type, value)
        if tb is None
        else traceback.format_exception(type, value, tb)
    )
    s = "".join(slist)
    return s


def nrnpy_pass():
    return 1


def nrnpy_pr(stdoe, s):
    if stdoe == 1:
        sys.stdout.write(s.decode())
    else:
        sys.stderr.write(s.decode())
        sys.stderr.flush()
    return 0


# HOC output already goes through Python's sys.stdout: the interface installs
# the same nrnpy_set_pr_etal hook (via nrn_stdout_redirect) when it initializes.


def _nrnpy_rvp_pyobj_callback(f):
    # unless f is an rxd variable, we return it directly
    f_type = str(type(f))
    if f_type not in (
        "<class 'neuron.rxd.species.SpeciesOnRegion'>",
        "<class 'neuron.rxd.species.Species'>",
        "<class 'neuron.rxd.species.State'>",
        "<class 'neuron.rxd.species.Parameter'>",
        "<class 'neuron.rxd.species.StateOnRegion'>",
        "<class 'neuron.rxd.species.ParameterOnRegion'>",
    ):
        return f

    # if we're here, f is an rxd variable, and we return a function that looks
    # up the weighted average concentration given an x and n.cas()
    # this is not particularly efficient so it is probably better to use this for
    # fixed timepoints rather than displays that update mid-simulation
    fref = weakref.ref(f)

    def result(x):
        if x == 0 or x == 1:
            raise Exception("Concentration is only defined for interior.")
        sp = fref()
        if sp:
            try:
                # n.cas() will fail if there are no sections
                nodes = sp.nodes(n.cas()(x))
            except:
                return None
            if nodes:
                total_volume = sum(node.volume for node in nodes)
                return (
                    sum(node.concentration * node.volume for node in nodes)
                    / total_volume
                )
            else:
                return None
        return None

    return result


# Vector arithmetic with numbers is the interface's Vector's (the legacy
# nrnpy_vec_math is gone); the RangeVarPlot hook that turns an rxd species into
# a callable is the interface's.
_interface.set_rvp_converter(_nrnpy_rvp_pyobj_callback)

try:
    from neuron.psection import psection

    nrn.set_psection(psection)
except:
    print("Failed to setup nrn.Section.psection")
pass


try:
    from IPython import get_ipython as _get_ipython
except:
    _get_ipython = lambda *args: None


def _hocobj_html(item):
    try:
        if item.hname().split("[")[0] == "ModelView":
            return _mview_html_tree(item.display.top)
        return None
    except:
        return None


def _mview_html_tree(hlist, inside_mechanisms_in_use=0):
    items = []
    if inside_mechanisms_in_use:
        miu_level = inside_mechanisms_in_use + 1
    else:
        miu_level = 0
    my_miu_level = miu_level
    for ho in hlist:
        html = ho.s.lstrip(" *")
        if ho.children:
            if html == "Mechanisms in use":
                my_miu_level = 1
        if html or miu_level == 3:
            if ho.children:
                children_data = _mview_html_tree(
                    ho.children, inside_mechanisms_in_use=my_miu_level
                )
                if miu_level == 3:
                    items.append(html + children_data)
                else:
                    items.append(
                        f"<div><details><summary style='cursor:pointer'>{html}</summary><div style='margin-left:1.06em'>{children_data}</div></div>"
                    )
            else:
                if miu_level == 3:
                    items.append(html)
                else:
                    items.append(f"<div style='margin-left:1.06em'>{html}</div>")

    if miu_level == 3:
        return f"{'<br>'.join(items)}"
    else:
        return f"{''.join(items)}"


# register our ModelView display formatter with Jupyter if available
if _get_ipython() is not None:
    html_formatter = _get_ipython().display_formatter.formatters["text/html"]
    html_formatter.for_type(hoc.HocObject, _hocobj_html)

# in case Bokeh is installed, register a serialization function for hoc.Vector
try:
    from bokeh.core.serialization import Serializer

    Serializer.register(
        n.Vector, lambda obj, serializer: [serializer.encode(item) for item in obj]
    )
except ImportError:
    pass
