"""
This is an updated version of the file neuron/__init__.py which is distributed
with the NEURON distribution. The original file should be replaced with
(a symlink to) this one.

An attempt to make use of HocObjects more Pythonic, by wrapping them in Python
classes with the same names as the Hoc classes, and adding various typical
Python methods, such as __len__().

$Id: __init__.py,v 1.1 2008/05/26 11:39:44 hines Exp hines $
"""

try:
    import hoc
except ImportError:
    raise ImportError, "neuron.hoc module not found.\n \n Are you perhaps importing neuron in\n the nrnpython source directory? \n Please move out of this directory and try again,\n or remove nrnpython from your import path."
    

import nrn
h  = hoc.HocObject()

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
        m.append(b)
    if (len(m) > 1):
      raise TypeError('Multiple Inheritance of HocObject in %s' % name
        + ' through %s not allowed' % ','.join(b.__name__ for b in m))
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

def new_point_process(name):
    """
    Returns a Python-wrapped hoc class where the object needs to be associated
    with a section.
    """
    h('obfunc new_%s() { return new %s($1) }' % (name, name))
    class someclass(Wrapper):
        def __init__(self, section, position=0.5):
            assert 0 <= position <= 1
            section.push()
            self.__dict__['hoc_obj'] = getattr(h, 'new_%s' % name)(position) # have to put directly in __dict__ to avoid infinite recursion with __getattr__
            h.pop_section()
    someclass.__name__ = name
    return someclass

def new_hoc_class(name):
    """
    Returns a Python-wrapped hoc class where the object does not need to be
    associated with a section.
    """
    h('obfunc new_%s() { return new %s() }' % (name, name))
    class someclass(Wrapper):
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
    section.push()
    h.psection()
    h.pop_section()

def init():
    h.finitialize()
    
def run(tstop):
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

ExpSyn = new_point_process('ExpSyn')
Exp2Syn = new_point_process('Exp2Syn')
VClamp = new_point_process('VClamp')
SEClamp = new_point_process('SEClamp')
APCount = new_point_process('APCount')

ParallelContext = new_hoc_class('ParallelContext')
NetStim = new_hoc_class('NetStim')
Random = new_hoc_class('Random')
CVode = new_hoc_class('CVode')

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
    
    def __init__(self):
        self.hoc_obj = h.newlist()
        
    def __len__(self):
        return self.count()
    
    
class Vector(Wrapper):
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

