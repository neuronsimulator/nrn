.. _procedure-too-big-error:

I see an error message that says ... procedure too big in ./foo.hoc ...
---------------------------------------------------------------------

There is an upper limit on the size of a procedure that the hoc parser can handle. The workaround is simple. 

Instead of having a single giant procedure, break it into several smaller procedures, and then call these procedures one after another. For example, suppose your procedure is 

.. code::
    c++

    proc buildcell() {
   ... lots of hoc statements ...
   }

just chop it into smaller chunks like this

.. code::
    c++

    proc buildcell_1() {
   ... some hoc statements ...
   }
   proc buildcell_2() {
   ... some more hoc statements ...
   }
   ... etc ...

then execute them with

.. code::
    c++

    buildcell_1()
   buildcell_2()
   ...

How big can a procedure be? I've never tried to find out. Try cutting your big procedure in half and see if that works. If it doesn't, cut the pieces in half and try again. Eventually you'll find a size that works.

