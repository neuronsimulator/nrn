NMODL "pointers"
================

Mechanisms can refer to values in other mechanisms, e.g. the sodium current
``ina``. Therefore, it supports a notion of "pointer", called ``Datum``. A datum
can store a pointer to a double, a stable pointer to a double, integers, or
pointers to anything else.

Integer Variables
-----------------
One important subset of Datum are pointers to RANGE variables. Meaning they are
pointers to parameters in other mechanisms or pointers to the parameters
associated with each node, e.g. the voltage. Since the storage of RANGE
variable is controlled by NEURON/CoreNEURON, these pointers have stronger
semantics than a ``double*``.

These make up the majority of usecases for Datum; and are considered the
well-mannered subset.

In CoreNEURON this subset of Datums are treated differently for other Datums.
Because CoreNEURON stores the values these Datums can point to in a single
contiguous array of doubles, the "pointers" can be expressed as indices into
this array.

Therefore, this subset of Datums is referred to as "integer variables".

In NEURON these pointers are a ``data_handle`` to the value they point to.
Before the simulation phase they are "resolved" and a cache stores a list of
``double*`` to the appropriate values. 

