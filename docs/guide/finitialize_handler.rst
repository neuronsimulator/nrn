.. _finitialize_handler:

FIinitializeHandler
====================

Syntax 
------

.. code::
    c++

    fih = new FInitializeHandler("stmt", [obj])
    fih = new FInitializeHandler(type, "stmt", [obj])

Description 
-----------

Install an initialization handler statement to be called during a call to :func:`finitialize`. The default type is 1. The statement will be executed at the top level of the interpreter or else in the context of the optional obj arg.

Type 0 handlers are called before the mechanism INITIAL blocks.

Type 1 handlers are called after the mechanism INITIAL blocks. This is the best place to change state values.

Type 2 handlers are called just before return from finitialize. This is the best place to record values at t=0.

Type 3 handlers are called at the beginning of finitialize. At this point it is allowed to change the structure of the model.

See :func:`finitialize` for more details about the order of initialization processes within that function.

.. seealso::
    
    :class:`FInitializeHandler`

This class helps alleviate the administrative problems of maintaining variations of the proc :ref:`Init <runcontrol_initrun>`.

Examples 
--------

:download:`execute following example <data/finithnd1.hoc.txt>`

.. code::
    c++

    // specify an example model
    load_file("nrngui.hoc")
    create a, b
    access a
    forall insert hh

    objref fih[3]
    fih[0] = new FInitializeHandler(0, "fi0()")
    fih[1] = new FInitializeHandler(1, "fi1()")
    fih[2] = new FInitializeHandler(2, "fi2()")

    proc fi0() {
            print "fi0() called after v set but before INITIAL blocks"
            printf("  a.v=%g a.m_hh=%g\n", a.v, a.m_hh)
            a.v = 10
    }

    proc fi1() {
        print "fi1() called after INITIAL blocks but before BREAKPOINT blocks"
        print "     or variable step initialization."
        print "     Good place to change any states."
        printf("  a.v=%g a.m_hh=%g\n", a.v, a.m_hh)
        printf("  b.v=%g b.m_hh=%g\n", b.v, b.m_hh)
        b.v = 10
    }

    proc fi2() {
            print "fi2() called after everything initialized. Just before return"
            print "     from finitialize."
            print "     Good place to record or plot initial values"
            printf("  a.v=%g a.m_hh=%g\n", a.v, a.m_hh)
            printf("  b.v=%g b.m_hh=%g\n", b.v, b.m_hh)
    }

    begintemplate Test
    objref fih, this
    proc init() {
            fih = new FInitializeHandler("p()", this)
    }
    proc p() {
            printf("inside %s.p()\n", this)
    }
    endtemplate Test

    objref test
    test = new Test()

    stdinit()
    fih[0].allprint()

Syntax 
------

.. code::
    c++

    fih.allprint()

Description
-----------

Prints all the FInitializeHandler statements along with their object context in the order they will be executed during an :func:`finitialize` call.

