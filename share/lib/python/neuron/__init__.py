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


Important names and subpackages
---------------------

neuron.h

   The top-level hoc interpreter.

   Execute hoc commands by calling h with a string arguement:
   h('objref myobj')
   h('myobj = new Vector(10)')

   All hoc defined variables are accesible by attribute access to h.
   
   Example:

   print h.myobj.x[9]
   
  

$Id: __init__.py,v 1.1 2008/05/26 11:39:44 hines Exp hines $
"""

try:
    import hoc
except ImportError:
    raise ImportError, "neuron.hoc module not found.\n \n Are you perhaps importing neuron in\n the nrnpython source directory? \n Please move out of this directory and try again,\n or remove nrnpython from your import path."
    

import nrn
h  = hoc.HocObject()


def test():
    """ Runs a battery of unit tests on the neuron module."""
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

    arch_list = [platform.machine(), 'i686', 'x86_64', 'powerpc']

    for arch in arch_list:
        lib_path = os.path.join(path, arch, '.libs', 'libnrnmech.so')
        if os.path.exists(lib_path):
            h.nrn_load_dll(lib_path)
            nrn_dll_loaded.append(path)
            return
    print "NEURON mechanisms not found in %s." % path


import os
if 'NRN_NMODL_PATH' in os.environ:
    nrn_nmodl_path = os.environ['NRN_NMODL_PATH'].split(':')
    print 'NRN_NMODL_PATH=%s' % os.environ['NRN_NMODL_PATH']
    print 'Loading mechanisms...'
    for x in nrn_nmodl_path:
        print "From path: %s ..." % x
        load_mechanisms(x)
        print "Done."
    


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
# ------------------------------------------------------------------------------
# Python wrappers around Hoc objects
# For most objects we use a generic wrapper, created using the factory functions
#   `new_point_process()` or `new_hoc_class()`.
# Where we wish to modify the methods of the Hoc class, or add new methods,
#   we write an explicit Python class.
# ------------------------------------------------------------------------------

doc = """

class ExpSyn

pointprocesses

syn = ExpSyn(section, position=0.5)
syn.tau --- ms decay time constant
syn.e -- mV reversal potential
syn.i -- nA synaptic current

Description:
    
Synapse with discontinuous change in conductance at an event followed by an
exponential decay with time constant tau.

    i = G * (v - e)      i(nanoamps), g(micromhos);
      G = weight * exp(-t/tau)

The weight is specified by the weight field of a NetCon object.

This synapse summates. 

See:

http://www.neuron.yale.edu/neuron/docs/help/neuron/neuron/mech.html#ExpSyn

"""
ExpSyn = new_point_process('ExpSyn',doc=doc)


doc = """

class Exp2Syn

pointprocess

syn = Exp2Syn(section, position=0.5)
syn.tau1 --- ms rise time
syn.tau2 --- ms decay time
syn.e -- mV reversal potential
syn.i -- nA synaptic current

Description:
    
Two state kinetic scheme synapse described by rise time tau1, and decay time
constant tau2. The normalized peak condductance is 1. Decay time MUST be greater than rise time.

The kinetic scheme

    A    ->   G   ->   bath
       1/tau1   1/tau2

produces a synaptic current with alpha function like conductance (if tau1/tau2 is appoximately 1) defined by

    i = G * (v - e)      i(nanoamps), g(micromhos);
      G = weight * factor * (exp(-t/tau2) - exp(-t/tau1))

The weight is specified by the weight field of a NetCon object. The factor is defined so that
the normalized peak is 1. If tau2 is close to tau1 this has the property that the maximum value
is weight and occurs at t = tau1.

Because the solution is a sum of exponentials, the coupled equations for the kinetic
scheme can be solved as a pair of independent equations by the more efficient cnexp method.

This synapse summates. 

See:

http://www.neuron.yale.edu/neuron/docs/help/neuron/neuron/mech.html#Exp2Syn


"""
Exp2Syn = new_point_process('Exp2Syn',doc=doc)

doc = """

class VClamp

pointprocess

obj = VClamp(section,position=0.5)
dur[3]
amp[3]
gain, rstim, tau1, tau2
i

Description:
    
Two electrode voltage clamp. 

See:

http://www.neuron.yale.edu/neuron/docs/help/neuron/neuron/mech.html#VClamp

"""
VClamp = new_point_process('VClamp',doc=doc)

doc = """

class SEClamp

pointprocess

clampobj = SEClamp(section,position=0.5)
dur1 dur2 dur3 -- ms
amp1 amp2 amp3 -- mV
rs -- MOhm

vc -- mV
i -- nA

Description:
    
Single electrode voltage clamp with three levels. 

See:

http://www.neuron.yale.edu/neuron/docs/help/neuron/neuron/mech.html#SEClamp

"""
SEClamp = new_point_process('SEClamp',doc=doc)


doc = """

class APCount

pointprocess

apc = APCount(section, position=0.5)
apc.thresh --- mV
apc.n -- 
apc.time --- ms
apc.record(vector)

Description:
    
Counts the number of times the voltage at its location crosses a
threshold voltage in the positive direction. n contains the count and
time contains the time of last crossing.

If a Vector is attached to the apc, then it is resized to 0 when the
INITIAL block is called and the times of threshold crossing are
appended to the Vector. apc.record() will stop recording into the
vector. The apc is not notified if the vector is freed but this can be
fixed if it is convenient to add this feature.

See:

http://www.neuron.yale.edu/neuron/docs/help/neuron/neuron/mech.html#APcount

"""
APCount = new_point_process('APCount',doc=doc)


doc = """

class ParallelContext

"Embarrassingly" parallel computations using a Bulletin board style analogous to LINDA.

See:

http://www.neuron.yale.edu/neuron/docs/help/neuron/neuron/classes/parcon.html

"""
ParallelContext = new_hoc_class('ParallelContext',doc=doc)


doc = """

class NetStim

pointprocess

Generates a train of presynaptic stimuli. Can serve as the source for
a NetCon. This NetStim can also be be triggered by an input event. i.e
serve as the target of a NetCon.

See:

http://www.neuron.yale.edu/neuron/docs/help/neuron/neuron/mech.html#NetStim

"""
NetStim = new_hoc_class('NetStim',doc=doc)


doc = """

class Random

r = Random()
r.normal(0,5)
r.uniform(10,20)

The Random class provides commonly used random distributions which
are useful for stochastic simulations. The default distribution is
normal with mean = 0 and standard deviation = 1. 


See:
    
http://www.neuron.yale.edu/neuron/docs/help/neuron/general/classes/random.html#Random

Note:

For python based random number generation, numpy.random is an alternative to be considered.

"""
Random = new_hoc_class('Random',doc=doc)


doc = """

class CVode

Multi order variable time step integration method which may be
used in place of the default staggered fixed time step method.

See:

http://www.neuron.yale.edu/neuron/docs/help/neuron/neuron/classes/cvode.html

"""
CVode = new_hoc_class('CVode',doc=doc)

h('obfunc new_IClamp() { return new IClamp($1) }')
h('obfunc newlist() { return new List() }')
h('obfunc newvec() { return new Vector($1) }')
h('obfunc new_NetConO() { return new NetCon($o1, $o2, $3, $4, $5) }')
h('obfunc new_NetConP() { return new NetCon(&v($1), $o2, $3, $4, $5) }')
h('objref nil')
h('obfunc new_NetConO_nil() { return new NetCon($o1, nil) }')
h('obfunc new_NetConP_nil() { return new NetCon(&v($1), nil) }')
h('obfunc new_File() { return new File() }')

class List(Wrapper):
    """
    class List()
    
    A wrapper class for hoc List objects.
    """
    
    def __init__(self):
        self.hoc_obj = h.newlist()
        
    def __len__(self):
        return self.count()
    
    
class Vector(Wrapper):
    """
    class Vector(arg=10)
    
    A wrapper class for hoc Vector objects.

    arg - the length of the vector
    
    """
    n = 0
    
    def __init__(self,arg=10):
        self.name = 'vector%d' % Vector.n
        Vector.n += 1
        h('objref %s' % self.name)
        if isinstance(arg,int):
            h('%s = new Vector(%d)' % (self.name, arg))
            self.hoc_obj = getattr(h, self.name)
            return
        try:
            # passed iterable?
            arg_iter = iter(arg)
        except TypeError:
            raise TypeError("arg must be iteger or iterable")
        
        h('%s = new Vector(%d)' % (self.name, len(arg)))
        self.hoc_obj = getattr(h, self.name)
        for i,x in enumerate(arg_iter):
            self.x[i] = x
   
    def __len__(self):
        return self.size()

    # define the __array__ method
    # if numpy support is available
    if not hoc.test_numpy()==None:
        def __array__(self):
            return self.x.__array__()
   
    def __str__(self):
        tmp = self.printf()
        return ''
   
    def __repr__(self):
        tmp = self.printf()
        return ''

    # allow array(Vector())
    # Need Vector().toarray for speed though
    def __getitem__(self,i):
        return self.x[i]
   
    def __setitem__(self,i,y):
        self.x[i] = y

    def __getslice__(self,i,j):
        return [self.x[ii] for ii in xrange(i,j)]

    def __setslice__(self,i,j,y):
        assert(len(y)==j-i)

        iter = y.__iter__()

        for ii in xrange(i,j):
            self.x[ii] = iter.next()

    def tolist(self):
        return [self.x[i] for i in range(int(self.size()))]

    def toarray_slow(self):
        import numpy
        return numpy.array(self)

    def toarray_numpy(self):
        return self.x.toarray()

    # for array conversion,
    # use numpy support if available
    if hoc.test_numpy()==None:
        toarray = toarray_slow
    else:
        toarray = toarray_numpy

    def record(self, section, variable, position=0.5):
        #ref = h.ref(variable)
        #self.hoc_obj.record(ref)
        section.push()
        h('%s.record(&%s(%g))' % (self.name, variable, position))
        h.pop_section()                      

class IClamp(object):
    """

    class IClamp(section,position=0.5,delay=0,dur=0,amp=0)

    A wrapper class for the IClamp pointprocess

    Single pulse current clamp point process. This is an electrode current so positive
    amp depolarizes the cell. The current (i) is set to amp when t is within the closed
    interval delay to delay+dur. Time varying current stimuli can be simulated by setting delay=0,
    amp=1e9 and playing a vector into amp with the play Vector method.

    delay -- ms
    dur -- ms
    amp -- nA
    i -- nA

    See:

        http://www.neuron.yale.edu/neuron/docs/help/neuron/neuron/mech.html#IClamp

        http://www.neuron.yale.edu/neuron/docs/help/neuron/neuron/mech.html#pointprocesses

    Note:

        del was renamed to delay in Python to avoid name collision with the Python keyword "del".

    """
      
    def __init__(self, section, position=0.5, delay=0, dur=0, amp=0):
        assert 0 <= position <= 1
        section.push()
        self.hoc_obj = h.new_IClamp(position)
        h.pop_section()
        self.delay = delay
        self.dur = dur
        self.amp = amp
        
    def __getattr__(self, name):
        if name == "delay":
            return self.hoc_obj.__getattribute__('del')
        elif name in ('amp', 'dur'):
            return self.hoc_obj.__getattribute__(name)
        else:
            return self.__getattribute__(name)
     
    def __setattr__(self, name, value):
        if name == "delay":
            self.hoc_obj.__setattr__('del', value)
        elif name in ('amp', 'dur'):
            self.hoc_obj.__setattr__(name, value)
        else:
            object.__setattr__(self, name, value)


class NetCon(Wrapper):
    """

    class NetCon(source, target, threshold=10, delay=1, weight=0, section=None, position=0.5)

    A wrapper class for hoc NetCon objects.

    See:

    http://www.neuron.yale.edu/neuron/docs/help/neuron/neuron/classes/netcon.html#NetCon

    """
    
    def __init__(self, source, target, threshold=10, delay=1, weight=0, section=None, position=0.5):
        # What about generalising to obtain the NetCon object via pc.gid_connect(), if source is
        # an integer?
        if hasattr(target, 'hoc_obj'):
           target = target.hoc_obj
        if target is None:
            if section:
                section.push()
                self.hoc_obj = h.new_NetConP_nil(position)
                h.pop_section()
            else:
                self.hoc_obj = h.new_NetConO_nil(source.hoc_obj)
        else:
            if section:
                section.push()
                self.hoc_obj = h.new_NetConP(position, target, threshold, delay, weight)
                h.pop_section()
            else:
                self.hoc_obj = h.new_NetConO(source.hoc_obj, target, threshold, delay, weight)
        self.section = section

        
class File(Wrapper):
    """Hoc file object with Python-like syntax added."""
    
    def __init__(self, name, mode='r'):
        assert mode in ('r', 'w', 'a')
        self.hoc_obj = h.new_File()
        open_func = getattr(self.hoc_obj, "%sopen" % mode)
        open_func(name)
        
    def write(self, s):
        pass

