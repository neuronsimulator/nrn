"""

neuron
======

For empirically-based simulations of neurons and networks of neurons in Python.

This is the top-level module of the official python interface to
the NEURON simulation environment (http://www.neuron.yale.edu/neuron/).

Documentation is available in the docstrings.

For a list of available names, try dir(neuron).

Example:

$ ipython
In [1]: import neuron
NEURON -- VERSION 6.2 2008-08-22
Duke, Yale, and the BlueBrain Project -- Copyright 1984-2007
See http://www.neuron.yale.edu/credits.html


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

try:
  import hoc
except:
  #Python3.1 extending needs to look into the module explicitly
  import neuron.hoc

import nrn
h  = hoc.HocObject()


# As a workaround to importing doc at neuron import time
# (which leads to chicken and egg issues on some platforms)
# define a dummy help function which imports doc,
# calls the real help function, and reassigns neuron.help to doc.help
# (thus replacing the dummy)
def help(request):
    global help
    print "Enabling NEURON+Python help system."
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

class MetaHocObject(type):
  """Provides Exception for Inheritance of multiple HocObject"""
  def __new__(cls, name, bases, attrs):
    #print cls, name, bases
    m = []
    for b in bases:
      if issubclass(b, hoc.HocObject):
        m.append(b.__name__)
    if (len(m) > 1):
      raise TypeError('Multiple Inheritance of HocObject in %s' % name
        + ' through %s not allowed' % ','.join(m))
      #note that join(b.__name__ for b in m) is not valid for Python 2.3
    return type.__new__(cls, name, bases, attrs)

def hclass(c):
    """Class factory for subclassing h.anyclass. E.g. class MyList(hclass(h.List)):..."""
    if c == h.Section :
        return nrn.Section
    class hc(hoc.HocObject):
        __metaclass__ = MetaHocObject
        def __new__(cls, *args, **kwds):
            kwds.update({'hocbase':cls.htype})
            return hoc.HocObject.__new__(cls, *args, **kwds)
    setattr(hc, 'htype', c)
    return hc


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
        print "Mechanisms already loaded from path: %s.  Aborting." % path
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
    print "NEURON mechanisms not found in %s." % path


import os,sys
if 'NRN_NMODL_PATH' in os.environ:
    nrn_nmodl_path = os.environ['NRN_NMODL_PATH'].split(':')
    print 'Auto-loading mechanisms:'
    print 'NRN_NMODL_PATH=%s' % os.environ['NRN_NMODL_PATH']
    for x in nrn_nmodl_path:
        #print "from path %s:" % x
        load_mechanisms(x)
        #print "\n"
    print "Done.\n"
    


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
            for k,v in kwargs.items():
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

    http://www.neuron.yale.edu/neuron/docs/help/neuron/neuron/nrnoc.html#psection

    """
    section.push()
    h.psection()
    h.pop_section()

def init():
    """
    function init():

    Initialize the simulation kernel.  This should be called before a run(tstop) call.

    Equivalent to hoc finitialize():

    http://www.neuron.yale.edu/neuron/docs/help/neuron/neuron/nrnoc.html#finitialize
    
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


