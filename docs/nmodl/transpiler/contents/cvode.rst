Variable timestep integration (CVODE)
=====================================

As opposed to fixed timestep integration, variable timestep integration (CVODE
in NEURON parlance) uses the SUNDIALS package to solve a ``DERIVATIVE`` or
``KINETIC`` block using a variable timestep. This allows for faster computation
times if the function in question does not vary too wildly.

Implementation in NMODL
-----------------------

The code generation for CVODE is activated only if exactly one of the following
is satisfied:

1. there is one ``KINETIC`` block in the mod file
2. there is one ``DERIVATIVE`` block in the mod file
3. a ``PROCEDURE`` block is solved with the ``after_cvode``, ``cvode_t``, or
   ``cvode_t_v`` methods

In NMODL, all ``KINETIC`` blocks are internally first converted to
``DERIVATIVE`` blocks. The ``DERIVATIVE`` block is then converted to a
``CVODE`` block, which contains two parts; the first part contains the update
step for non-stiff systems (functional iteration), while the second part
contains the update step for stiff systems (additional step using the
Jacobian).  For more information, see `CVODES documentation`_, eqs. (4.8) and
(4.9). Given a ``DERIVATIVE`` block of the form:

.. _CVODES documentation: https://sundials.readthedocs.io/en/latest/cvodes/Mathematics_link.html

.. code-block::

   DERIVATIVE state {
       x_i' = f(x_1, ..., x_n)
   }

the structure of the ``CVODE`` block is then roughly:

.. code-block::

   CVODE state[n] {
       Dx_i = f_i(x_1, ..., x_n)
   }{
       Dx_i = Dx_i / (1 - dt * J_ii(f))
   }

where ``N`` is the total number of ODEs to solve, and ``J_ii(f)`` is the
diagonal part of the Jacobian, i.e.

.. math::

   J_{ii}(f) = \frac{ \partial f_i(x_1, \ldots, x_n) }{\partial x_i}

As an example, consider the following ``DERIVATIVE`` block:

.. code-block::

    DERIVATIVE state {
        X' = - X
    }

Where ``X`` is a ``STATE`` variable with some initial value, specified in the
``INITIAL`` block. The corresponding ``CVODE`` block is then:

.. code-block::

   CVODE state[1] {
       DX = - X
   }{
       DX = DX / (1 - dt * (-1))
   }


**NOTE**: in case there are ``CONSERVE`` statements in ``KINETIC`` blocks, as
they are merely hints to NMODL, and have no impact on the results, they are
removed from ``CVODE`` blocks before the codegen stage.
