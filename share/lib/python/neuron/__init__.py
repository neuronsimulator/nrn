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
version = h.nrnversion(5)
__version__ = version
_original_hoc_file = None
if not hasattr(hoc, "__file__"):
  import platform
  import os
  p = h.nrnversion(6)
  if "--prefix=" in p:
    p = p[p.find('--prefix=') + 9:]
    p = p[:p.find("'")]
  else:
    p = "/usr/local/nrn"
  if sys.version_info >= (3, 0):
    import sysconfig
    phoc = p + "/lib/python/neuron/hoc%s" % sysconfig.get_config_var('SO')
  else:
    phoc = p + "/lib/python/neuron/hoc.so"
  if not os.path.isfile(phoc):
    phoc = p + "/%s/lib/libnrnpython%d.so" % (platform.machine(), sys.version_info[0])
  if not os.path.isfile(phoc):
    phoc = p + "/%s/lib/libnrnpython.so" % platform.machine()
  setattr(hoc, "__file__", phoc)
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

def test():
    """ Runs a global battery of unit tests on the neuron module."""
    import neuron.tests
    import unittest

    runner = unittest.TextTestRunner(verbosity=2)
    runner.run(neuron.tests.suite())

def test_rxd():
    """ Runs a tests on the rxd and crxd modules."""
    import neuron.tests
    import unittest

    runner = unittest.TextTestRunner(verbosity=2)
    runner.run(neuron.tests.test_rxd.suite())


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

    The path should specify the directory in which nrnivmodl or mknrndll was run,
    and in which the directory 'i686' (or 'x86_64' or 'powerpc' depending on your platform)
    was created"""

    import platform
    
    global nrn_dll_loaded
    if path in nrn_dll_loaded:
        print("Mechanisms already loaded from path: %s.  Aborting." % path)
        return True
    
    # in case NEURON is assuming a different architecture to Python,
    # we try multiple possibilities

    libname = 'libnrnmech.so'
    libsubdir = '.libs'
    arch_list = [platform.machine(), 'i686', 'x86_64', 'powerpc', 'umac']

    # windows loads nrnmech.dll
    if h.unix_mac_pc() == 3:
        libname = 'nrnmech.dll'
        libsubdir = ''
        arch_list = ['']

    for arch in arch_list:
        lib_path = os.path.join(path, arch, libsubdir, libname)
        if os.path.exists(lib_path):
            h.nrn_load_dll(lib_path)
            nrn_dll_loaded.append(path)
            return True
    print("NEURON mechanisms not found in %s." % path)
    return False

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
    
    Use section.psection() instead to get a data structure that
    contains the same information and more.

    See:

    https://www.neuron.yale.edu/neuron/static/py_doc/modelspec/programmatic/topology.html?#psection

    """
    h.psection(sec=section)

def init():
    """
    function init():

    Initialize the simulation kernel.  This should be called before a run(tstop) call.

    Use h.finitialize() instead, which allows you to specify the membrane potential
    to initialize to; via e.g. h.finitialize(-65)

    https://www.neuron.yale.edu/neuron/static/py_doc/simctrl/programmatic.html?#finitialize
    
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
        if printpath: print ("hoc.__file__ %s" % _original_hoc_file)
        the_dll = ctypes.cdll[_original_hoc_file]
        return the_dll        
    except:
        pass


    success = False
    if sys.platform == 'msys' or sys.platform == 'win32':
      p = 'hoc%d%d' % (sys.version_info[0], sys.version_info[1])
    else:
      p = 'hoc'

    try:
        # maybe hoc.so in this neuron module
        base_path = os.path.join(os.path.split(__file__)[0], p)
        dlls = glob.glob(base_path + '*.*')
        for dll in dlls:
            try:
                the_dll = ctypes.cdll[dll]
                if printpath : print(dll)
                return the_dll
            except:
                pass
    except:
        pass
    # maybe old default module location
    neuron_home = os.path.split(os.path.split(h.neuronhome())[0])[0]
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


class _WrapperPlot:
  def __init__(self, data):
    '''do not call directly'''
    self._data = data
  def __repr__(self):
    return '{}.plot()'.format(repr(self._data))

class _RangeVarPlot(_WrapperPlot):
  """Plots the current state of the RangeVarPlot on the graph.

  Additional arguments and keyword arguments are passed to the graph's
  plotting method.

  Example, showing plotting to NEURON graphics, bokeh, and matplotlib:

  .. code::

    from matplotlib import pyplot
    from neuron import h, gui
    import bokeh.plotting as b
    import math

    dend = h.Section(name='dend')
    dend.nseg = 55
    dend.L = 6.28

    # looping over dend.allseg instead of dend to set 0 and 1 ends
    for seg in dend.allseg():
        seg.v = math.sin(dend.L * seg.x)

    r = h.RangeVarPlot('v', dend(0), dend(1))

    # matplotlib
    graph = pyplot.gca()
    r.plot(graph, linewidth=10, color='r')

    # NEURON Interviews graph
    g = h.Graph()
    r.plot(g, 2, 3)
    g.exec_menu('View = plot')

    # Bokeh
    bg = b.Figure()
    r.plot(bg, line_width=10)
    b.show(bg)

    pyplot.show()"""
    
  def __call__(self, graph, *args, **kwargs):      
      yvec = h.Vector()
      xvec = h.Vector()
      self._data.to_vector(yvec, xvec)
      if isinstance(graph, hoc.HocObject):
        return yvec.line(graph, xvec, *args)
      if hasattr(graph, 'plot'):
        # works with e.g. pyplot or a matplotlib axis
        return graph.plot(xvec, yvec, *args, **kwargs)
      if hasattr(graph, 'line'):
        # works with e.g. bokeh
        return graph.line(xvec, yvec, *args, **kwargs)
      if str(type(graph)) == "<class 'matplotlib.figure.Figure'>":
        raise Exception('plot to a matplotlib axis not a matplotlib figure')
      raise Exception('Unable to plot to graphs of type {}'.format(type(graph)))


class _PlotShapePlot(_WrapperPlot):
  '''Plots the currently selected data on an object.

  Currently only pyplot is supported, e.g.

  from matplotlib import pyplot
  ps = h.PlotShape(False)
  ps.variable('v')
  ps.plot(pyplot)
  pyplot.show()

  Limitations: many. Currently only supports plotting a full cell colored based on a variable.'''
  # TODO: handle pointmark, specified sections, color
  def __call__(self, graph, *args, **kwargs):
    def _get_pyplot_axis3d(fig):
      '''requires matplotlib'''
      from matplotlib.pyplot import cm
      import matplotlib.pyplot as plt
      from mpl_toolkits.mplot3d import Axes3D
      import numpy as np
      from neuron.gui2.utilities import _segment_3d_pts

      class Axis3DWithNEURON(Axes3D):
          def auto_aspect(self):
              """sets the x, y, and z range symmetric around the center

              Probably needs a square figure to preserve lengths as you rotate."""
              bounds = [self.get_xlim(), self.get_ylim(), self.get_zlim()]
              half_delta_max = max([(item[1] - item[0]) / 2 for item in bounds])
              xmid = sum(bounds[0]) / 2
              ymid = sum(bounds[1]) / 2
              zmid = sum(bounds[2]) / 2
              self.auto_scale_xyz([xmid - half_delta_max, xmid + half_delta_max],
                                  [ymid - half_delta_max, ymid + half_delta_max],
                                  [zmid - half_delta_max, zmid + half_delta_max])

          def mark(self, segment, marker='or', **kwargs):
              """plot a marker on a segment
              
              Args:
                  segment = the segment to mark
                  marker = matplotlib marker
                  **kwargs = passed to matplotlib's plot
              """
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
              self.plot([x], [y], [z], marker)
              return self



          def _do_plot(self, val_min, val_max,
                      sections,
                      variable,
                      cmap=cm.cool,
                      **kwargs):
              """
              Plots a 3D shapeplot
              Args:
                  sections = list of h.Section() objects to be plotted
                  **kwargs passes on to matplotlib (e.g. linewidth=2 for thick lines)
              Returns:
                  lines = list of line objects making up shapeplot
              """
              # Adapted from
              # https://github.com/ahwillia/PyNeuron-Toolbox/blob/master/PyNeuronToolbox/morphology.py
              # Accessed 2019-04-11, which had an MIT license
              
              # Default is to plot all sections. 
              if sections is None:
                  sections = list(h.allsec())

              h.define_shape()
              
              # default color is black
              kwargs.setdefault('color', 'black')

              # Plot each segement as a line
              lines = {}
              lines_list = []
              vals = []
              for sec in sections:
                  all_seg_pts = _segment_3d_pts(sec)
                  for seg, (xs, ys, zs, _, _) in zip(sec, all_seg_pts):
                      line, = self.plot(xs, ys, zs, '-', **kwargs)
                      if variable is not None:
                          try:
                              if '.' in variable:
                                  mech, var = variable.split('.')
                                  val = getattr(getattr(seg, mech), var)
                              else:
                                  val = getattr(seg, variable)
                          except AttributeError:
                              # leave default color if no variable found
                              val = None
                          vals.append(val)
                      lines[line] = '%s at %s' % (val, seg)
                      lines_list.append(line)
              if variable is not None:
                  val_range = val_max - val_min
                  if val_range:
                      for sec in sections:
                          for line, val in zip(lines_list, vals):
                              if val is not None:
                                  col = cmap(int(255 * (val - val_min) / (val_range)))
                                  line.set_color(col)
              return lines
      return Axis3DWithNEURON(fig)
    

    
    def _do_plot_on_matplotlib_figure(fig):
      import ctypes
      get_plotshape_data = nrn_dll_sym('get_plotshape_data')
      get_plotshape_data.restype = ctypes.py_object
      variable, lo, hi, secs = get_plotshape_data(ctypes.py_object(self._data))
      kwargs.setdefault('picker', 2)
      result = _get_pyplot_axis3d(fig)
      _lines = result._do_plot(lo, hi, secs, variable, *args, **kwargs)
      result._mouseover_text = ''
      def _onpick(event):
        if event.artist in _lines: 
          result._mouseover_text = _lines[event.artist]
        else:
          result._mouseover_text = ''
        return True
      result.auto_aspect()
      fig.canvas.mpl_connect('pick_event', _onpick)
      def format_coord(*args):
        return result._mouseover_text
      result.format_coord = format_coord
      return result
    

    if hasattr(graph, '__name__') and graph.__name__ == 'matplotlib.pyplot':
      fig = graph.figure()
      return _do_plot_on_matplotlib_figure(fig)
    elif str(type(graph)) == "<class 'matplotlib.figure.Figure'>":
      return _do_plot_on_matplotlib_figure(graph)
    else:
      raise NotImplementedError

try:
  import ctypes
  def _rvp_plot(rvp):
    return _RangeVarPlot(rvp)

  def _plotshape_plot(ps):
    return _PlotShapePlot(ps)


  set_graph_plots = nrn_dll_sym('nrnpy_set_graph_plots')
  _rvp_plot_callback = ctypes.py_object(_rvp_plot)
  _plotshape_plot_callback = ctypes.py_object(_plotshape_plot)
  set_graph_plots(_rvp_plot_callback, _plotshape_plot_callback)
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

def nrnpy_pass():
  return 1

def nrnpy_pr(stdoe, s):
  if stdoe == 1:
    sys.stdout.write(s.decode())
  else:
    sys.stderr.write(s.decode())
  return 0

if not embedded:
  try:
    # nrnpy_pr callback in place of hoc printf
    # ensures consistent with python stdout even with jupyter notebook.
    # nrnpy_pass callback used by h.doNotify() in MINGW when not called from
    # gui thread in order to allow the gui thread to run.

    nrnpy_set_pr_etal = nrn_dll_sym('nrnpy_set_pr_etal')

    nrnpy_pr_proto = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_int, ctypes.c_char_p)
    nrnpy_pass_proto = ctypes.CFUNCTYPE(ctypes.c_int)
    nrnpy_set_pr_etal.argtypes = [nrnpy_pr_proto, nrnpy_pass_proto]

    nrnpy_pr_callback = nrnpy_pr_proto(nrnpy_pr)
    nrnpy_pass_callback = nrnpy_pass_proto(nrnpy_pass)
    nrnpy_set_pr_etal(nrnpy_pr_callback, nrnpy_pass_callback)
  except:
    print("Failed to setup nrnpy_pr")
    pass


def nrnpy_vec_math(op, flag, arg1, arg2=None):
  import numbers
  valid_types = (numbers.Number, hoc.HocObject)
  if isinstance(arg1, valid_types):
    if flag == 2:
      # unary
      arg1 = arg1.c()
      if op == 'uneg':
        return arg1.mul(-1)
      if op == 'upos':
        return arg1
      if op == 'uabs':
        return arg1.abs()
    elif isinstance(arg2, valid_types):
      if flag == 1:
        # either reversed (flag=1) or unary (flag=2)
        arg2 = arg2.c()
        if op in ('mul', 'add'):
          return getattr(arg2, op)(arg1)
        if op == 'div':
          return arg2.pow(-1).mul(arg1)
        if op == 'sub':
          return arg2.mul(-1).add(arg1)
      else:
        arg1 = arg1.c()
        return getattr(arg1, op)(arg2)

  return NotImplemented

try:
  nrnpy_vec_math_register = nrn_dll_sym('nrnpy_vec_math_register')
  nrnpy_vec_math_register(ctypes.py_object(nrnpy_vec_math))
except:
  print("Failed to setup nrnpy_vec_math")

try:
  from neuron.psection import psection
  nrn.set_psection(psection)
except:
  print("Failed to setup nrn.Section.psection")
pass
