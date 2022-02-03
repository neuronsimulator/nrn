Non-supported NEURON features
=============================

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
