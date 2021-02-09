.. _python:

.. warning::

    Some of the idioms on this page are out of date, but they still work.
    See the NEURON Python tutorial for modern idioms.

Python Language
---------------

This document describes installation and basic use of NEURON's Python interface. For information on the modules in the ``neuron`` namespace, see:

.. toctree:: :maxdepth: 1

    neuronpython.rst


Installation
~~~~~~~~~~~~


Syntax:
    ``./configure --with-nrnpython ...``

    ``make``

    ``make install``


Description:
    Builds NEURON with Python embedded as an alternative interpreter to HOC. 
    The Python version used is that found from ``which python``. 

    "make install" automatically does a (``cd src/nrnpython ; python setup.py install --home=<prefix>``)
    which generally puts the neuron module under <prefix>/lib/python. *Note:* This
    path then needs to be added to your ``PYTHONPATH`` environment variable.
    To install in the default system Python directory, manually do the ``cd`` and run
    ``python setup.py install`` (possibly with a ``sudo``). Be advised that this
    installs only for one version of Python, so if you use both Python 2 and 3,
    keep reading.

    Other python related configure arguments are:
      ``--enable-pysetup=installoption``
                              Execute 'python setup.py install installoption' as
                              the last installation step. --disable-pysetup or an
                              installoption of 'no' means do NOT execute 'python
                              setup.py...' The default installoption is
                              '--home=<prefix>'
                              
      ``--disable-rx3d``          Do not compile the cython translated 3-d rxd
                              features
                              
      ``--with-nrnpython-only``   configure and make only the nrnpython folder

      ``--with-nrnpython=``desired python binary or ``dynamic``
                              Python interpreter can be used (default is NO)
                              Probably need to set PYLIBDIR to find libpython...
                              and PYINCDIR to find Python.h

      ``--with-pyexe``=desired python binary (when ``--with-nrnpython=dynamic``)

      ``--disable-pysetup``   skips the automatic invocation of ``python setup.py``;
                              to use Python, this will have to be done manually.



    I use this script to build a version of NEURON that works with python2.5-7 and python3.5-6

    .. code-block::
        console

        hines@hines-T7500:~/neuron$ cat bld-nrndynam.sh
        #!/bin/bash

        cd $HOME/neuron/nrndynam

        ../nrn/configure --prefix=`pwd` --with-paranrn=dynamic \
          --with-nrnpython=dynamic --with-pyexe=python3 --disable-rx3d
        rm -r -f src/nrnpython/build
        make -j 6 install

        ../nrn/configure --prefix=`pwd` --with-paranrn=dynamic \
          --with-nrnpython=dynamic --with-pyexe=python2 --disable-rx3d \
          --with-nrnpython-only
        rm -r -f src/nrnpython/build
        make -j 6 install

        cd lib/python/neuron
        cp hoc.cpython-35m-x86_64-linux-gnu.so hoc.cpython-36m-x86_64-linux-gnu.so
        cd ../../..
        # or alternatively configure ..; rm...; make... with other versions for --with-pyexe

    Originally I thought that ``nrniv -python ...`` was the best way to launch (and perhaps is
    for parallel and on many supercomputers). Now, especially with the possibility of many python versions and
    one (--with-nrnpython=dynamic) version is it is simpler to launch the correct python with only the necessity
    of a uniform ``export PYTHONPATH=<prefix>/lib/python`` (and it is then a bad idea to install the neuron module
    in a specific python's site-packages). This gives the possibility of
    python foo.py
    python2 foo.py
    python3.5 foo.py
    python3.6 foo.py
    Whereas launching ``nrniv`` would require an environment with the proper ``PYTHONHOME`` and ``PYNRNLIB``
    The former to avoid the dreaded "site" problem and the latter to ensure the right python library is loaded.
    In vicious cases of failure to launch due to site problems, the first diagnostic info needed to fix the
    problem is
    
    .. code-block::
        python

        import site
        print (site.__file__)

    For launching ``nrniv``,there is a script nrnpyenv.sh that helps determine a correct PYTHONHOME, etc for the
    default ``python`` executable.

    If launching Python and using parallel neuron simulations there is the issue of who calls MPI_Initial.
    Default assumes Python will call it (perhaps via ``from mpi4py import MPI``). If that is not the case, force the
    neuron module to initialize MPI when the neuron module is loaded by setting the environment variable
    NEURON_INIT_MPI

    If you just can't seem to get nrniv to launch and you don't need Python, you can try
    ``nrniv -nopython ...``
    or permanently add
    ``nopython``: on
    to $HOME/.nrn.defaults or :file:`<prefix>/share/nrn/lib/nrn.defaults` (c:/nrn/lib/nrn.def on windows)

    You can use the hoc function :func:`nrn_load_dll` to load mechanism files 
    as well, e.g. if neurondemo was used earlier so the shared object exists, 

    .. code-block::
        python

        from neuron import h
        h.nrn_load_dll("$(NEURONHOME)/demo/release/x86_64/.libs/libnrnmech.so")


.. _python_accessing_hoc:

Python Accessing HOC
~~~~~~~~~~~~~~~~~~~~



Syntax:
    ``nrniv -python [file.hoc file.py  -c "python_statement"]``

    ``nrngui -python ...``

    ``neurondemo -python ...``


Description:
    Launches NEURON with Python as the command line interpreter. 
    File arguments with a .hoc suffix are interpreted using the 
    Hoc interpreter. File arguments with the .py suffix are interpreted 
    using the Python interpreter. The -c statement causes python to 
    execute the statement. 
    The import statements allow use of the following 

         

----

.. note::

    Most of the following is from the perspective of someone familiar
    with HOC; for a Python-based introduction to NEURON, see
    http://neuron.yale.edu/neuron/static/docs/neuronpython/index.html


.. class:: neuron.hoc.HocObject


    Syntax:
        ``from neuron import h``

        ``h = neuron.hoc.HocObject()``


    Description:
        
        Allow access to anything in the Hoc interpreter. 
        ``h`` is an instance of a ``neuron.hoc.HocObject`` object. Note that
        there is only one Hoc interpreter, no matter how many interface
        objects are created, so there is no advantage to creating another.

        .. code-block::
            python

            h("any hoc statement") 
         
        Any hoc variable or string in the Hoc world can be accessed 
        in the Python world: 

        .. code-block::
            python

            h('strdef s') 
            h('{x = 3  s = "hello"}') 
            print(h.x)          # prints 3.0 
            print(h.s)          # prints hello 

        And if it is assigned a value in the python world it will be that value 
        in the Hoc world. (Note that any numeric python type becomes a double 
        in Hoc.) 

        .. code-block::
            python

            h.x = 25 
            h.s = 'goodbye' 
            h('print x, s')    #prints 25 goodbye 

        Note, however, that new Hoc variables cannot be defined from Python except via, e.g.
        ``h('strdef s')``.


        Any hoc object can be handled in Python, and can use Python idioms for that type of
        object despite being created in hoc. e.g. in hoc, you would have to use vec.size() to
        get the Vector's size. This still works in Python, but you can also use the Pythonic
        len(h.vec): 

        .. code-block::
            python

            h('objref vec') 
            h('vec = new Vector(5)') 
            print(h.vec)        # prints Vector[0] 
            print(len(h.vec))   # prints 5.0 

        There is, however, in pure Python models never a need to create a hoc object;
        e.g. if no HOC code needed to access the :class:`Vector`, the above is equivalent to

        .. code-block::
            python

            vec = h.Vector(5)
            print(vec)
            print(len(vec))

        Note that any hoc object method or field may be called, or evaluated/assigned 
        using the normal dot notation which is consistent between hoc and python. 
        However, hoc object methods MUST have the parentheses or else the Python 
        object is not the return value of the method but a method object. ie. 

        .. code-block::
            python

            x = h.vec.size     # not 5 but a python callable object 
            print(x)            # prints: Vector[0].size() 
            print(x())          # prints 5

        This is also true for indices 

        .. code-block::
            python

            h.vec.indgen().add(10) # fills elements with 10, 11, ..., 14 
            print(h.vec[2])    # prints 12.0 
            x = h.vec.x        # a python indexable object 
            print(x)           # prints Vector[0].x[?] 
            print(x[2])        # prints 12.0 

        Note that the .x notation is not needed in Python for reading or (as of NEURON 7.7) writing to vectors.

        The hoc object can be created directly in Python. E.g. 

        .. code-block::
            python

            v = h.Vector(range(10, 20)) 

         
        Iteration over hoc Vector, List, and arrays is supported. e.g. 

        .. code-block::
            python

            v = h.Vector(range(10, 14)) 
            for x in v: 
              print(x)
             
            l = h.List(); l.append(v); l.append(v); l.append(v) 
            for x in l: 
              print(x)
             
            h('objref o[2][3]') 
            for x in h.o: 
              for y in x: 
                print(x, y)
             

         
        Any hoc Section can be handled in Python. E.g. 

        .. code-block::
            python

            h('create soma, axon') 
            ax = h.axon 

        makes ax a Python :class:`~neuron.h.Section` which references the hoc 
        axon section. Many hoc functions use the currently accessed section;
        most of these are now available as section methods, however for user
        written hoc and in legacy code, a "sec" keyword parameter temporarily
        makes the Section value the currently accessed section during 
        the scope of the function call. e.g 

        .. code-block::
            python

            print(h.secname(sec=ax))
        
        .. note::

            In Python, one can simply:

            .. code-block::
                python

                print(ax)
            
            Or use ``str(ax)`` to get the name of the section ax.

        Most such functions now have an alternative form that avoids the need for
        sec=; often they are available as section methods. This is usually listed
        in the function definition.

         
        Point processes are handled by direct object creation as in 

        .. code-block::
            python

            stim = IClamp(ax(1.0)) 
        
        Many hoc functions use call by reference and return information by 
        changing the value of an argument. These are called from the Python 
        world by passing a HocObject.ref() object. Here is an example that 
        changes a string. 

        .. code-block::
            python

            h('proc chgstr() { $s1 = "goodbye" }') 
            s = h.ref('hello') 
            print(s[0])          # notice the index to dereference. prints hello 
            h.chgstr(s) 
            print(s[0])          # prints goodbye 
            h.sprint(s, 'value is %d', 2+2) 
            print(s[0])          # prints value is 4 

        and here is an example that changes a pointer to a double 

        .. code-block::
            python

            h('proc chgval() { $&1 = $2 }') 
            x = h.ref(5) 
            print(x[0])          # prints 5.0 
            h.chgval(x, 1+1) 
            print(x[0])          # prints 2.0 

        Finally, here is an example that changes a objref arg. 

        .. code-block::
            python

            h('proc chgobj() { $o1 = new List() }') 
            v = h.ref([1,2,3])  # references a Python object 
            print(v[0])          # prints [1, 2, 3] 
            h.chgobj(v) 
            print(v[0])          # prints List[0] 

        Unfortunately, the HocObject.ref() is not often useful since it is not really 
        a pointer to a variable. For example consider 

        .. code-block::
            python

            h('x = 1') 
            y = h.ref(h.x) 
            print(y)                     # prints hoc ref value 1 
            print('%g %g' % (h.x, y[0])) # prints 1.0 1.0 
            h.x = 2 
            print('%g %g' % (h.x, y[0])) # prints 2.0 1.0 
            
        and thus in not what is needed in the most common 
        case of a hoc function holding a pointer to a variable such as 
        :meth:`Vector.record` or :meth:`Vector.play`. For this one needs the :samp:`_ref_{varname}` idiom 
        which works for any hoc variable and acts exactly like a c pointer. eg: 

        .. code-block::
            python

            h('x = 1') 
            y = h._ref_x 
            print(y)                     # prints pointer to hoc value 1
            print('%g %g' % (h.x, y[0])) # prints 1.0 1.0 
            h.x = 2 
            print('%g %g' % (h.x, y[0])) # prints 2.0 2.0 
            y[0] = 3 
            print('%g %g' % (h.x, y[0])) # prints 3.0 3.0 

        Of course, this works only for hoc variables, not python variables.  For 
        arrays, use all the index arguments and prefix the name with _ref_.  The 
        pointer will be to the location indexed and one may access any element 
        beyond the location by giving one more non-negative index.  No checking 
        is done with regard to array bounds errors.  e.g 

        .. code-block::
            python

            v = h.Vector(range(10, 14)) 
            y = v._ref_x[1]    # holds pointer to second element of v 
            print('%g %g' % (v[2], y[1])) # prints 12.0 12.0 
            y[1] = 50 
            v.printf()         # prints 10 11 50 13 

        The idiom is used to record from (or play into) voltage and mechanism variables. eg 

        .. code-block::
            python

            from neuron import h
            soma = h.Section(name='soma')
            soma.insert('pas')
            v = h.Vector().record(soma(0.5)._ref_v)
            pi = h.Vector().record(soma(0.5).pas._ref_i)
            ip = h.Vector().record(soma(0.5)._ref_i_pas)

         
        The factory idiom is one way to create Hoc objects and use them 
        in Python. 

        .. code-block::
            python

            h('obfunc newvec() { return new Vector($1) }') 
            v = h.newvec(10).indgen().add(10) 
            v.printf()          # prints 10 11 ... 19 (not 10.0 ... since printf is a hoc function) 

        but that idiom is more or less obsolete as the same thing can be accomplished 
        directly as shown a few fragments back. Also consider the minimalist 

        .. code-block::
            python

            vt = h.Vector 
            v = vt(4).indgen().add(10) 

        Any Python object can be stored in a Hoc List. It is more efficient 
        when navigating the List to use a python callable that avoids repeated 
        lookup of a Hoc method symbol. Note that in the Hoc world a python object 
        is of type PythonObject but python strings and scalars are translated back 
        and forth as strdef and scalar doubles respectively. 

        .. code-block::
            python

            h('obfunc newlist() { return new List() }') 
            my_list = h.newlist() 
            apnd = my_list.append 
            apnd([1,2,3])      # Python list in hoc List 
            apnd(('a', 'b', 'c')) # Python tuple in hoc List 
            apnd({'a':1, 'b':2, 'c':3}) # Python dictionary in hoc List 
            for item in my_list:
                print(item)
             
            h('for i=0, List[0].count-1 print List[0].object(i)') 

         
        To see all the methods available for a hoc object, use, for example, 

        .. code-block::
            python

            dir(h.Vector) 

         
        h.anyclass can be subclassed with 

        .. code-block::
            python

            class MyVector(neuron.hclass(neuron.h.Vector)) : 
              pass 
            
            v = MyVector(10) 
            v.zzz = 'hello' # a new attribute 
            print(v.size()) # call any base method 

        If you override a base method such as 'size' use 

        .. code-block::
            python

            v.baseattr('size')() 

        to access the base method. Multiple inheritance involving hoc classes 
        probably does not make sense. 
        If you override the __init__ procedure when subclassing a Section, 
        be sure to explicitly 
        initialize the Section part of the instance with 

        .. code-block::
            python

            nrn.Section.__init__() 

         
        Since nrn.Section is a standard Python class one can 
        subclass it normally with 

        .. code-block::
            python

            class MySection(neuron.nrn.Section): 
              pass 

         
        The hoc setpointer statement is effected in Python as a function call 
        with a syntax for POINT_PROCESS and SUFFIX (density)mechanisms respectively 
        of 

        .. code-block::
            python

            h.setpointer(_ref_hocvar, 'POINTER_name', point_proces_object) 
            h.setpointer(_ref_hocvar, 'POINTER_name', nrn.Mechanism_object) 

        See :file:`nrn/share/examples/nrniv/nmodl/`\ (:file:`tstpnt1.py` and :file:`tstpnt2.py`) for 
        examples of usage. For a density mechanism, the 'POINTER_name' cannot 
        have the SUFFIX appended. For example if a mechanism with suffix foo has 
        a POINTER bar and you want it to point to t use 

        .. code-block::
            python

            h.setpointer(_ref_t, 'bar', sec(x).foo) 

         

    .. seealso::
        :meth:`Vector.to_python`, :meth:`Vector.from_python`

         

----



.. method:: neuron.hoc.hoc_ac


    Syntax:
        ``import hoc``

        ``double_value = hoc.hoc_ac()``

        ``hoc.hoc_ac(double_value)``


    Description:
        Get and set the hoc global scalar, :data:`hoc_ac_`-variables. 
        This is obsolete since HocObject 
        is far more general. 

        .. code-block::
            python

            import hoc 
            hoc.hoc_ac(25) 
            hoc.execute('print hoc_ac_') # prints 25 
            hoc.execute('hoc_ac_ = 17') 
            print(hoc.hoc_ac())  # prints 17 


         

----



.. method:: neuron.h.cas


    Syntax:
        ``sec = h.cas()``

    Description:
        Returns the :ref:`currently accessed section <CurrentlyAccessedSection>` as a Python 
        :class:`~neuron.h.Section` object. 

        .. code-block::
            python

            from neuron import h
            h(''' 
              create soma, dend[3], axon 
              access dend[1] 
            ''') 
             
            sec = h.cas() 
            print(sec)
        
        It is generally best to avoid writing code that manipulatesd the section stack. Use Python
        section objects, sec=, and section methods instead.


         

----



.. class:: neuron.h.Section


    Syntax:
        ``sec = h.Section()``

        ``sec = h.Section([name='string', [cell=self])``



    Description:
        The Python Section object allows modification and evaluation of the 
        information associated with a NEURON :ref:`geometry_section`. The typical way to get 
        a reference to a Section in Python is with :meth:`neuron.h.cas`  or 
        by using the hoc section name as in ``asec = h.dend[4]``. 
        The ``sec = Section()`` will create an anonymous Section with a hoc name 
        constructed from "Section" and the Python reference address. 
        Access to Section variables is through standard dot notation. 
        The "anonymous" python section can be given a name with the named 
        parameter and/or associated with a cell object using the named cell parameter. 
        Note that a cell association is required if one anticipates using the 
        :meth:`~ParallelContext.gid2cell` method of :class:`ParallelContext`. 

        .. code-block::
            python

            import neuron 
            h = neuron.h 
            sec = h.Section() 
            print(sec)         # prints <nrn.Section object at 0x2a96982108> 
            print(sec.name())  # prints PySec_2a96982108 
            sec.nseg = 3       # section has 3 segments (compartments) 
            sec.insert("hh")   # all compartments have the hh mechanism 
            sec.L = 20         # Length of the entire section is 20 um. 
            for seg in sec:    # iterates over the section compartments 
              for mech in seg: # iterates over the segment mechanisms 
                print('%s %g %s' % (sec, seg.x, mech.name()))

        A Python Section can be made the currently accessed 
        section by using its push method. Be sure to use :func:`pop_section` 
        when done with it to restore the previous currently accessed section. 
        I.e, given the above fragment, 

        .. code-block::
            python

            from neuron import h 
            h(''' 
            objref p 
            p = new PythonObject() 
            {p.sec.push() psection() pop_section()} 
            ''') 
            #or 
            print(sec)
            h.psection(sec=sec) 

        When calling a hoc function it is generally preferred to named sec arg style 
        to automatically push and pop the section stack during the scope of the 
        hoc function. ie 

        .. code-block::
            python

            h.psection(sec=sec) 
        
        The ``psection`` section method is different, in that it returns a Python dictionary rather
        than printing to the screen. It also provides more information, such as reaction-diffusion
        mechanisms that are present. One could, for example, do

        .. code-block::
            python

            from pprint import pprint
            pprint(sec.psection())
        
        The section ``psection`` method was added in NEURON 7.6.

         
        With a :class:`SectionRef` one can, for example, 

        .. code-block::
            python

            sr = h.SectionRef(sec=h.dend[2])
            sr.root.push(); print(h.secname()); h.pop_section() 

        or, more compactly and avoiding the modification of the section stack, 
        
        .. code-block::
            python

            sr = h.SectionRef(sec=h.dend[2]) 
            print('%s %s' % (sr.root.name(), h.secname(sec=sr.root)))

         
        Iteration over sections is accomplished with 

        .. code-block::
            python

            for s in h.allsec(): 
              print(s)
             
            sl = h.SectionList(); sl.wholetree() 
            for s in sl: 
              print(s)
        
        In lieu of using a SectionList, one can get the whole tree containing a given section
        as a Python list via:

        .. code-block::
            python

            tree_secs = my_sec.wholetree()
        
        (The wholetree section method was added in NEURON 7.7.)


         
        Connecting a child section to a parent section uses the connect method 
        using either 

        .. code-block::
            python

            childsec.connect(parentsec, parentx, childx) 
            childsec.connect(parentsegment, childx) 

        In the first form parentx and childx are optional with default values of 
        1 and 0 respectively. ``childx`` must be 0 or 1 (orientation of the child). Parentx is in the
        range [0 - 1] but will actually be connected to the center of the parent segment
        that contains parentx (or exactly at 0 or 1).
         
        sec.cell() returns the cell object that 'owns' the section. The return 
        value is None if no object owns the section (a top level section), the 
        instance of the hoc template that created the section, or the python 
        object specified by the named cell parameter 
        when the python section was created. 
         

----



Segment
=======

    Syntax:
        ``seg = section(x)``


    Description:
        A Segment object is obtained from a Section with the function notation where 
        the argument is 0 <= x <= 1 an the segment is the compartment that contains 
        the location x. The x value of the segment is seg.x and the section is 
        seg.sec . From a Segment one can obtain a Mechanism. 

        To iterate over segments, use ``for seg in sec: print ("%s(%g)" % (seg.sec.name, seg.x))``
        This does not include 0 area segments at 0 and 1. For those use ``for seg in sec.allseg():...``

----



Mechanism
=========


    Syntax:
        ``mech = segment.mechname``


    Description:
        A Mechanism object is obtained from a Segment. From a Mechanism one can 
        obtain a range variable. The range variable can also be obtained from the 
        segment by using the hoc range variable name that has the mechanism suffix. 

        To iterate over density mechanisms, use: ``for mech in seg: print (mech)``
        To get a python list of point processes in a segment: ``pplist = seg.point_processes()``

----

.. _Hoc_accessing_Python:

HOC accessing Python
~~~~~~~~~~~~~~~~~~~~


    Syntax:
        ``nrniv [file.py|file.hoc...]``


    Description:
        The absence of a -python argument causes NEURON to launch with Hoc 
        as the command line interpreter. Python files (or Hoc files) are run
        with the appropriate interpreter before presenting a Hoc user-interface.
        From the hoc world any python statement can be executed and anything 
        in the python world can be assigned or evaluated. 


----



.. function:: nrnpython


    Syntax:
        ``nrnpython("any python statement")``


    Description:
        Executes any python statement. Returns 1 on success; 0 if an exception
        was raised or if python support is not available.
        
        In particular, ``python_available = nrnpython("")`` is 1 (true) if
        python support is available and 0 (false) if python support is not
        available.
    
    Example:

        .. code-block::
            python

            nrnpython("import sys") 
            nrnpython("print(sys.path)") 
            nrnpython("a = [1,2,3]") 
            nrnpython("print(a)") 
            nrnpython("from neuron import h") 
            nrnpython("h('print PI')") 
            

         

----



.. class:: PythonObject


    Syntax:
        ``p = new PythonObject()``


    Description:
        Accesses any python object. Almost equivalent to :class:`~neuron.hoc.HocObject` in the 
        python world but because of some hoc syntax limitations, ie. hoc does not 
        allow an object to be a callable function, and top level indices have 
        different semantics, we sometimes need to use a special idiom, ie. the '_' 
        method. Strings and double numbers move back and forth between Python and 
        Hoc (but Python integers, etc. become double values in Hoc, and when they 
        get back to the Python world, they are doubles). 
         

        .. code-block::
            python

            objref p 
            p = new PythonObject() 
            nrnpython("ev = lambda arg : eval(arg)") // interprets the string arg as an 
                                      //expression and returns the value 
            objref tup 
            print p.ev("3 + 4")       // prints 7 
            print p.ev("'hello' + 'world'") // prints helloworld 
            tup = p.ev("('xyz',2,3)") // tup is a PythonObject wrapping a Python tuple 
            print tup                 // prints PythonObject[1] 
            print tup._[2]            // the 2th tuple element is 3 
            print tup._[0]            // the 0th tuple element is xyz 
             
            nrnpython("from neuron import h")   // back in the Python world 
            nrnpython("print h.tup")  // prints ('xyz', 2, 3) 

        Note that one needs the '_' method, equivalent to 'this', because trying to 
        get at an element through the built-in python method name via 

        .. code-block::
            python

            tup.__getitem__(0) 

        gives the error "TypeError: tuple indices must be integers" since 
        the Hoc 0 argument is a double 0.0 when it gets into Python. 
        It is difficult to pass an integer to a Python function from the hoc world. 
        The only time Hoc doubles appear as integers in Python, is when they are 
        the value of an index. If the index is not an integer, e.g. a string, use 
        the __getitem__ idiom. 

        .. code-block::
            python

            objref p 
            p = new PythonObject() 
            nrnpython("ev = lambda arg : eval(arg)") 
            objref d 
            d = p.ev("{'one':1, 'two':2, 'three':3}") 
            print d.__getitem__("two")        // prints 2 
             
            objref dg 
            dg = d.__getitem__ 
            print dg._("two")                // prints 2 

         
        To assign a value to a python variable that exists in a module use 

        .. code-block::
            python

            nrnpython("a = 10") 
            p = new PythonObject() 
            p.a = 25 
            p.a = "hello" 
            p.a = new Vector(4) 
            nrnpython("b = []") 
            p.a = p.b 


         

----

.. method:: neuron.hoc.execute


    Syntax:
        ``import neuron``

        ``neuron.hoc.execute('any hoc statement')``


    Description:
        Execute any statement or expression using the Hoc interpreter. This is 
        obsolete since the same thing can be accomplished with HocObject with 
        less typing. 
        Note that triple quotes can be used for multiple line statements. 
        A '\n' should be escaped as '\\n'. 

        .. code-block::
            python

            hoc.execute('load_file("nrngui.hoc")') 


    .. seealso::
        :func:`nrnpython`

    