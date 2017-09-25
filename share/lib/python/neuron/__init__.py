"""

neuron
======

For empirically-based simulations of neurons and networks of neurons in Python.

This is the top-level module of the official python interface to
the NEURON simulation environment (http://neuron.yale.edu/neuron/).

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

  neuron.init, run, psection, load_mechanisms


neuron.h

   The top-level Hoc interpreter.

   Execute Hoc commands by calling h with a string argument:

   >>> h('objref myobj')
   >>> h('myobj = new Vector(10)')

   All Hoc defined variables are accessible by attribute access to h.
   
   Example:

   >>> print h.myobj.x[9]

   Hoc Classes are also defined, for example:

   >>> v = h.Vector([1,2,3])
   >>> soma = h.Section()

   More help is available for the respective class by looking in the object docstring:

   >>> help(h.Vector)
   


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
#try:
#  import sys
#  import DLFCN
#  sys.setdlopenflags(DLFCN.RTLD_NOW | DLFCN.RTLD_GLOBAL)
#except:
#  pass

import sys
embedded = True if 'hoc' in sys.modules else False

try:
    import hoc
except:
  try:
    #Python3.1 extending needs to look into the module explicitly
    import neuron.hoc
  except: # mingw name strategy
    exec("import neuron.hoc%d%d as hoc" % (sys.version_info[0], sys.version_info[1]))

import nrn
import _neuron_section
h  = hoc.HocObject()


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

def test():
    """ Runs a global battery of unit tests on the neuron module."""
    import neuron.tests
    import unittest

    runner = unittest.TextTestRunner(verbosity=2)
    runner.run(neuron.tests.suite())


# ------------------------------------------------------------------------------
# class factory for subclassing h.anyclass
# h.anyclass methods may be overridden. If so the base method can be called
# using the idiom self.basemethod = self.baseattr('methodname')
# ------------------------------------------------------------------------------

if sys.version_info[0] == 2:
  from neuron.hclass2 import hclass
else:
  from neuron.hclass3 import hclass

# global list of paths already loaded by load_mechanisms
nrn_dll_loaded = []

def load_mechanisms(path):
    """
    load_mechanisms(path)

    Search for and load NMODL mechanisms from the path given.

    This function will not load a mechanism path twice.

    The path should specify the directory in which nrnmodliv was run,
    and in which the directory 'i686' (or 'x86_64' or 'powerpc' depending on your platform)
    was created"""

    import platform
    
    global nrn_dll_loaded
    if path in nrn_dll_loaded:
        print("Mechanisms already loaded from path: %s.  Aborting." % path)
        return
    
    # in case NEURON is assuming a different architecture to Python,
    # we try multiple possibilities

    arch_list = [platform.machine(), 'i686', 'x86_64', 'powerpc', 'umac']

    for arch in arch_list:
        lib_path = os.path.join(path, arch, '.libs', 'libnrnmech.so')
        if os.path.exists(lib_path):
            h.nrn_load_dll(lib_path)
            nrn_dll_loaded.append(path)
            return
    print("NEURON mechanisms not found in %s." % path)


import os,sys
if 'NRN_NMODL_PATH' in os.environ:
    nrn_nmodl_path = os.environ['NRN_NMODL_PATH'].split(':')
    print('Auto-loading mechanisms:')
    print('NRN_NMODL_PATH=%s' % os.environ['NRN_NMODL_PATH'])
    for x in nrn_nmodl_path:
        #print "from path %s:" % x
        load_mechanisms(x)
        #print "\n"
    print("Done.\n")
    


# ------------------------------------------------------------------------------
# Python classes and functions without a Hoc equivalent, mainly for internal
# use within this file.
# ------------------------------------------------------------------------------

class HocError(Exception): pass

class Wrapper(object):
    """Base class to provide attribute access for HocObjects."""
    def __getattr__(self, name):
        if name == 'hoc_obj':
            return self.__dict__['hoc_obj']
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

def new_point_process(name,doc=None):
    """
    Returns a Python-wrapped hoc class where the object needs to be associated
    with a section.

    doc - specify a docstring for the new pointprocess class
    """
    h('obfunc new_%s() { return new %s($1) }' % (name, name))
    class someclass(Wrapper):
        __doc__ = doc
        def __init__(self, section, position=0.5):
            assert 0 <= position <= 1
            section.push()
            self.__dict__['hoc_obj'] = getattr(h, 'new_%s' % name)(position) # have to put directly in __dict__ to avoid infinite recursion with __getattr__
            h.pop_section()
    someclass.__name__ = name
    return someclass

def new_hoc_class(name,doc=None):
    """
    Returns a Python-wrapped hoc class where the object does not need to be
    associated with a section.

    doc - specify a docstring for the new hoc class
    """
    h('obfunc new_%s() { return new %s() }' % (name, name))
    class someclass(Wrapper):
        __doc__ = doc
        def __init__(self, **kwargs):
            self.__dict__['hoc_obj'] = getattr(h, 'new_%s' % name)()
            for k,v in list(kwargs.items()):
                setattr(self.hoc_obj, k, v)
    someclass.__name__ = name
    return someclass

# ------------------------------------------------------------------------------
# Python equivalents to Hoc functions
# ------------------------------------------------------------------------------

xopen = h.xopen
quit = h.quit

def hoc_execute(hoc_commands, comment=None):
    assert isinstance(hoc_commands,list)
    if comment:
        logging.debug(comment)
    for cmd in hoc_commands:
        logging.debug(cmd)
        success = hoc.execute(cmd)
        if not success:
            raise HocError('Error produced by hoc command "%s"' % cmd)

def hoc_comment(comment):
    logging.debug(comment)

def psection(section):
    """
    function psection(section):

    Print info about section in a hoc format which is executable.
    (length, parent, diameter, membrane information)

    See:

    http://neuron.yale.edu/neuron/docs/help/neuron/neuron/nrnoc.html#psection

    """
    section.push()
    h.psection()
    h.pop_section()

def init():
    """
    function init():

    Initialize the simulation kernel.  This should be called before a run(tstop) call.

    Equivalent to hoc finitialize():

    http://neuron.yale.edu/neuron/docs/help/neuron/neuron/nrnoc.html#finitialize
    
    """
    h.finitialize()
    
def run(tstop):
    """
    function run(tstop)
    
    Run the simulation (advance the solver) until tstop [ms]

    """
    h('tstop = %g' % tstop)
    h('while (t < tstop) { fadvance() }')
    # what about pc.psolve(tstop)?

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
        _nrn_hocobj_ptr = nrn_dll_sym('nrn_hocobj_ptr')
        _nrn_hocobj_ptr.restype = ctypes.py_object    
        _double_ptr = ctypes.POINTER(ctypes.c_double)
        _double_size = ctypes.sizeof(ctypes.c_double)
    void_p = ctypes.cast(numpy_array.ctypes.data_as(_double_ptr), ctypes.c_voidp).value + index * _double_size
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
    import os
    if os.name == 'nt':
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
    import os
    if len(nt_dlls) is 0:
      b = 'bin'
      if h.nrnversion(8).find('i686') is 0:
        b = 'bin'
      path = os.path.join(h.neuronhome().replace('/','\\'), b)
      p = sys.version_info[0]*10 + sys.version_info[1]
      for dllname in ['nrniv.dll', 'libnrnpython%d.dll'%p]:
        p = os.path.join(path, dllname)
        nt_dlls.append(ctypes.cdll[p])
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
    raise Exception('unable to connect to the NEURON library containing '+name)

def nrn_dll(printpath=False):
    """Return a ctypes object corresponding to the NEURON library.
    
    .. warning::
    
        This provides access to the C-language internals of NEURON and should
        be used with care.
    """
    import ctypes
    import os
    import platform
    import glob

    try:
        #extended? if there is a __file__, then use that
        if printpath: print ("hoc.__file__ %s" % hoc.__file__)
        the_dll = ctypes.cdll[hoc.__file__]
        return the_dll        
    except:
      pass

    neuron_home = os.path.split(os.path.split(h.neuronhome())[0])[0]

    success = False
    if sys.platform == 'msys' or sys.platform == 'win32':
      p = 'hoc%d%d' % (sys.version_info[0], sys.version_info[1])
    else:
      p = 'hoc'
    base_path = os.path.join(neuron_home, 'lib' , 'python', 'neuron', p)
    for extension in ['', '.dll', '.so', '.dylib']:
        dlls = glob.glob(base_path + '*' + extension)
        for dll in dlls:
            try:
                the_dll = ctypes.cdll[dll]
                if printpath : print(dll)
                success = True
            except:
                pass
            if success: break
        if success: break
    else:
        raise Exception('unable to connect to the NEURON library')
    return the_dll

# TODO: put this someplace else
#       can't be in rxd because that would break things if no scipy
_sec_db = {}
def _declare_contour(secobj, secname):
    j = secobj.first
    center_vec = secobj.contourcenter(secobj.raw.getrow(0), secobj.raw.getrow(1), secobj.raw.getrow(2))
    x0, y0, z0 = [center_vec.x[i] for i in range(3)]    
    # (is_stack, x, y, z, xcenter, ycenter, zcenter)
    _sec_db[secname] = (True if secobj.contour_list else False, secobj.raw.getrow(0).c(j), secobj.raw.getrow(1).c(j), secobj.raw.getrow(2).c(j), x0, y0, z0)

def _create_all_list(obj):
    # used by import3d
    obj.all = []

def _create_sections_in_obj(obj, name, numsecs):
    # used by import3d to instantiate inside of a Python object
    setattr(obj, name, [h.Section(name="%s[%d]" % (name, i), cell=obj) for i in range(int(numsecs))])

def _connect_sections_in_obj(obj, childsecname, childx, parentsecname, parentx):
    # used by import3d
    childarray, childi = _parse_import3d_name(childsecname)
    parentarray, parenti = _parse_import3d_name(parentsecname)
    getattr(obj, childarray)[childi].connect(getattr(obj, parentarray)[parenti](parentx), childx)

def _parse_import3d_name(name):
    if '[' in name:
        import re
        array, i = re.search(r'(.*)\[(\d*)\]', name).groups()
        i = int(i)
    else:
        array = name
        i = 0
    return array, i

def _pt3dstyle_in_obj(obj, name, x, y, z):
    # used by import3d
    array, i = _parse_import3d_name(name)
    h.pt3dstyle(1, x, y, z, sec=getattr(obj, array)[i])

def _pt3dadd_in_obj(obj, name, x, y, z, d):
    array, i = _parse_import3d_name(name)
    h.pt3dadd(x, y, z, d, sec=getattr(obj, array)[i])


def numpy_from_pointer(cpointer, size):
    if sys.version_info.major < 3:
        return numpy.frombuffer(numpy.core.multiarray.int_asbuffer(
            ctypes.addressof(cpointer.contents),
            size * numpy.dtype(float).itemsize))
    else:
        buf_from_mem = ctypes.pythonapi.PyMemoryView_FromMemory
        buf_from_mem.restype = ctypes.py_object
        buf_from_mem.argtypes = (ctypes.c_void_p, ctypes.c_int, ctypes.c_int)
        cbuffer = buf_from_mem(
            cpointer, size * numpy.dtype(float).itemsize, 0x200)
        return numpy.ndarray((size,), numpy.float, cbuffer, order='C')


try:
  import ctypes
  import numpy
  import traceback
  vec_to_numpy_prototype = ctypes.CFUNCTYPE(ctypes.py_object, ctypes.c_int, ctypes.POINTER(ctypes.c_double))
  def vec2numpy(size, data):
    try:
      return numpy_from_pointer(data, size)
    except:
      traceback.print_exc()
      return None
  vec_to_numpy_callback = vec_to_numpy_prototype(vec2numpy)
  set_vec_as_numpy = nrn_dll_sym('nrnpy_set_vec_as_numpy')
  set_vec_as_numpy(vec_to_numpy_callback)
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
  #print 'neuron._pkl arg is ', arg
  return h.Vector(0)

def nrnpy_pr(s):
  sys.stdout.write(s.decode())
  return 0

if not embedded:
  try:
    # callback in place of hoc printf
    # ensures consistent with python stdout even with jupyter notebook.
    nrnpy_pr_proto = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_char_p)
    nrnpy_set_pr = nrn_dll_sym('nrnpy_set_pr')
    nrnpy_set_pr.argtypes = [nrnpy_pr_proto]

    nrnpy_pr_callback = nrnpy_pr_proto(nrnpy_pr)
    nrnpy_set_pr(nrnpy_pr_callback)
  except:
    print("Failed to setup nrnpy_pr")
    pass

