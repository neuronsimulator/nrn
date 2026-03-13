"""

neuron
======

For empirically-based simulations of neurons and networks of neurons in Python.

This is the top-level module of the official python interface to
the NEURON simulation environment (https://nrn.readthedocs.io).

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

import sys
import os
import warnings
import weakref

embedded = "hoc" in sys.modules

# First, check that the compiled extension (neuron.hoc) was built for this version of
# Python. If not, fail early and helpfully.
from ._config_params import (
    supported_python_versions,
    mechanism_prefix,
    mechanism_suffix,
)

current_version = "{}.{}".format(*sys.version_info[:2])
if current_version not in supported_python_versions:
    message = (
        "Python {} is not supported by this NEURON installation (supported: {}). Either re-build "
        "NEURON with support for this version, use a supported version of Python, or try using "
        "nrniv -python so that NEURON can suggest a compatible version for you."
    ).format(current_version, " ".join(supported_python_versions))
    raise ImportError(message)

try:  # needed since python 3.8 on windows if python launched
    # do this here as NEURONHOME may be changed below
    nrnbindir = os.path.abspath(os.environ["NEURONHOME"] + "/bin")
    os.add_dll_directory(nrnbindir)
except:
    pass

# With pip we need to rewrite the NEURONHOME
nrn_path = os.path.abspath(os.path.join(os.path.dirname(__file__), ".data/share/nrn"))
if os.path.isdir(nrn_path):
    os.environ["NEURONHOME"] = nrn_path

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

# Import the compiled HOC extension. We already checked above that it exists for the
# current Python version.
from . import hoc

# These are strange beasts that are defined inside the compiled `hoc` extension, all
# efforts to make them relative imports (because they are internal) have failed. It's
# not clear if the import of _neuron_section is needed, and this could probably be
# handled more idiomatically.
import nrn
import _neuron_section

# we keep the old h as before with the old repr and type
h = hoc.HocObject()


class _NEURON_INTERFACE(hoc.HocObject):
    """
    neuron.n
    ========

    neuron.n is the top-level NEURON inteface, starting in NEURON 9.

    >>> from neuron import n
    >>> n
    <TopLevelNEURONInterface>

    Most NEURON classes and functions are defined in the n namespace
    and can be accessed as follows:

    >>> v = n.Vector(10)
    >>> soma = n.Section("soma")
    >>> input = n.IClamp(soma(0.5))
    >>> n.finitialize(-65)

    Each built-in class has its own type, so for the above definitions we have:

    >>> type(v)
    <class 'hoc.Vector'>
    >>> type(soma)
    <class 'nrn.Section'>

    But since ``IClamp`` is defined by a MOD file:

    >>> type(input)
    <class 'hoc.HocObject'>

    Other submodules of neuron exist, including rxd and units.

    You can see the functions, classes, etc available inside n via dir(n)
    and can get help on each via the standard Python help system, e.g.,

    >>> help(n.finitialize)

    The full NEURON documentation is available online at
    https://nrn.readthedocs.io
    """

    def __repr__(self):
        return "<TopLevelNEURONInterface>"

    def __dir__(self):
        return dir(h)


n = _NEURON_INTERFACE()

version = n.nrnversion(5)
__version__ = version
_userrxd = False

# Initialise neuron.config.arguments
from neuron import config

config._parse_arguments(h)


def _check_for_intel_openmp():
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

_original_hoc_file = None
if not hasattr(hoc, "__file__"):
    # first try is to derive from neuron.__file__
    origin = None  # path to neuron/__init__.py
    from importlib import util

    mspec = util.find_spec("neuron")
    if mspec:
        origin = mspec.origin
    if origin is not None:
        import sysconfig

        hoc_path = (
            origin.rstrip("__init__.py")
            + "hoc"
            + sysconfig.get_config_var("EXT_SUFFIX")
        )
        setattr(hoc, "__file__", hoc_path)
else:
    _original_hoc_file = hoc.__file__


# As a workaround to importing doc at neuron import time
# (which leads to chicken and egg issues on some platforms)
# define a dummy help function which imports doc,
# calls the real help function, and reassigns neuron.help to doc.help
# (thus replacing the dummy)
def help(request=None):
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


def test(exitOnError=True):
    """Runs a global battery of unit tests on the neuron module."""
    import neuron.tests
    import unittest

    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(neuron.tests.suite()).wasSuccessful()
    if exitOnError and result is False:
        sys.exit(1)
    return result


def test_rxd(exitOnError=True):
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


def load_mechanisms(path, warn_if_already_loaded=True):
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
    nrn_nmodl_path = os.environ["NRN_NMODL_PATH"].split(":")
    print("Auto-loading mechanisms:")
    print(f"NRN_NMODL_PATH={os.environ['NRN_NMODL_PATH']}")
    for x in nrn_nmodl_path:
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
                return self.hoc_obj.__getattribute__(name)

    def __setattr__(self, name, value):
        try:
            self.hoc_obj.__setattr__(name, value)
        except LookupError:
            object.__setattr__(self, name, value)


def new_point_process(name, doc=None):
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


_nrn_dll = None
_nrn_hocobj_ptr = None
_double_ptr = None
_double_size = None


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
    global _nrn_dll, _double_ptr, _double_size, _nrn_hocobj_ptr
    import ctypes

    if _nrn_hocobj_ptr is None:
        _nrn_hocobj_ptr = nrn_dll_sym("nrn_hocobj_ptr")
        _nrn_hocobj_ptr.restype = ctypes.py_object
        _double_ptr = ctypes.POINTER(ctypes.c_double)
        _double_size = ctypes.sizeof(ctypes.c_double)
    void_p = (
        ctypes.cast(numpy_array.ctypes.data_as(_double_ptr), ctypes.c_voidp).value
        + index * _double_size
    )
    return _nrn_hocobj_ptr(ctypes.cast(void_p, _double_ptr))


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


def nrn_dll_sym_nt(name, type):
    """return the specified object from the NEURON dlls.
    helper for nrn_dll_sym(name, type). Try to find the name in either
    nrniv.dll or libnrnpython1013.dll
    """
    global nt_dlls
    import ctypes

    if len(nt_dlls) == 0:
        b = "bin"
        path = os.path.join(n.neuronhome().replace("/", "\\"), b)
        for dllname in [
            "libnrniv.dll",
            "libnrnpython{}.{}.dll".format(*sys.version_info[:2]),
        ]:
            p = os.path.join(path, dllname)
            try:
                nt_dlls.append(ctypes.cdll[p])
            except:
                pass
    for dll in nt_dlls:
        try:
            a = dll.__getattr__(name)
        except:
            a = None
        if a:
            if type is None:
                return a
            else:
                return type.in_dll(dll, name)
    raise Exception("unable to connect to the NEURON library containing " + name)


def nrn_dll(printpath=False):
    """Return a ctypes object corresponding to the NEURON library.

    .. warning::

        This provides access to the C-language internals of NEURON and should
        be used with care.
    """
    import ctypes
    import glob

    try:
        # extended? if there is a __file__, then use that
        if printpath:
            print(f"hoc.__file__ {_original_hoc_file}")
        the_dll = ctypes.pydll[_original_hoc_file]
        return the_dll
    except:
        pass

    success = False
    if sys.platform == "msys" or sys.platform == "win32":
        p = f"hoc{sys.version_info[0]}{sys.version_info[1]}"
    else:
        p = "hoc"

    try:
        # maybe hoc.so in this neuron module
        base_path = os.path.join(os.path.split(__file__)[0], p)
        dlls = glob.glob(base_path + "*.*")
        for dll in dlls:
            try:
                the_dll = ctypes.pydll[dll]
                if printpath:
                    print(dll)
                return the_dll
            except:
                pass
    except:
        pass
    # maybe old default module location
    neuron_home = os.path.split(os.path.split(n.neuronhome())[0])[0]
    base_path = os.path.join(neuron_home, "lib", "python", "neuron", p)
    for extension in ["", ".dll", ".so", ".dylib"]:
        dlls = glob.glob(base_path + "*" + extension)
        for dll in dlls:
            try:
                the_dll = ctypes.pydll[dll]
                if printpath:
                    print(dll)
                success = True
            except:
                pass
            if success:
                break
        if success:
            break
    else:
        raise Exception("unable to connect to the NEURON library")
    return the_dll


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


def numpy_from_pointer(cpointer, size):
    buf_from_mem = ctypes.pythonapi.PyMemoryView_FromMemory
    buf_from_mem.restype = ctypes.py_object
    buf_from_mem.argtypes = (ctypes.c_void_p, ctypes.c_int, ctypes.c_int)
    cbuffer = buf_from_mem(cpointer, size * numpy.dtype(float).itemsize, 0x200)
    return numpy.ndarray((size,), float, cbuffer, order="C")


try:
    import ctypes
    import numpy
    import traceback

    vec_to_numpy_prototype = ctypes.CFUNCTYPE(
        ctypes.py_object, ctypes.c_int, ctypes.POINTER(ctypes.c_double)
    )

    def vec2numpy(size, data):
        try:
            return numpy_from_pointer(data, size)
        except:
            traceback.print_exc()
            return None

    vec_to_numpy_callback = vec_to_numpy_prototype(vec2numpy)
    set_vec_as_numpy = nrn_dll_sym("nrnpy_set_vec_as_numpy")
    set_vec_as_numpy(vec_to_numpy_callback)
except:
    pass


class _WrapperPlot:
    def __init__(self, data):
        """do not call directly"""
        self._data = data

    def __repr__(self):
        return "{}.plot()".format(repr(self._data))


class _RangeVarPlot(_WrapperPlot):
    """Plots the current state of the RangeVarPlot on the graph.

    Additional arguments and keyword arguments are passed to the graph's
    plotting method.

    Example, showing plotting to NEURON graphics, bokeh, matplotlib,
    plotnine/ggplot, and plotly:

    .. code::

      from matplotlib import pyplot
      from neuron import n, gui
      import bokeh.plotting as b
      import plotly
      import plotly.graph_objects as go
      import plotnine as p9
      import math

      dend = n.Section(name='dend')
      dend.nseg = 55
      dend.L = 6.28

      # looping over dend.allseg instead of dend to set 0 and 1 ends
      for seg in dend.allseg():
          seg.v = math.sin(dend.L * seg.x)

      r = n.RangeVarPlot('v', dend(0), dend(1))

      # matplotlib
      graph = pyplot.gca()
      r.plot(graph, linewidth=10, color='r')

      # NEURON Interviews graph
      g = n.Graph()
      r.plot(g, 2, 3)
      g.exec_menu('View = plot')

      # Bokeh
      bg = b.Figure()
      r.plot(bg, line_width=10)
      b.show(bg)

      # plotly
      r.plot(plotly).show()

      # also plotly
      fig = go.Figure()
      r.plot(fig)
      fig.show()

      pyplot.show()

      # plotnine/ggplot
      p9.ggplot() + r.plot(p9)

      # alternative plotnine/ggplot
      r.plot(p9.ggplot())
    """

    def __call__(self, graph, *args, **kwargs):
        yvec = n.Vector()
        xvec = n.Vector()
        self._data.to_vector(yvec, xvec)
        if isinstance(graph, hoc.HocObject):
            return yvec.line(graph, xvec, *args)
        str_type_graph = str(type(graph))
        if str_type_graph == "<class 'plotly.graph_objs._figure.Figure'>":
            # plotly figure
            import plotly.graph_objects as go

            kwargs.setdefault("mode", "lines")
            return graph.add_trace(go.Scatter(x=xvec, y=yvec, *args, **kwargs))
        if str_type_graph == "<class 'plotnine.ggplot.ggplot'>":
            # ggplot object
            import plotnine as p9
            import pandas as pd

            return graph + p9.geom_line(
                *args,
                data=pd.DataFrame({"x": xvec, "y": yvec}),
                mapping=p9.aes(x="x", y="y"),
                **kwargs,
            )
        str_graph = str(graph)
        if str_graph.startswith("<module 'plotly' from "):
            # plotly module
            import plotly.graph_objects as go

            fig = go.Figure()
            kwargs.setdefault("mode", "lines")
            return fig.add_trace(go.Scatter(x=xvec, y=yvec, *args, **kwargs))
        if str_graph.startswith("<module 'plotnine' from "):
            # plotnine module (contains ggplot)
            import plotnine as p9
            import pandas as pd

            return p9.geom_line(
                *args,
                data=pd.DataFrame({"x": xvec, "y": yvec}),
                mapping=p9.aes(x="x", y="y"),
                **kwargs,
            )
        if hasattr(graph, "plot"):
            # works with e.g. pyplot or a matplotlib axis
            return graph.plot(xvec, yvec, *args, **kwargs)
        if hasattr(graph, "line"):
            # works with e.g. bokeh
            return graph.line(xvec, yvec, *args, **kwargs)
        if str_type_graph == "<class 'matplotlib.figure.Figure'>":
            raise Exception("plot to a matplotlib axis not a matplotlib figure")
        raise Exception("Unable to plot to graphs of type {}".format(type(graph)))


class _PlotShapePlot(_WrapperPlot):
    """Plots the currently selected data on an object.

    Currently only pyplot is supported, e.g.

    from matplotlib import pyplot
    ps = n.PlotShape(False)
    ps.variable('v')
    ps.plot(pyplot)
    pyplot.show()

    Limitations: many. Currently only supports plotting a full cell colored based on a variable.
    """

    # TODO: handle pointmark, specified sections, color
    def __call__(self, graph, *args, **kwargs):
        from neuron.gui2.utilities import _segment_3d_pts

        def _get_pyplot_axis3d(fig):
            """requires matplotlib"""
            from . import rxd
            from matplotlib.pyplot import cm
            import matplotlib.pyplot as plt
            from mpl_toolkits.mplot3d import Axes3D
            import numpy as np

            class Axis3DWithNEURON(Axes3D):
                def auto_aspect(self):
                    """sets the x, y, and z range symmetric around the center

                    Probably needs a square figure to preserve lengths as you rotate."""
                    bounds = [self.get_xlim(), self.get_ylim(), self.get_zlim()]
                    half_delta_max = max([(item[1] - item[0]) / 2 for item in bounds])
                    xmid = sum(bounds[0]) / 2
                    ymid = sum(bounds[1]) / 2
                    zmid = sum(bounds[2]) / 2
                    self.auto_scale_xyz(
                        [xmid - half_delta_max, xmid + half_delta_max],
                        [ymid - half_delta_max, ymid + half_delta_max],
                        [zmid - half_delta_max, zmid + half_delta_max],
                    )

                def mark(self, segment, marker="or", **kwargs):
                    """plot a marker on a segment

                    Args:
                        segment = the segment to mark
                        marker = matplotlib marker
                        **kwargs = passed to matplotlib's plot
                    """
                    x, y, z = _get_3d_pt(segment)
                    self.plot([x], [y], [z], marker)
                    return self

                def _do_plot(
                    self,
                    val_min,
                    val_max,
                    sections,
                    variable,
                    mode,
                    line_width=2,
                    cmap=cm.cool,
                    **kwargs,
                ):
                    """
                    Plots a 3D shapeplot
                    Args:
                        sections = list of n.Section() objects to be plotted
                        **kwargs passes on to matplotlib (e.g. linewidth=2 for thick lines)
                    Returns:
                        lines = list of line objects making up shapeplot
                    """
                    # Adapted from
                    # https://github.com/ahwillia/PyNeuron-Toolbox/blob/master/PyNeuronToolbox/morphology.py
                    # Accessed 2019-04-11, which had an MIT license

                    # Default is to plot all sections.
                    if sections is None:
                        sections = list(n.allsec())

                    n.define_shape()

                    # Plot each segement as a line
                    lines = {}
                    lines_list = []
                    vals = []

                    if isinstance(variable, rxd.species.Species):
                        if len(variable.regions) > 1:
                            raise Exception("Please specify region for the species.")

                    for sec in sections:
                        all_seg_pts = _segment_3d_pts(sec)
                        for seg, (xs, ys, zs, _, _) in zip(sec, all_seg_pts):
                            if mode == 0:
                                width = seg.diam
                            else:
                                width = line_width
                            (line,) = self.plot(
                                xs, ys, zs, "-", linewidth=width, **kwargs
                            )
                            if variable is not None:
                                val = _get_variable_seg(seg, variable)
                                vals.append(val)
                                if val is not None:
                                    lines[line] = f"{val} at {seg}"
                                else:
                                    lines[line] = str(seg)
                            else:
                                lines[line] = str(seg)
                            lines_list.append(line)
                    if variable is not None:
                        val_range = val_max - val_min
                        if val_range:
                            for sec in sections:
                                for line, val in zip(lines_list, vals):
                                    if val is not None:
                                        if "color" not in kwargs:
                                            col = _get_color(
                                                variable,
                                                val,
                                                cmap,
                                                val_min,
                                                val_max,
                                                val_range,
                                            )
                                        else:
                                            col = kwargs["color"]
                                    else:
                                        col = kwargs.get("color", "black")
                                    line.set_color(col)
                    return lines

            ax = Axis3DWithNEURON(fig)
            fig.add_axes(ax)
            return ax

        def _get_variable_seg(seg, variable):
            if isinstance(variable, str):
                try:
                    if "." in variable:
                        mech, var = variable.split(".")
                        val = getattr(getattr(seg, mech), var)
                    else:
                        val = getattr(seg, variable)
                except AttributeError:
                    # leave default color if no variable found
                    val = None
            else:
                try:
                    vals = variable.nodes(seg).concentration
                    val = sum(vals) / len(vals)
                except:
                    val = None

            return val

        def _get_3d_pt(segment):
            import numpy as np

            # TODO: there has to be a better way to do this
            sec = segment.sec
            n3d = sec.n3d()
            arc3d = [sec.arc3d(i) for i in range(n3d)]
            x3d = np.array([sec.x3d(i) for i in range(n3d)])
            y3d = np.array([sec.y3d(i) for i in range(n3d)])
            z3d = np.array([sec.z3d(i) for i in range(n3d)])
            seg_l = sec.L * segment.x
            x = np.interp(seg_l, arc3d, x3d)
            y = np.interp(seg_l, arc3d, y3d)
            z = np.interp(seg_l, arc3d, z3d)
            return x, y, z

        def _do_plot_on_matplotlib_figure(fig, *args, **kwargs):
            import ctypes

            get_plotshape_data = nrn_dll_sym("get_plotshape_data")
            get_plotshape_data.restype = ctypes.py_object
            variable, varobj, lo, hi, secs = get_plotshape_data(
                ctypes.py_object(self._data)
            )
            if varobj is not None:
                variable = varobj
            kwargs.setdefault("picker", 2)
            result = _get_pyplot_axis3d(fig)
            ps = self._data
            mode = ps.show()
            _lines = result._do_plot(lo, hi, secs, variable, mode, *args, **kwargs)
            result._mouseover_text = ""

            def _onpick(event):
                if event.artist in _lines:
                    result._mouseover_text = _lines[event.artist]
                else:
                    result._mouseover_text = ""
                return True

            result.auto_aspect()
            fig.canvas.mpl_connect("pick_event", _onpick)

            def format_coord(*args):
                return result._mouseover_text

            result.format_coord = format_coord
            return result

        def _get_color(variable, val, cmap, lo, hi, val_range):
            if variable is None or val is None:
                col = "black"
            elif val_range == 0:
                if val < lo:
                    col = color_to_hex(cmap(0))
                elif val > hi:
                    col = color_to_hex(cmap(255))
                else:
                    col = color_to_hex(cmap(128))
            else:
                col = color_to_hex(
                    cmap(int(255 * (min(max(val, lo), hi) - lo) / (val_range)))
                )
            return col

        def color_to_hex(col):
            items = [hex(int(255 * col_item))[2:] for col_item in col][:-1]
            return "#" + "".join(
                [item if len(item) == 2 else "0" + item for item in items]
            )

        def _do_plot_on_plotly(width=2, color=None, cmap=None):
            """requires matplotlib for colormaps if not specified explicitly"""
            import ctypes
            from . import rxd
            import plotly.graph_objects as go

            class FigureWidgetWithNEURON(go.FigureWidget):
                def mark(self, segment, marker="or", **kwargs):
                    """plot a marker on a segment


                    Args:
                        segment = the segment to mark
                        **kwargs = passed to go.Scatter3D plot
                    """
                    x, y, z = _get_3d_pt(segment)
                    # approximately match the appearance of the matplotlib defaults
                    kwargs.setdefault("marker_size", 5)
                    kwargs.setdefault("marker_color", "red")
                    kwargs.setdefault("marker_opacity", 1)

                    self.add_trace(
                        go.Scatter3d(
                            x=[x],
                            y=[y],
                            z=[z],
                            name="",
                            hovertemplate=str(segment),
                            **kwargs,
                        )
                    )
                    return self

            get_plotshape_data = nrn_dll_sym("get_plotshape_data")
            get_plotshape_data.restype = ctypes.py_object
            variable, varobj, lo, hi, secs = get_plotshape_data(
                ctypes.py_object(self._data)
            )

            ps = self._data
            mode = ps.show()

            if varobj is not None:
                variable = varobj
            if secs is None:
                secs = list(n.allsec())

            if variable is None and varobj is None:
                kwargs.setdefault("color", "black")

                data = []
                for sec in secs:
                    xs = [sec.x3d(i) for i in range(sec.n3d())]
                    ys = [sec.y3d(i) for i in range(sec.n3d())]
                    zs = [sec.z3d(i) for i in range(sec.n3d())]
                    data.append(
                        go.Scatter3d(
                            x=xs,
                            y=ys,
                            z=zs,
                            name="",
                            hovertemplate=str(sec),
                            mode="lines",
                            line=go.scatter3d.Line(color=kwargs["color"], width=width),
                        )
                    )
                return FigureWidgetWithNEURON(data=data, layout={"showlegend": False})

            else:
                if "cmap" not in kwargs:
                    # use same default colormap as the matplotlib version
                    from matplotlib.pyplot import cm

                    kwargs["cmap"] = cm.cool

                cmap = kwargs["cmap"]

                # show_diam = False

                # calculate bounds

                val_range = hi - lo

                data = []

                if isinstance(variable, rxd.species.Species):
                    if len(variable.regions) > 1:
                        raise Exception("Please specify region for the species.")

                for sec in secs:
                    all_seg_pts = _segment_3d_pts(sec)
                    for seg, (xs, ys, zs, _, _) in zip(sec, all_seg_pts):
                        val = _get_variable_seg(seg, variable)
                        hover_template = str(seg)
                        if val is not None:
                            hover_template += "<br>" + f"{val:.3f}"
                        if color is None:
                            col = _get_color(variable, val, cmap, lo, hi, val_range)
                        else:
                            col = color
                        if mode == 0:
                            diam = seg.diam
                        else:
                            diam = width

                        data.append(
                            go.Scatter3d(
                                x=xs,
                                y=ys,
                                z=zs,
                                name="",
                                hovertemplate=hover_template,
                                mode="lines",
                                line=go.scatter3d.Line(color=col, width=diam),
                            )
                        )

                return FigureWidgetWithNEURON(data=data, layout={"showlegend": False})

        if hasattr(graph, "__name__"):
            if graph.__name__ == "matplotlib.pyplot":
                fig = graph.figure()
                return _do_plot_on_matplotlib_figure(fig, *args, **kwargs)
            elif graph.__name__ == "plotly":
                return _do_plot_on_plotly(*args, **kwargs)
        elif str(type(graph)) == "<class 'matplotlib.figure.Figure'>":
            return _do_plot_on_matplotlib_figure(graph)
        raise NotImplementedError


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


try:
    import ctypes

    def _rvp_plot(rvp):
        return _RangeVarPlot(rvp)

    def _plotshape_plot(ps):
        n.define_shape()
        return _PlotShapePlot(ps)

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

    set_toplevel_callbacks = nrn_dll_sym("nrnpy_set_toplevel_callbacks")
    _rvp_plot_callback = ctypes.py_object(_rvp_plot)
    _plotshape_plot_callback = ctypes.py_object(_plotshape_plot)
    _get_mech_object_callback = ctypes.py_object(_get_mech_object)
    _restore_savestate_callback = ctypes.py_object(_restore_savestate)
    _store_savestate_callback = ctypes.py_object(_store_savestate)
    set_toplevel_callbacks(
        _rvp_plot_callback,
        _plotshape_plot_callback,
        _get_mech_object_callback,
        _store_savestate_callback,
        _restore_savestate_callback,
    )
except:
    pass


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


# nrnpy_pr callback in place of hoc printf
# ensures consistent with python stdout even with jupyter notebook.
# nrnpy_pass callback used by n.doNotify() in MINGW when not called from
# gui thread in order to allow the gui thread to run.
# When this was introduced in ef4da5dbf293580ee1bf86b3a94d3d2f80226f62 it was wrapped in a
# try .. except .. pass block for reasons that are not obvious to olupton, who removed it.
if not embedded:
    # Unconditionally redirecting NEURON printing via Python seemed to cause re-ordering
    # of NEURON output in the ModelDB CI. This might be because the redirection is only
    # triggered by `import neuron`, and an arbitrary amount of NEURON code may have been
    # executed before that point.
    nrnpy_set_pr_etal = nrn_dll_sym("nrnpy_set_pr_etal")

    nrnpy_pr_proto = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_int, ctypes.c_char_p)
    nrnpy_pass_proto = ctypes.CFUNCTYPE(ctypes.c_int)
    nrnpy_set_pr_etal.argtypes = [nrnpy_pr_proto, nrnpy_pass_proto]

    nrnpy_pr_callback = nrnpy_pr_proto(nrnpy_pr)
    nrnpy_pass_callback = nrnpy_pass_proto(nrnpy_pass)
    nrnpy_set_pr_etal(nrnpy_pr_callback, nrnpy_pass_callback)


def nrnpy_vec_math(op, flag, arg1, arg2=None):
    import numbers

    valid_types = (numbers.Number, hoc.HocObject)
    if isinstance(arg1, valid_types):
        if flag == 2:
            # unary
            arg1 = arg1.c()
            if op == "uneg":
                return arg1.mul(-1)
            if op == "upos":
                return arg1
            if op == "uabs":
                return arg1.abs()
        elif isinstance(arg2, valid_types):
            if flag == 1:
                # either reversed (flag=1) or unary (flag=2)
                arg2 = arg2.c()
                if op in ("mul", "add"):
                    return getattr(arg2, op)(arg1)
                if op == "div":
                    return arg2.pow(-1).mul(arg1)
                if op == "sub":
                    return arg2.mul(-1).add(arg1)
            else:
                arg1 = arg1.c()
                return getattr(arg1, op)(arg2)

    return NotImplemented


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


try:
    nrnpy_vec_math_register = nrn_dll_sym("nrnpy_vec_math_register")
    nrnpy_vec_math_register(ctypes.py_object(nrnpy_vec_math))
except:
    print("Failed to setup nrnpy_vec_math")

try:
    _nrnpy_rvp_pyobj_callback_register = nrn_dll_sym(
        "nrnpy_rvp_pyobj_callback_register"
    )
    _nrnpy_rvp_pyobj_callback_register(ctypes.py_object(_nrnpy_rvp_pyobj_callback))
except:
    print("Failed to setup _nrnpy_rvp_pyobj_callback")

try:
    from neuron.psection import psection

    nrn.set_psection(psection)
except:
    print("Failed to setup nrn.Section.psection")
pass

import atexit as _atexit


@_atexit.register
def clear_gui_callback():
    try:
        nrnpy_set_gui_callback = nrn_dll_sym("nrnpy_set_gui_callback")
        nrnpy_set_gui_callback(None)
    except:
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
