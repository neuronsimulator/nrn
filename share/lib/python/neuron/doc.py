"""
neuron.doc
===================

This module is used behind the scenes to generate docstrings of all HocObjects
of various types

To get general help on the neuron module try:

>>> import neuron
>>> help(neuron)

or in ipython

In []: import neuron
In []: neuron ?

From there, you can get help on the various objects in the hoc world:

In []: fom neuron import h
In []: v = h.Vector()
In []: ? v.to_python

a feature whose implementation is based on the neuron.doc module.

"""

import pydoc,sys,StringIO,inspect
from neuron import h


header = """
NEURON+Python Online Help System
================================

"""


# override basic helper functionality to give proper help on HocObjects
class NRNPyHelper(pydoc.Helper):

    def __call__(self, request=None):
        if type(request)==type(h):
            pydoc.pager(header+request.__doc__+"\n\n\n"+doc_asstring(request))
        else:
            pydoc.Helper.__call__(self,request)

help = NRNPyHelper(sys.stdin, sys.stdout)


def doc_asstring(thing, title='Python Library Documentation: %s', forceload=0):
    """return text documentation as a string, given an object or a path to an object."""
    try:
        object, name = pydoc.resolve(thing, forceload)
        desc = pydoc.describe(object)
        module = inspect.getmodule(object)
        if name and '.' in name:
            desc += ' in ' + name[:name.rfind('.')]
        elif module and module is not object:
            desc += ' in module ' + module.__name__
        if not (inspect.ismodule(object) or
                inspect.isclass(object) or
                inspect.isroutine(object) or
                inspect.isgetsetdescriptor(object) or
                inspect.ismemberdescriptor(object) or
                isinstance(object, property)):
            # If the passed object is a piece of data or an instance,
            # document its available methods instead of its value.
            object = type(object)
            desc += ' object'
        return title % desc + '\n\n' + pydoc.text.document(object, name)
    except (ImportError, ErrorDuringImport), value:
        print value




# override systemwide behaviour
pydoc.help = help




doc_h = """

neuron.h
========

neuron.h is the top-level HocObject, allowing interaction between python and hoc.

It is callable like a function, and takes Hoc code as an argument to be exectuted.

The top-level Hoc namespace is exposed as attributes to the h object.

Ex:

    >>> h = neuron.h
    >>> h("objref myvec")
    >>> h("myvec = new Vector(10)")
    >>> h.myvec.x[0]=1.0
    >>> h.myvec.printf()
    1       0       0       0       0
    0       0       0       0       0


Hoc classes are defined in the h namespace and can be constructed as follows:

>>> v = h.Vector(10)
>>> soma = h.Section()
>>> input = h.IClamp(soma(0.5))

More help on individual classes defined in Hoc and exposed in python
is available using ipython's online help feature

In []: ? h.Section

or in standard python by displaying the object docstring:

>>> print h.Section.__doc__

Using the python help system

>>> help(h.Vector)

yields only generic information about the HocObject class.



For a list of symbols defined in neuron.h try:

>>> dir(neuron.h)

Several hoc symbols are not useful in python, and thus raise an exception when accesed, for example:

In []: h.objref
---------------------------------------------------------------------------
TypeError                                 Traceback (most recent call last)

/home/emuller/hg/nrn_neurens_hg/<ipython console> in <module>()

TypeError: no HocObject interface for objref (hoc type 322)

In []: h.objref ?
Object `h.objref` not found.


"""


class_docstrings = dict(
    ExpSyn = """

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

""",
    Exp2Syn = """

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


""",
    SEClamp = """

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

""",
    VClamp = """

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

""",
    APCount = """

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

""",
    ParallelContext = """

class ParallelContext

"Embarrassingly" parallel computations using a Bulletin board style analogous to LINDA.

See:

http://www.neuron.yale.edu/neuron/docs/help/neuron/neuron/classes/parcon.html

""",
    NetStim = """

class NetStim

pointprocess

Generates a train of presynaptic stimuli. Can serve as the source for
a NetCon. This NetStim can also be be triggered by an input event. i.e
serve as the target of a NetCon.

See:

http://www.neuron.yale.edu/neuron/docs/help/neuron/neuron/mech.html#NetStim

""",
    Random = """

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

""",
    CVode = """

class CVode

Multi order variable time step integration method which may be
used in place of the default staggered fixed time step method.

See:

http://www.neuron.yale.edu/neuron/docs/help/neuron/neuron/classes/cvode.html

""",
    List = """

class List

List of objects

SYNTAX

List()
List("templatename")

DESCRIPTION

The List class provides useful tools for creating and manipulating
lists of objects. For example, if you have a network of cells
connected at synapses and each synapse is a separate object, you may
want to use lists to help organize the network. You could create one
list of all pre-synaptic connections and another of post-synaptic
connections, as well as a list of all the connecting cells.

List()

    Create an empty list. Objects added to the list are
    referenced. Objects removed from the list are unreferenced.

List("templatename")

    Create a list of all the object instances of the template. These
    object instances are NOT referenced and therefore the list
    dynamically changes as objects of template instances are created
    and destroyed. Adding and removing objects from this kind of list
    is not allowed. 

See:

http://www.neuron.yale.edu/neuron/docs/help/neuron/general/classes/list.html


""",
    Vector = """

class Vector

obj = Vector()
obj = Vector(size)
obj = Vector(size, init)
obj = Vector(obj)
  where obj is a sequence (tuple, list, iterator, etc.), or an object which exposes the array interface, like a numpy array.

Inplace operations

obj1 = numpy.zeros(10)
obj2 = Vector(10,1)
obj2.to_python(obj1)
  writes obj2 to obj1
obj2.from_python(obj1)
  fills obj2 with the contents of obj1

  
DESCRIPTION

The vector class provides convenient functions for manipulating
one-dimensional arrays of numbers. An object created with this class
can be thought of as containing a double x[] variable. Individual
elements of this array can be manipulated with the normal
objref.x[index] notation. Most of the Vector functions apply their
operations to each element of the x array thus avoiding the often
tedious scaffolding required by an otherwise un-encapsulated double
array.

A vector can be created with length size and with each element set to the value of init.

Vector methods that modify the elements are generally of the form

obj = vsrcdest.method(...)

in which the values of vsrcdest on entry to the method are used as
source values by the method to compute values which replace the old
values in vsrcdest and the original vsrcdest object reference is the
return value of the method. For example, v1 = v2 + v3 would be
written,

v1 = v2.add(v3)

However, this results in two, often serious, side effects. First, the
v2 elements are changed and so the original values are
lost. Furthermore v1 at the end is a reference to the same Vector
object pointed to by v2. That is, if you subsequently change the
elements of v2, the elements of v1 will change as well since v1 and v2
are in fact labels for the same object.

When these side effects need to be avoided, one uses the Vector.c
function which returns a reference to a completely new Vector which is
an identical copy. ie.

	v1 = v2.c.add(v3)

leaves v2 unchanged, and v1 points to a completely new Vector. One can
build up elaborate vector expressions in this manner, ie v1 = v2*s2 +
v3*s3 + v4*s4could be written

	v1 = v2.c.mul(s2).add(v3.c.mul(s3)).add(v4.c.mul(s4))

but if the expressions get too complex it is probably clearer to
employ temporary objects to break the process into several separate
expressions.

EXAMPLES

    vec = Vector(20,5)

will create a vector with 20 indices, each having the value of 5.

    vec1 = Vector()

will create a vector with 1 index which has value of 0. It is seldom
necessary to specify a size for a new vector since most operations, if
necessary, increase or decrease the number of available elements as
needed.

See:

    http://www.neuron.yale.edu/neuron/docs/help/neuron/general/classes/vector/vect.html#Vector

""",
    IClamp = """

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

""",
    NetCon = """

class NetCon(source, target, threshold=10, delay=1, weight=0, section=None, position=0.5)

A wrapper class for hoc NetCon objects.

See:

http://www.neuron.yale.edu/neuron/docs/help/neuron/neuron/classes/netcon.html#NetCon

""",
    File = """

class File

SYNTAX

fobj = File()
fobj = File("filename")

DESCRIPTION

This class allows you to open several files at once, whereas the top
level functions, ropen and wopen , allow you to deal only with one
file at a time.

The functionality of xopen is not implemented in this class so use

    	h("strdef tstr")
    	fobj.getname(h.tstr)
    	neuron.xopen(h.tstr)

to execute a sequence of interpreter commands in a file.

EXAMPLES

    f1 = File()		//state that f1, f2, and f3 are pointers to the File class
    f2 = File()
    f3 = File()
    f1.ropen("file1")	//open file1 for reading
    f2.wopen("file2")	//open file2 for writing
    f3.aopen("file3")	//open file3 for appending to the end of the file

opens file1, file2, and file3 for reading, writing, and appending (respectively).

BUGS

The mswindows/dos version must deal with the difference between binary
and text mode files. The difference is transparent to the user except
for one limitation. If you mix text and binary data and you write text
first to the file, then you need to do a File.seek(0) to explicitly
switch to binary mode just after opening the file and prior to the
first File.printf. An error message will occur if you read/write text
to a file in text mode and then try to read/write a binary
vector. This issue does not arise with the unix version.

See:

    http://www.neuron.yale.edu/neuron/docs/help/neuron/general/classes/file.html

""")
    
    
    

default_class_doc_template = """
No docstring available for class '%s'
A more extensive help for Hoc is available here:

http://www.neuron.yale.edu/neuron/docs/docs.html

most of which still applies for python.
"""


default_object_doc_template = """
No docstring available for object type '%s'
A more extensive help for Hoc is available here:

http://www.neuron.yale.edu/neuron/docs/docs.html

most of which still applies for python.
"""


default_member_doc_template = """
No docstring available for the class member '%s.%s'
A more extensive help for Hoc is available here:

http://www.neuron.yale.edu/neuron/docs/docs.html

most of which still applies for python.

==================================================

%s
    
"""




def get_docstring(objtype, symbol):
    """ Get the docstring for object-type and symbol.

    Ex:
    get_docstring('Vector','sqrt')

    returns a string

    """

    if (objtype,symbol)==('',''):

        return doc_h

    # are we asking for help on a class, e.g. h.Vector
    if objtype == '':
        return class_docstrings.get(symbol,default_class_doc_template % (symbol,) )

    # are we asking for help on a object, e.g. h.Vector()
    if symbol == '':
        return class_docstrings.get(objtype,default_object_doc_template % (objtype,) )


    # are we asking for help on a member of an object, e.g. h.Vector.size

    return default_member_doc_template % (objtype,symbol, get_docstring(objtype,"") )





