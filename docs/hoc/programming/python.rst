
.. _hoc_python:

         
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
    The python version used is that found from ``which python``. 
     
    NEURON can be used as an extension to Python if, after building as above, 
    one goes to the src/nrnpython directory containing the Makefile and types 
    something analogous to 

    .. code-block::
        none

        python setup.py install --home=$HOME 

    Which on my machine installs in :file:`/home/hines/lib64/python/neuron`
    and can be imported into NEURON with 

    .. code-block::
        python

        ipython 
        import sys 
        sys.path.append("/home/hines/lib64/python") 
        import neuron 

    It is probably better to avoid the incessant ``import sys``... and instead 
    add to your shell environment something analogous to 

    .. code-block::
        none

        export PYTHONPATH=$PYTHONPATH:/home/hines/lib64/python 

    since when launching NEURON and embedding Python, the path is automatically 
    defined so that ``import neuron`` does not require any prerequisites. 
    If there is a ``@<host-cpu@>/.libs/libnrnmech.so`` file in your working 
    directory, those nmodl mechanisms will be loaded as well. 
    After this, you will probably want to: 

    .. code-block::
        python

        h = neuron.h # neuron imports hoc and does a  h = hoc.HocObject() 

    In the past we also recommended an "import nrn" but this is no longer 
    necessary as everything in that module is also directly available from 
    the "h" object. 
    You can use the hoc function :hoc:func:`nrn_load_dll` to load mechanism files
    as well, e.g. if neurondemo was used earlier so the shared object exists, 

    .. code-block::
        python

        h = hoc.HocObject() 
        h('nrn_load_dll("$(NEURONHOME)/demo/release/x86_64/.libs/libnrnmech.so")') 



.. _hoc_python_accessing_hoc:

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



.. hoc:method:: neuron.hoc.execute


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
        :hoc:func:`nrnpython`

         

----



.. hoc:class:: neuron.hoc.HocObject


    Syntax:
        ``import neuron``

        ``h = neuron.hoc.HocObject()``


    Description:
        Allow access to anything in the Hoc interpreter. 
        Note that ``h = neuron.h`` is the typical statement used since the 
        neuron module creates an h field. 
        When created via ``hoc.HocObject()`` its print string is "TopLevelHocInterpreter". 

        .. code-block::
            python

            h("any hoc statement") 

        is the same as hoc.execute(...) 
         
        Any hoc variable or string in the Hoc world can be accessed 
        in the Python world: 

        .. code-block::
            python

            h('strdef s') 
            h('{x = 3  s = "hello"}') 
            print h.x          # prints 3.0 
            print h.s          # prints hello 

        And if it is assigned a value in the python world it will be that value 
        in the Hoc world. (Note that any numeric python type becomes a double 
        in Hoc.) 

        .. code-block::
            python

            h.x = 25 
            h.s = 'goodbye' 
            h('print x, s')    #prints 25 goodbye 

         
        Any hoc object can be handled in Python. 

        .. code-block::
            python

            h('objref vec') 
            h('vec = new Vector(5)') 
            print h.vec        # prints Vector[0] 
            print h.vec.size() # prints 5.0 

        Note that any hoc object method or field may be called, or evaluated/assigned 
        using the normal dot notation which is consistent between hoc and python. 
        However, hoc object methods MUST have the parentheses or else the Python 
        object is not the return value of the method but a method object. ie. 

        .. code-block::
            python

            x = h.vec.size     # not 5 but a python callable object 
            print x            # prints: Vector[0].size() 
            print x()          # prints 5.0 

        This is also true for indices 

        .. code-block::
            python

            h.vec.indgen().add(10) # fills elements with 10, 11, ..., 14 
            print h.vec.x[2]   # prints 12.0 
            x = h.vec.x        # a python indexable object 
            print x            # prints Vector[0].x[?] 
            print x[2]         # prints 12.0 

        The hoc object can be created directly in Python. E.g. 

        .. code-block::
            python

            v = h.Vector(10).indgen.add(10) 

         
        Iteration over hoc Vector, List, and arrays is supported. e.g. 

        .. code-block::
            python

            v = h.Vector(4).indgen().add(10) 
            for x in v : 
              print x 
             
            l = h.List() ; l.append(v); l.append(v); l.append(v) 
            for x in l : 
              print x 
             
            h('objref o[2][3]') 
            for x in h.o : 
              for y in x : 
                print x, y 
             

         
        Any hoc Section can be handled in Python. E.g. 

        .. code-block::
            python

            h('create soma, axon') 
            ax = h.axon 

        makes ax a Python :hoc:class:`~neuron.h.Section` which references the hoc
        axon section. Many hoc functions require a currently accessed section 
        and for these a typical idiom is 

        .. code-block::
            python

            ax.push() ; print secname() ; h.pop_section() 

        More compact is to use the "sec" keyword parameter after the last positional 
        parameter which makes the Section value the currently accessed section during 
        the scope of the function call. e.g 

        .. code-block::
            python

            print secname(sec=ax) 

         
        Point processes are handled by direct object creation as in 

        .. code-block::
            python

            stim = IClamp(1.0, sec = ax) 
            // or 
            stim = IClamp(ax(1.0)) 

        The latter is a somewhat simpler idiom that uses the Segment object which knows both the 
        section and the location in the section and can also be used with the 
        stim.loc function. 
         
        Many hoc functions use call by reference and return information by 
        changing the value of an argument. These are called from the python 
        world by passing a HocObject.ref() object. Here is an example that 
        changes a string. 

        .. code-block::
            python

            h('proc chgstr() { $s1 = "goodbye" }') 
            s = h.ref('hello') 
            print s[0]          # notice the index to dereference. prints hello 
            h.chgstr(s) 
            print s[0]          # prints goodbye 
            h.sprint(s, 'value is %d', 2+2) 
            print s[0]          # prints value is 4 

        and here is an example that changes a pointer to a double 

        .. code-block::
            python

            h('proc chgval() { $&1 = $2 }') 
            x = h.ref(5) 
            print x[0]          # prints 5.0 
            h.chgval(x, 1+1) 
            print x[0]          # prints 2.0 

        Finally, here is an example that changes a objref arg. 

        .. code-block::
            python

            h('proc chgobj() { $o1 = new List() }') 
            v = h.ref([1,2,3])  # references a Python object 
            print v[0]          # prints [1, 2, 3] 
            h.chgobj(v) 
            print v[0]          # prints List[0] 

        Unfortunately, the HocObject.ref() is not often useful since it is not really 
        a pointer to a variable. For example consider 

        .. code-block::
            python

            h('x = 1') 
            y = h.ref(h.x) 
            print y         # prints hoc ref value 1 
            print h.x, y[0] # prints 1.0 1.0 
            h.x = 2 
            print h.x, y[0] # prints 2.0 1.0 

        and thus in not what is needed in the most common 
        case of a hoc function holding a pointer to a variable such as 
        :hoc:meth:`Vector.record` or :hoc:meth:`Vector.play`. For this one needs the :samp:`_ref_{varname}` idiom
        which works for any hoc variable and acts exactly like a c pointer. eg: 

        .. code-block::
            python

            h('x = 1') 
            y = h._ref_x 
            print y          # prints pointer to hoc value 1 
            print h.x, y[0]  # prints 1.0 1.0 
            h.x = 2 
            print h.x, y[0]  # prints 2.0 2.0 
            y[0] = 3 
            print h.x, y[0]  # prints 3.0 3.0 

        Of course, this works only for hoc variables, not python variables.  For 
        arrays, use all the index arguments and prefix the name with _ref_.  The 
        pointer will be to the location indexed and one may access any element 
        beyond the location by giving one more non-negative index.  No checking 
        is done with regard to array bounds errors.  e.g 

        .. code-block::
            python

            v = h.Vector(4).indgen().add(10) 
            y = v._ref_x[1]    # holds pointer to second element of v 
            print v.x[2], y[1] # prints 12.0 12.0 
            y[1] = 50 
            v.printf()         # prints 10 11 50 13 

        The idiom is used to record from (or play into) voltage and mechanism variables. eg 

        .. code-block::
            python

            v = h.Vector() 
            v.record(h.soma(.5)._ref_v, sec = h.soma) 
            pi = h.Vector() 
            pi.record(h.soma(.5).pas._ref_i, sec = h.soma) 
            ip = h.Vector() 
            ip.record(h.soma(.5)._ref_i_pas, sec = h.soma) 

         
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
            list = h.newlist() 
            apnd = list.append 
            apnd([1,2,3])      # Python list in hoc List 
            apnd(('a', 'b', 'c')) # Python tuple in hoc List 
            apnd({'a':1, 'b':2, 'c':3}) # Python dictionary in hoc List 
            item = list.object 
            for i in range(0, int(list.count())) : # notice the irksome cast to int. 
              print item(i) 
             
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
            print v.size() # call any base method 

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
        :hoc:meth:`Vector.to_python`, :hoc:meth:`Vector.from_python`

         

----



.. hoc:method:: neuron.hoc.hoc_ac


    Syntax:
        ``import hoc``

        ``double_value = hoc.hoc_ac()``

        ``hoc.hoc_ac(double_value)``


    Description:
        Get and set the hoc global scalar, :hoc:data:`hoc_ac_`-variables.
        This is obsolete since HocObject 
        is far more general. 

        .. code-block::
            python

            import hoc 
            hoc.hoc_ac(25) 
            hoc.execute('print hoc_ac_') # prints 25 
            hoc.execute('hoc_ac_ = 17') 
            print hoc.hoc_ac()  # prints 17 


         

----



.. hoc:method:: neuron.h.cas


    Syntax:
        ``sec = h.cas()``

        ``or``

        ``import nrn``

        ``sec = nrn.cas()``


    Description:
        Returns the :ref:`currently accessed section <hoc_CurrentlyAccessedSection>` as a Python
        :hoc:class:`~neuron.h.Section` object.

        .. code-block::
            python

            import neuron 
            neuron.h(''' 
              create soma, dend[3], axon 
              access dend[1] 
            ''') 
             
            sec = h.cas() 
            print sec, sec.name() 


         

----



.. hoc:class:: neuron.h.Section


    Syntax:
        ``sec = h.Section()``

        ``sec = h.Section([name='string', [cell=self])``

        ``or``

        ``import nrn``

        ``sec = nrn.Section()``


    Description:
        The Python Section object allows modification and evaluation of the 
        information associated with a NEURON :ref:`hoc_geometry_section`. The typical way to get
        a reference to a Section in Python is with :hoc:meth:`neuron.h.cas`  or
        by using the hoc section name as in ``asec = h.dend[4]``. 
        The ``sec = Section()`` will create an anonymous Section with a hoc name 
        constructed from "Section" and the Python reference address. 
        Access to Section variables is through standard dot notation. 
        The "anonymous" python section can be given a name with the named 
        parameter and/or associated with a cell object using the named cell parameter. 
        Note that a cell association is required if one anticipates using the 
        :hoc:meth:`~ParallelContext.gid2cell` method of :hoc:class:`ParallelContext`.

        .. code-block::
            python

            import neuron 
            h = neuron.h 
            sec = h.Section() 
            print sec        # prints <nrn.Section object at 0x2a96982108> 
            print sec.name() # prints PySec_2a96982108 
            sec.nseg = 3     # section has 3 segments (compartments) 
            sec.insert("hh") # all compartments have the hh mechanism 
            sec.L = 20       # Length of the entire section is 20 um. 
            for seg in sec :   # iterates over the section compartments 
              for mech in seg : # iterates over the segment mechanisms 
                print sec.name(), seg.x, mech.name() 

        A Python Section can be made the currently accessed 
        section by using its push method. Be sure to use :hoc:func:`pop_section`
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
            sec.push() 
            h.secname() 
            h.psection() 
            h.pop_section() 

        When calling a hoc function it is generally preferred to named sec arg style 
        to automatically push and pop the section stack during the scope of the 
        hoc function. ie 

        .. code-block::
            python

            h.psection(sec=sec) 

         
        With a :hoc:class:`SectionRef` one can, for example,

        .. code-block::
            python

            h.dend[2].push() ; sr = h.SectionRef() ; h.pop_section() 
            sr.root.push() ; print h.secname() ; h.pop_section() 

        or, more compactly, 
        
        .. code-block::
            python

            sr = h.SectionRef(sec=h.dend[2]) 
            print sr.root.name(), h.secname(sec=sr.root) 

         
        Iteration over sections is accomplished with 

        .. code-block::
            python

            for s in h.allsec() : 
              print h.secname() 
             
            sl = h.SectionList() ; sl.wholetree() 
            for s in sl : 
              print h.secname() 


         
        Connecting a child section to a parent section uses the connect method 
        using either 

        .. code-block::
            python

            childsec.connect(parentsec, parentx, childx) 
            childsec.connect(parentsegment, childx) 

        In the first form parentx and childx are optional with default values of 
        1 and 0 respectively. Parentx must be 0 or 1. In the second form, childx 
        is optional and by default is 0. The parentsegment must be either 
        parentsec(0) or parentsec(1). 
         
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

         

----



Mechanism
=========


    Syntax:
        ``mech = segment.mechname``


    Description:
        A Mechanism object is obtained from a Segment. From a Mechanism one can 
        obtain a range variable. The range variable can also be obtained from the 
        segment by using the hoc range variable name that has the mechanism suffix. 

         

----


.. _hoc_Hoc_accessing_Python:

HOC accessing Python
~~~~~~~~~~~~~~~~~~~~


    Syntax:
        ``nrniv [file.hoc...]``


    Description:
        The absence of a -python argument causes NEURON to launch with Hoc 
        as the command line interpreter. At present, no :file:`file.py` arguments 
        are allowed as all named files are treated as hoc files. Nevertheless, 
        from the hoc world any python statement can be executed and anything 
        in the python world can be assigned or evaluated. 


----



.. hoc:function:: nrnpython


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
            nrnpython("print sys.path") 
            nrnpython("a = [1,2,3]") 
            nrnpython("print a") 
            nrnpython("import hoc") 
            nrnpython("hoc.execute('print PI')") 
            

         

----



.. hoc:class:: PythonObject


    Syntax:
        ``p = new PythonObject()``


    Description:
        Accesses any python object. Almost equivalent to :hoc:class:`~neuron.hoc.HocObject` in the
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
             
            nrnpython("import hoc")   // back in the Python world 
            nrnpython("h = hoc.HocObject()") // tup is a Python Tuple object 
            nrnpython("print h.tup")   // prints ('xyz', 2, 3) 

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


         

