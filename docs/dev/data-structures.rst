Data structures
###############
This section provides higher-level documentation of the data structures used in NEURON,
complementing the lower-level Doxygen documentation of the
`neuron::container:: <../doxygen/namespaceneuron_1_1container.html>`_ namespace.

NEURON 9 contains substantial changes to the organisation of model data, of which the most notable
is a transposition from an array-of-structs (AoS) layout to a struct-of-arrays (SoA) layout,
following the model of CoreNEURON.
These changes were introduced in GitHub pull request
`#2027 <https://github.com/neuronsimulator/nrn/pull/2027>`_.

As well as adopting an SoA layout, this work also introduces new "data handle" types that enable
persistent references to elements in the data structures, which automatically remain valid even
when the underlying storage arrays are re-allocated, or their elements are reordered (permuted).

The basic idea is to allow us to refer to logical elements of a NEURON model (*e.g.* "the Node at
the centre of this Section") via struct-like "handle" objects that abstract away both the size of
the underlying arrays containing the Node data, and the current index of a given (logical) Node in
those arrays.

Performance-sensitive code code, such as the Node matrix solver algorithm and "current" and "state"
functions that are generated from MOD files, can operate directly on the underlying array storage,
taking advantage of improved cache efficiency and (in some cases) vectorisation, without paying for
the relatively slow indirection inherent to the "data handle" and "Node handle" types introduced to
above.

Overview
--------
NEURON's SoA data structures are based on the
`neuron::container::soa <../doxygen/structneuron_1_1container_1_1soa.html>`_  variadic template
class.
Here is an example of its use:

.. image:: soa-architecture.svg

This defines an SoA data structure (``ab_store``) with two **data** arrays for variables
imaginatively named ``A`` (red) and ``B`` (blue).
There is an implicit extra "index" array (purple) that is needed for the implementation of the
"handle" types introduced above; no matter how many variables are added to the structure, there is
still just one index array.

Let's unpick this example a little more, starting with the definition of our ``ab_store`` type:

.. code-block:: c++

    struct ab_store: soa<ab_store, field::A, field::B>

The ``neuron::container::soa<...>`` template uses
`CRTP <https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern>`_, which is why the
first template argument to ``soa<...>`` is the derived class name ``ab_store``; the reasons for
this are not important for a high-level overview and it can be ignored for the moment.

All the remaining template arguments, ``field::A, field::B`` in this case, are **tag types** that
define the **fields** of our data structure.
A minimal definition of these would be something like:

.. code-block:: c++

    namespace field {

    struct A {
      using type = double;
    };

    struct B {
      using type = int;
    };

    }  // namespace field

Which would specify that the ``A`` values shown above (red ``a0``, ``a1``, *etc.*) are of type
``double``, while the ``B`` values (blue ``b0``, ``b1``, *etc.*) are of type ``int``.
Certain additional functions and variables can be added to the tag types to control, for example,
pretty-printing of data handles, non-zero default values, and non-scalar fields.

In essence, our struct ``ab_store``, if we wrote it out manually, would look something like:

.. code-block:: c++

    struct ab_store_manual {
      std::vector<double> a_values;
      std::vector<int> b_values;
      std::vector</* unspecified */> indices;
    };

As with any other ``std::vector``-based type, the currently allocated capacity and the current size
are different to one another; in the illustration above there are five elements (the size), but
there are two unused elements (``...``) in each array, so the capacity is seven.

The index column has so far been glossed over, but you may have noticed that, for example,
0\ :sup:`th` entry is shown as "→ 0" in the figure above.
In essence, the index column type is "pointer to integer", and the pointed-to integers are kept up
to date so they always hold the **current** index into the storage array of a given logical entry.
Expressed as code, this means that:

.. code-block:: c++

    for (auto i = 0; i < indices.size(); ++i) {
      assert(i == *indices[i]);
    }

should never trigger an error.

.. note::

    While a large part of the motivation for allowing the underlying arrays to be reordered is that
    it allows explicit permutation of the data for performance reasons, it also permits other
    optimisations. For example, deletion from NEURON's data structures is :math:`\mathcal{O}(1)`,
    whereas erasing from a regular ``std::vector`` is :math:`\mathcal{O}(N)`. This is because the
    index mechanism allows deletion to be implemented by swapping the last element of the array
    into the deleted position and reducing the size by one.

The data handle type described above essentially hold a pair of pointers: one that can be
dereferenced to get the address of the 0\ :sup:`th` entry in the data array, and one
pointer-to-integer taken from the index column:

.. code-block:: c++

    struct data_handle_double_manual {
      double* const* ptr_to_array_base;
      unsigned long* ptr_to_current_row;
      double& get_value() {
        return (*ptr_to_array_base)[*ptr_to_current_row];
      }
    };

This is enough indirection that neither re-ordering nor re-allocating the actual data storage
invalidates any instances of ``data_handle_double_manual``.
The real type used in the NEURON code-base is the
`neuron::container::data_handle <../doxygen/structneuron_1_1container_1_1data__handle.html>`_
template, *i.e.* we use ``data_handle<double>`` in place of ``data_handle_double_manual``.

.. note::

    You may wonder what happens when an entry is deleted from the data structures. In this case the
    storage for the **data** of the deleted element (*i.e.* its ``a`` and ``b`` values) is released
    and made available for re-use, but its entry in the index vector is not freed and the
    pointed-to integer is updated with a sentinel value. This means that data handles that referred
    to now-deleted entries (``a`` and ``b`` values) can detect that they are no longer valid and
    will not return invalid values.

Of course, this indirection also means that these data handles are not especially performant, but
in general they are intended to solve otherwise-tedious bookkeeping problems, and
performance-critical code is expected to operate directly on the underlying vectors.
In other cases, such as ``POINTER`` variables in MOD files, data handles are used while the model
is being built in memory, but they are "flattened" into plain ``double*`` for use during the actual
simulation, where performance **is** important **and** it is known that no re-allocation or
re-ordering will occur that could invalidate those raw pointers.

The "data handle" type just discussed is the right tool for the job if we want to refer to a single
value of a type that is known at compile time, but there are a few other types of "handle" that are
also supported:

* `neuron::container::generic_data_handle <../doxygen/structneuron_1_1container_1_1generic__data__handle.html>`_
  is a type-erased version of ``neuron::container::data_handle``, similar to
  `std::any <https://en.cppreference.com/w/cpp/utility/any>`_.
* Handles to higher-level objects. For example, if the entity that has an "a" [side] and a "b"
  [side] is a vinyl, we can also have "vinyl handles", which provide accessors ``a()`` and ``b()``.
  These handles come in two flavours:

  * non-owning: like the "data handle" types above, these refer to an entry in the ``ab_store``
    container and provide access to both "a" and "b" [these are currently not used]
  * owning: like non-owning handles, these refer to an entry in the ``ab_store`` container and
    provide access to both "a" and "b" values. The key difference is that owning handles have
    owning semantics: creating an owning handle appends a new entry to the underlying data arrays
    and destroying an owning handle deletes that (owned) entry from the arrays.

The following code snippet illustrates the use of owning handles:

.. code-block:: c++

    ab_store my_data{};
    data_handle<double> dh{};
    assert(!dh); // not pointing to a valid value
    {
      // my_data.size() == 0
      owning_vinyl_handle heroes{my_data}; 
      // now my_data.size() == 1
      heroes.a() = 19.817; // runtime in minutes
      heroes.b() = 5;      // track count
      // higher-level handles-to-entities can produce lower-level handles-to-values
      dh = heroes.b_handle();
      assert(dh); // now pointing to a valid value
      assert(*dh == 5);
      *dh = 6; // bonus track
      assert(heroes.b() == 6); // `dh` and `heroes.b()` refer to the same value
    } // `heroes` is destroyed when it goes out of scope at this semicolon
    // my_data.size() == 0 again 
    assert(!dh); // pointed-to value is no longer valid

</laboured_vinyl_analogies>