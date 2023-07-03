CoreNEURON Compatibility
########################

CoreNEURON can transparently handle all spiking network simulations, including gap junction coupling, with the fixed time step method. In this document we summarise NEURON features that are not supported by CoreNEURON and changes required to make a model compatible with CoreNEURON.


Unsupported Features
********************

The below table summarises NEURON features that are presently not supported with CoreNEURON:

.. list-table::
   :widths: 45 10 10 35
   :header-rows: 1
   :class: fixed-table

   * - Feature
     - NEURON
     - CoreNEURON
     - Remarks *
   * - Variable time step methods (i.e CVODE and IDA)
     - ✔
     - ✖
     -
   * - Extracellular Mechanism
     - ✔
     - ✖ *
     - If `i_membrane` is of sole interest, then you can switch from extracellular to `cvode.use_fast_imem(1)` and make use of `i_membrane_`
   * - Linear Mechanism
     - ✔
     - ✖
     -
   * - RxD Module
     - ✔
     - ✖
     -
   * - Per-timestep interpreter calculations during simulation
     - ✔
     - ✖ *
     - Generally, `Vector.record` and `Vector.play` are ok, as are the display of GUI variable trajectories.
       Anything requiring callbacks into the interpreter (Python or HOC) are not implemented.

If you are using any of the above mentioned features in your model then CoreNEURON library can not be used for simulations. If you are interested in these features with CoreNEURON or performance improvement in general, feel free to provide feedback via a GitHub issue.


MOD File Compatibility
**********************

In order to use CoreNEURON, you need to build MOD files with CoreNEURON support i.e. use ``-coreneuron`` option as ``nrnivmodl -coreneuron <mod dir>``. If this command fails during compilation or translating MOD files but ``nrnivmodl <mod dir>`` is working fine then most likely there are MOD files incompatibility issues. In this section we describe how you can address such incompatibility.


Thread Safe MOD Files
~~~~~~~~~~~~~~~~~~~~~

One of the important difference in CoreNEURON execution with respect to NEURON is that multiple instances of a specific channel or synapse are executed in parallel. This parallelism could be via threads or SIMD instructions on modern CPUs/GPUs. Most of the MOD files are thread safe for parallel execution but there are certain constructs like ``GLOBAL`` variables or ``VERBATIM`` blocks that are not compatible by default. In this case user has to make sure MOD files are
thread safe. NEURON provides a script ``mkthreadsafe`` that can provide some help to make your MOD files are thread safe. You can find more information `here <https://neuron.yale.edu/neuron/docs/multithread-parallelization>`_. Here are some additional examples that will help you to make a MOD file thread safe:

* If you are using ``GLOBAL`` variables in a mod file, make sure those are not updated during execution (e.g. in ``INITIAL``, ``BREAKPOINT`` or ``DERIVATIVE`` block etc). If ``GLOBAL`` variable is writen then those variables are now needed to be defined as ``RANGE``. You can find information about ``GLOBAL`` and ``RANGE`` variables in :ref:`NMODL specification <nmodltoneuron>`.

    As an example, below MOD file defines ``minf`` variable as a ``GLOBAL`` and it is being updated in the ``DERIVATIVE`` block when ``PROCEDURE rates()`` is executed.
    
    .. code-block:: c++
    
      NEURON {
        SUFFIX test
        GLOBAL minf
      }
    
      ...
    
      DERIVATIVE states {
        rates(v)
        m' = (minf-m)/1.5
      }
    
      PROCEDURE rates(v (mV)) {
        minf = minf+1
      }

    As ``GLOBAL`` variable is shared across multiple instances of a mechanism, parallel execution will result into a race condition when ``minf`` is updated. In order to avoid this we need to simply convert it to a ``RANGE`` variable:
    
    .. code-block:: c++
    
      NEURON {
        SUFFIX test
        RANGE minf
      }
      ...
    
    Note that if you are accessing such variable via HOC or Python scripting interface then syntax for accessing ``GLOBAL`` is different than ``RANGE`` variable. So you need to update your code accordingly.

* If you are using ``VERBATIM`` blocks to overcome some limitations of NMODL language then such MOD file is by default treated as not thread safe. For example, in below case we are are using ``VERBATIM`` block to include some C header and return early from ``INITIAL`` block:

    .. code-block:: c++
    
      NEURON {
        SUFFIX test
        RANGE minf
      }
    
      VERBATIM
      #include <stdlib.h>
      ENDVERBATIM
    
      ASSIGNED {
        v            (mV)
        minf
      }
    
      STATE {
        m
      }
    
      INITIAL {
        rate(v)
        m = minf
        VERBATIM
        return 0;
        ENDVERBATIM
      }
    
      ...
    
    Technically, this mod file is thread safe as we don't have any race condition. But due to ``VERBATIM`` block this mod file is assumed non thread safe and hence we have to explicitly specify `THREADSAFE` keywork in the beginning of the ``NEURON`` block as:
    
    .. code-block:: c++
    
      NEURON {
        THREADSAFE
        SUFFIX test
        RANGE minf
      }
    
      ...
    
    Also, note that ``NEURON`` block needs to be before any ``VERBATIM`` block in the MOD file. So its safer to keep ``NEURON`` block at the top of MOD file.

* Certain ``SOLVE`` methods like ``euler`` are not thread safe since the best practical methods are ``cnexp`` for HH-like equations and ``derivimplicit`` for all the others. If you have such a MOD file:

    .. code-block:: c++

      SOLVE state METHOD euler

    then replace ``euler`` with ``cnexp``.


TABLE Usage With GPU Execution
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Currently ``TABLE`` constructs are not supported if you are building MOD files with GPU support. As ``TABLE`` constructs are used for efficiency reason (and not accuracy), you can safely comment out ``TABLE`` statement using ``:`` operator:

    .. code-block:: c++

      PROCEDURE rates(v(mV)) {

         : TABLE minf, mtau, hinf, htau, ninf, ntau DEPEND celsius FROM -100 TO 100 WITH 200


NEURON Only MOD Files
~~~~~~~~~~~~~~~~~~~~~

Certain MOD files are used for aspects like progress callbacks, reading inputs, etc. Often such MOD files are heavily depend on ``VERBATIM`` blocks and use internal data structure or functions provided by NEURON. Most likely such MOD files won't be compiled by CoreNEURON as they are using internal, NEURON specific APIs in ``VERBATIM`` blocks. If such mod file is used for only a usability aspect like progress bar then you can exclude that from compilation. Other option is to conditionally compile all ``VERBATIM`` blocks using macro ``NRNBBCORE``.

As an example, the code in below ``#ifndef NRNBBCORE`` block will be only compiled for NEURON.

.. code-block:: c++

  VERBATIM
  #ifndef NRNBBCORE
  <code block to be executed only by NEURON>
  #endif
  ENDVERBATIM

This way you can hide NEURON specific code from CoreNEURON compilation process.



Explicit ION Variables Update
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In some old MOD files ion currents are explicitly initialized in ``INITIAL`` blocks using ``VERBATIM`` construct as:

.. code-block:: c++

  VERBATIM
  cai = _ion_cai;
  Cai = _ion_Cai;
  ENDVERBATIM

Since such ion variables are implicitely updated by the code from NMODL transpiler, ``VERBATIM`` blocks like above are not required and must be deleted from the MOD files.


Random Number Generators: Random123 vs MCellRan4
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Pseudo-random numbers from a variety of distributions can be generated using NEURON's ``Random`` class. CoreNEURON only supports ``Random123`` generator.



Memory Management for POINTER Variables
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

User-allocated data managed in NMODL is a complex topic. Using ``POINTER`` variables, users can reference data that has been allocated in ``HOC`` or in ``VERBATIM`` blocks. Using this end users can built more advanced data-structures that are not natively supported in NMODL. Another commonly used example is point processes / synapses where ``POINTER`` variables used for holding random number generator object.

Since NEURON itself has no knowledge of the layout and size of this user allocated data,
it cannot transfer ``POINTER`` data automatically to CoreNEURON.
Furtheremore, in many cases there is no need to transfer the data between the two instances.
In some cases, however, the programmer would like to transfer certain user-defined data into CoreNEURON.
The most prominent example are Random123 random number stream parameters used in synapse mechanisms.

In order to inform NEURON that such ``POINTER`` variable needs to be transferred, the ``BBCOREPOINTER`` type was introduced.
Variables that are declared as ``BBCOREPOINTER`` behave exactly the same as ``POINTER`` but are
additionally taken into account when NEURON is transferring model to CoreNEURON for simulation.
For NEURON to be able to write (and indeed CoreNEURON to be able to read) ``BBCOREPOINTER``
data, the programmer has to additionally provide two C functions that are called as part
of the serialization/deserialization:

.. code-block:: c++

   static void bbcore_write(double* x, int* d, int* d_offset, int* x_offset, _threadargsproto_);
   static void bbcore_read(double* x, int* d, int* d_offset, int* x_offset, _threadargsproto_);

The implementation of ``bbcore_write`` and ``bbcore_read`` determines the serialization and
deserialization of the per-instance mechanism data referenced through the various
``BBCOREPOINTER``.

NEURON will call ``bbcore_write`` twice per mechanism instance.
In a first sweep, the call is used to determine the required memory to be allocated on the serialization arrays.
In the second sweep the call is used to fill in the data per mechanism instance.

.. list-table:: Arguments to ``bbcore_read`` and ``bbcore_write``.
   :widths: 15 85
   :header-rows: 1
   :class: fixed-table

   * - Argument
     - Description
   * - ``x``
     - A ``double`` type array that will be allocated by NEURON to fill
       with real-valued data. In the first call, ``x`` is ``nullptr``
       as it has not been allocated yet.
   * - ``d``
     - An ``int`` type array that will be allocated by NEURON to fill
       with integer-valued data. In the first call, ``d`` is
       ``nullptr`` as it has not been allocated yet.
   * - ``x_offset``
     - The offset in ``x`` at which the mechanism instance should write
       its real-valued ``BBCOREPOINTER`` data. In the first call this is
       an output argument that is expected to be updated by the
       per-instance size to be allocated.
   * - ``d_offset``
     - The offset in ``d`` at which the mechanism instance should write
       its integer-valued ``BBCOREPOINTER`` data. In the first call
       this is an output argument that is expected to be updated by the
       per-instance size to be allocated.
   * - ``_threadargsproto_``
     - A macro placeholder for NEURON/CoreNEURON data-structure
       parameters. They are typically only used through generated
       defines and not by the programmer. The macro is defined as
       follows:

       .. code-block:: c++

          #define _threadargsproto_ int _iml, int _cntml_padded, double *_p, Datum *_ppvar, \
                                    ThreadDatum *_thread, NrnThread *_nt, double _v


Putting all of this together, the following is a minimal MOD using ``BBCOREPOINTER``:

.. code-block:: hoc

   TITLE A BBCOREPOINTER Example

   NEURON {
     BBCOREPOINTER my_data : changed from POINTER
   }

   ASSIGNED {
     my_data
   }

   : Do something interesting with my_data ...
   VERBATIM
   static void bbcore_write(double* x, int* d, int* x_offset, int* d_offset, _threadargsproto_) {
     if (x) {
       double* x_i = x + *x_offset;
       x_i[0] = _p_my_data[0];
       x_i[1] = _p_my_data[1];
     }
     *x_offset += 2; // reserve 2 doubles on serialization buffer x
   }

   static void bbcore_read(double* x, int* d, int* x_offset, int* d_offset, _threadargsproto_) {
     assert(!_p_my_data);
     double* x_i = x + *x_offset;
     // my_data needs to be allocated somehow
     _p_my_data = (double*)malloc(sizeof(double)*2);
     _p_my_data[0] = x_i[0];
     _p_my_data[1] = x_i[1];
     *x_offset += 2;
   }
   ENDVERBATIM

If you have models with ``POINTER`` variables and user allocated memory then this requires due diligence. Below are some of the existing models adapted for CoreNEURON. These MOD files can act as a reference or you can simply reuse them if applicable:

* https://github.com/HumanBrainProject/olfactory-bulb-3d/tree/master/sim
* https://github.com/nrnhines/nrntraub/tree/master/mod
* https://github.com/suny-downstate-medical-center/M1_NEURON_paper/tree/main/mod
* https://github.com/neuronsimulator/testcorenrn/tree/master/mod
* https://github.com/neuronsimulator/reduced_dentate/tree/master/mechanisms

Have a question?
~~~~~~~~~~~~~~~

If you have any questions to make your model compatible with CoreNEURON, reach out to us via `GitHub issue <https://github.com/neuronsimulator/nrn/issues>`_.
