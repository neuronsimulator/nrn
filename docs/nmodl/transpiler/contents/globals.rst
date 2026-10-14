Global Variables
----------------

The following global variables exist:

  * ``GLOBAL`` visible from HOC/Python.
  * ``LOCAL`` at file scope, called top-locals.
  * ``CONSTANT`` not visible from HOC/Python.

GLOBAL variables
================


GLOBAL variables behave in one of three different ways:

  * **read-only** if the variable is not written to from inside the MOD file,
    is considered read-only. It behaves like there's a single instance that's
    shared across all threads. Currently, they're implemented as a static
    ``double``, i.e. a regular C/C++ global variable. This MOD file is (still)
    thread-safe.

  * **read-write** Setting a value via ``PARAMETER`` is not considered a
    write-access. While setting the value from within the MOD file in all other
    contexts, including ``INITIAL``, counts as a write-access.

    * **THREAD_SAFE:**, if the MOD file is marked thread-safe, then the
      assumption is that it's safe to create multiple instances of the global
      variable, e.g. one per thread. We call these *thread-variables*. This MOD
      file is considered thread-safe.

    * **Not THREAD_SAFE:** if the MOD file is not stated to be ``THREADSAFE``,
      then the assumption is that the global variables must behave like a
      single instance would. As a result, these MOD files are not thread-safe.

The visibility of GLOBAL variables it the following:

  * **ASSIGNED:** any GLOBAL variable that appears in an ASSIGNED block
    is not visible from HOC/Python.

  * **PARAMETER:** any GLOBAL variable that appears in a PARAMETER block
    is visible (read/write) from HOC/Python. Any PARAMETER that's not
    explicitly made a RANGE variable is considered a GLOBAL.

  * **undefined:** any GLOBAL variable that's not listed in either an ASSIGNED
    or PARAMETER block is treated as if it were ASSIGNED, i.e. it's not visible
    from HOC/Python.

Top-LOCAL variables
===================

Top-LOCAL variables are LOCAL variables at file-scope. They are never visible
from HOC/Python and always treated as top-local variables.

Since top-locals can't be assigned a value from HOC/Python and assignment in
INITIAL blocks counts as a write-access, the only way of assigning a value to a
read-only top-local would be in the PARAMETER block. However, variables
mentioned in PARAMETER are always RANGE or GLOBAL variables; and it's not
allowed to have two global variables with the same name. Therefore, read-only
top-locals aren't possible.

Note that ``nocmodl`` promotes all top-local variables to thread-variables, even
if the MOD file isn't marked with ``THREADSAFE``. Hence, top-local variables are
always thread-variables.

Thread Variables
================
Thread variables can be safely used as scratch-pad memory. Historically,
they've been advertised as an optimization technique that reduces the memory
footprint.

The canonical example is ``hh.mod``. The common pattern is that they're used as
return values from a PROCEDURE:

.. code-block::

  DERIVATIVE states {
    rates(v)
    m' =  (minf-m)/mtau
  }

  PROCEDURE rates(v(mV)) {
    TABLE minf, mtau DEPEND celsius FROM -100 TO 100 WITH 200

    minf = ...
    mtau = ...
  }

What we see is that for every instance we compute the value ``minf`` and
``mtau``, before we use them in ``states``. Technically there's no need for one
copy per instance of the mechanism. For example in Python one could write:

.. code-block::

   minf, mtau = rates(v[i])
   dm[i] =  (minf-m[i])/mtau

Therefore, if the author doesn't need to record the value of ``minf`` and
``mtau``, then using RANGE variables might be considered wasting memory. Under
these circumstances, and before multi-threading existed, the solution was to use
a GLOBAL. When multi-core processors arrived, these MOD files were suddenly not
thread-safe. The solution was to introduce a keyword ``THREADSAFE`` and create
one copy of the global per thread.


Initial Values
~~~~~~~~~~~~~~
Note that thread-variables ignore the initial value set in the PARAMETER block
entirely.

For INITIAL blocks the requirement is that:

.. code-block::

   INITIAL {
     gbl = 2.0
   }

guarantees that all copies of ``gbl`` are assigned the value ``2.0``.

HOC/Python Access
~~~~~~~~~~~~~~~~~

Note that there's no synchronization when setting or writing to
thread-variables. What happens is that is acts the (or a) value of thread 0.
The value on other threads is either left unchanged when writing or ignored
when reading the global variable.


Implementation Details for NEURON
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

NMODL distinguishes between top-local variables and GLOBAL variables at the
level of the AST. Then for code-generation we introduce the concept of
"thread-variables". All top-locals are considered thread-variables. GLOBAL
variables that are **read-write** and THREADSAFE can be converted to thread
variables.

Since thread-variables are permitted to have multiple copies per thread, we can
generalize this to be multiple copies per thread and SIMD lane; or one copy per
instance of the mechanism for GPUs (effectively a RANGE variable similar to
what CoreNEURON does).

Registering GLOBAL variables for access from HOC/Python happens via

.. code-block:: 

   static DoubScal hoc_scdoub[] = {
     {"g_w_shared_global", &g_w_shared_global},
     {0, 0}
   };

   static DoubVec hoc_vdoub[] = {
     {"g_arr_shared_global", g_arr_shared_global, 3},
     {0, 0, 0}
   };

   hoc_register_var(hoc_scdoub, hoc_vdoub, hoc_intfunc);


which means for each global we register a stable address (e.g. the address of
some static variable) individually. The elements of ARRAY valued globals must
be stored contiguously.

The strategy is the following: each instance of the mechanism is associated
with a specific, not necessarily unique, copy of the thread-variable. For SIMD
this allows us to compute the copy of the thread-variable using modulo
arithmetic; on a GPU one could either assign a copy to each variable; or use
scratch pad memory (e.g. ``__shared__`` memory when using CUDA).

Quirks
~~~~~~

Collection of slightly surprising behaviour:

  * Thread variables effectively can't be use in NET_RECEIVE blocks, because
    the code ``nocmodl`` produces will cause a SEGFAULT.


PARAMETER variables
===================

These can be either RANGE or not RANGE. They can be both read and write. If
they're written to, they're converted to thread-variables. Therefore, the rest
of this section will only describe read-only PARAMETERs.

Additionally, parameters optionally have: a) a default value, b) units and c) a
valid range.

Default Values
~~~~~~~~~~~~~~
This section only applies to read-only PARAMETERs.

The behaviour differs for RANGE variables and non-RANGE variables. For RANGE
variables, default values need to be registered with NEURON for all PARAMETERs
that are RANGE variables. The function is called ``hoc_register_parm_default``.

Note, that NOCMODL uses it in the ``nrn_alloc`` in the generated `.cpp` files
and also in ``ndatclas.cpp``. Therefore, it seems registering the values isn't
optional.

Non-RANGE variables are semantically equivalent to ``static double``. They're
simply assigned their value in the definition of the global variable.

CONSTANT variables
==================

These are comparatively simple. In NOCMODL they're implemented as non-const
static doubles. They're not accessible from HOC/Python (which makes them
simple).

Quirks
~~~~~~

In certain versions of NOCMODL around `9.0` and before (and NMODL) it's
possible to change the value of CONSTANT variables. The MOD file will still be
considered "thread-safe" (even if it might not be).


What Does CoreNEURON support?
=============================
CoreNEURON only supports read-only GLOBAL variables. Anything else needs to be
converted to a RANGE variable manually.
