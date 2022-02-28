CoreNEURON compatibility
########################
CoreNEURON is designed as a library within the NEURON simulator and can transparently handle all spiking network simulations, including gap junction coupling, with the **fixed time step method**.
In order to run a NEURON model with CoreNEURON certain conditions must be met:

* MOD files must be ``THREADSAFE``, for more information see :ref:`THREADSAFE`
* Random123 must be used if a random number generator is needed; MCellRan4 is not supported
* ``POINTER`` variables must be converted to ``BBCOREPOINTER``, for more information see :ref:`BBCOREPOINTER`
* ``TABLE`` statements should be commented out in MOD files if they are
  to be executed with CoreNEURON on GPU.
  See `nrn#1505 <https://github.com/neuronsimulator/nrn/issues/1505>`_
  for more information.

Note that models using the following features cannot presently be simulated with CoreNEURON.

.. list-table:: Non-supported NEURON features
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

THREADSAFE
**********
CoreNEURON's acceleration is made possible through it's ability not only to run the simulations in parallel but also by being able to take advantage of the vectorization support of CPUs and execution on GPUs. To be able to execute a model in parallel using multiple threads the mod file developer should make sure that the mod file is thread safe. You can find more information about ``THREADSAFE`` mod files `here <https://neuron.yale.edu/neuron/docs/multithread-parallelization>`_.

By making the mod file thread safe the initialization and current and state update for all the sections that include this mechanism can be done in parallel using vectorized CPU instructions and can be executed in GPU safely.

Some useful instructions to make a mod file thread safe are:

* Make sure that all the variables that are written in the mod file and are defined as ``GLOBAL`` are now defined as ``RANGE``. More information about ``GLOBAL`` and ``RANGE`` variables can be found `here <https://nrn.readthedocs.io/en/latest/hoc/modelspec/programmatic/mechanisms/nmodl2.html>`_.
* In case there are any ``VERBATIM`` blocks those need to be threadsafe and there must be a ``THREADSAFE`` keyword in the ``NEURON`` block to declare that the ``VERBATIM`` blocks are thread safe. Also make sure that the ``NEURON`` block is defined before any ``VERBATIM`` block in the mod file


BBCOREPOINTER
*************
``BBCOREPOINTER`` is used to transfer dynamically allocated data between NEURON and CoreNEURON.

User-allocated data can be managed in NMODL using the ``POINTER`` type.
It allows the programmer to reference data that has been allocated in HOC or in ``VERBATIM`` blocks.
This allows for more advanced data-structures that are not natively supported in NMODL.

Since NEURON itself has no knowledge of the layout and size of this data it cannot
transfer ``POINTER`` data automatically to CoreNEURON.
Furtheremore, in many cases there is no need to transfer the data between the two instances.
In some cases, however, the programmer would like to transfer certain user-defined data into CoreNEURON.
The most prominent example are Random123 random number stream parameters used in synapse mechanisms.
To support this use-case the ``BBCOREPOINTER`` type was introduced.
Variables that are declared as ``BBCOREPOINTER`` behave exactly the same as ``POINTER`` but are
additionally taken into account when NEURON is serializing mechanism data (for file writing or
direct-memory transfer).
For NEURON to be able to write (and indeed CoreNEURON to be able to read) ``BBCOREPOINTER``
data, the programmer has to additionally provide two C functions that are called as part
of the serialization/deserialization.

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
     BBCOREPOINTER my_data
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
