.. include:: ../rst_substitutions.txt

.. _porting-mechanisms-to-cpp:

Adapting MOD files for C++ with |neuron_with_cpp_mechanisms|
============================================================

In older versions of NEURON, MOD files containing NMODL code were translated
into C code before being compiled and executed by NEURON.
Starting with |neuron_with_cpp_mechanisms|, NMODL code is translated into C++
code instead.

In most cases, this does not present any issues, as simple C code is typically
valid C++, and no changes are required.
However, C and C++ are not the same language, and there are cases in which MOD
files containing ``VERBATIM`` blocks need to be modified in order to build with
|neuron_with_cpp_mechanisms|.

Before you start, you should decide if you need your MOD files to be
compatible simultaneously with |neuron_with_cpp_mechanisms| **and** older
versions, or if you can safely stop supporting older versions.
Supporting both is generally possible, but it may be more cumbersome than
committing to using C++ features.
Considering NEURON has maintained strong backward compatibility and its
internal numerical methods haven't changed with migration to C++, it is likely
to be sufficient to adapt MOD files to C++ only and use
|neuron_with_cpp_mechanisms|.
If you do decide to preserve compatibility across versions, the preprocessor
macros described in :ref:`python_verbatim` may prove useful.

.. note::
  If you have a model that stopped compiling when you upgraded to or beyond
  |neuron_with_cpp_mechanisms|, the first thing that you should check is
  whether the relevant MOD files have already been updated in ModelDB or in the
  GitHub repository of that model. You can check the repository name with the
  model accession number under `<https://github.com/ModelDBRepository>`_.
  An updated version may already be available!

  The following models were updated in ModelDB in preparation for
  |neuron_with_cpp_mechanisms| and may serve as useful references:
  `2487 <https://github.com/ModelDBRepository/2487/pull/1>`_,
  `2730 <https://github.com/ModelDBRepository/2730/pull/1>`_,
  `2733 <https://github.com/ModelDBRepository/2733/pull/1>`_,
  `3658 <https://github.com/ModelDBRepository/3658/pull/1>`_,
  `7399 <https://github.com/ModelDBRepository/7399/pull/1>`_,
  `7400 <https://github.com/ModelDBRepository/7400/pull/1>`_,
  `8284 <https://github.com/ModelDBRepository/8284/pull/1>`_,
  `9889 <https://github.com/ModelDBRepository/9889/pull/1>`_,
  `12631 <https://github.com/ModelDBRepository/12631/pull/2>`_,
  `26997 <https://github.com/ModelDBRepository/26997/pull/1>`_,
  `35358 <https://github.com/ModelDBRepository/35358/pull/2>`_,
  `37819 <https://github.com/ModelDBRepository/37819/pull/1>`_,
  `51781 <https://github.com/ModelDBRepository/51781/pull/1>`_,
  `52034 <https://github.com/ModelDBRepository/52034/pull/1>`_,
  `64229 <https://github.com/ModelDBRepository/64229/pull/1>`_,
  `64296 <https://github.com/ModelDBRepository/64296/pull/1>`_,
  `87585 <https://github.com/ModelDBRepository/87585/pull/1>`_,
  `93321 <https://github.com/ModelDBRepository/93321/pull/1>`_,
  `97868 <https://github.com/ModelDBRepository/97868/pull/2>`_,
  `97874 <https://github.com/ModelDBRepository/97874/pull/2>`_,
  `97917 <https://github.com/ModelDBRepository/97917/pull/2>`_,
  `105507 <https://github.com/ModelDBRepository/105507/pull/2>`_,
  `106891 <https://github.com/ModelDBRepository/106891/pull/3>`_,
  `113732 <https://github.com/ModelDBRepository/113732/pull/1>`_,
  `116094 <https://github.com/ModelDBRepository/116094/pull/1>`_,
  `116830 <https://github.com/ModelDBRepository/116830/pull/1>`_,
  `116838 <https://github.com/ModelDBRepository/116838/pull/1>`_,
  `116862 <https://github.com/ModelDBRepository/116862/pull/1>`_,
  `123815 <https://github.com/ModelDBRepository/123815/pull/1>`_,
  `136095 <https://github.com/ModelDBRepository/136095/pull/1>`_,
  `136310 <https://github.com/ModelDBRepository/136310/pull/1>`_,
  `137845 <https://github.com/ModelDBRepository/137845/pull/1>`_,
  `138379 <https://github.com/ModelDBRepository/138379/pull/1>`_,
  `139421 <https://github.com/ModelDBRepository/139421/pull/1>`_,
  `140881 <https://github.com/ModelDBRepository/140881/pull/1>`_,
  `141505 <https://github.com/ModelDBRepository/141505/pull/1>`_,
  `144538 <https://github.com/ModelDBRepository/144538/pull/1>`_,
  `144549 <https://github.com/ModelDBRepository/144549/pull/1>`_,
  `144586 <https://github.com/ModelDBRepository/144586/pull/1>`_,
  `146949 <https://github.com/ModelDBRepository/146949/pull/1>`_,
  `149000 <https://github.com/ModelDBRepository/149000/pull/1>`_,
  `149739 <https://github.com/ModelDBRepository/149739/pull/1>`_,
  `150240 <https://github.com/ModelDBRepository/150240/pull/1>`_,
  `150245 <https://github.com/ModelDBRepository/150245/pull/1>`_,
  `150551 <https://github.com/ModelDBRepository/150551/pull/1>`_,
  `150556 <https://github.com/ModelDBRepository/150556/pull/1>`_,
  `150691 <https://github.com/ModelDBRepository/150691/pull/1>`_,
  `151126 <https://github.com/ModelDBRepository/151126/pull/1>`_,
  `151282 <https://github.com/ModelDBRepository/151282/pull/1>`_,
  `153280 <https://github.com/ModelDBRepository/153280/pull/1>`_,
  `154732 <https://github.com/ModelDBRepository/154732/pull/1>`_,
  `155568 <https://github.com/ModelDBRepository/155568/pull/1>`_,
  `155601 <https://github.com/ModelDBRepository/155601/pull/1>`_,
  `155602 <https://github.com/ModelDBRepository/155602/pull/1>`_,
  `156780 <https://github.com/ModelDBRepository/156780/pull/1>`_,
  `157157 <https://github.com/ModelDBRepository/157157/pull/1>`_,
  `168874 <https://github.com/ModelDBRepository/168874/pull/4>`_,
  `181967 <https://github.com/ModelDBRepository/181967/pull/1>`_,
  `182129 <https://github.com/ModelDBRepository/182129/pull/1>`_,
  `183300 <https://github.com/ModelDBRepository/183300/pull/1>`_,
  `185355 <https://github.com/ModelDBRepository/185355/pull/2>`_,
  `185858 <https://github.com/ModelDBRepository/185858/pull/1>`_,
  `186768 <https://github.com/ModelDBRepository/186768/pull/1>`_,
  `187604 <https://github.com/ModelDBRepository/187604/pull/2>`_,
  `189154 <https://github.com/ModelDBRepository/189154/pull/1>`_,
  `194897 <https://github.com/ModelDBRepository/194897/pull/2>`_,
  `195615 <https://github.com/ModelDBRepository/195615/pull/1>`_,
  `223031 <https://github.com/ModelDBRepository/223031/pull/1>`_,
  `225080 <https://github.com/ModelDBRepository/225080/pull/1>`_,
  `231427 <https://github.com/ModelDBRepository/231427/pull/2>`_,
  `232097 <https://github.com/ModelDBRepository/232097/pull/1>`_,
  `239177 <https://github.com/ModelDBRepository/239177/pull/1>`_,
  `241165 <https://github.com/ModelDBRepository/241165/pull/1>`_,
  `241240 <https://github.com/ModelDBRepository/241240/pull/1>`_,
  `244262 <https://github.com/ModelDBRepository/244262/pull/1>`_,
  `244848 <https://github.com/ModelDBRepository/244848/pull/2>`_,
  `247968 <https://github.com/ModelDBRepository/247968/pull/1>`_,
  `249463 <https://github.com/ModelDBRepository/249463/pull/1>`_,
  `256388 <https://github.com/ModelDBRepository/256388/pull/1>`_ and
  `259366 <https://github.com/ModelDBRepository/259366/pull/1>`_.

..
  Does this need some more qualification? Are there non-VERBATIM
  incompatibilities?

Legacy patterns that are invalid C++
------------------------------------

This section aims to list some patterns that the NEURON developers have found
in pre-|neuron_with_cpp_mechanisms| models that need to be modified to be valid
C++.

Implicit pointer type conversions
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
C++ has stricter rules governing pointer type conversions than C. For example

.. code-block:: cpp

  double* x = (void*)0;   // valid C, invalid C++
  double* x = nullptr;    // valid C++, invalid C
  double* x = (double*)0; // valid C and C++ (C-style casts discouraged in C++)

Similarly, in C one can pass a ``void*`` argument to a function that expects a
``double*``. In C++ this is forbidden.

The same issue may manifest itself in code such as

.. code-block:: cpp

  double* x = malloc(7 * sizeof(double));          // valid C, invalid C++
  double* x = new double[7];                       // valid C++, invalid C
  double* x = (double*)malloc(7 * sizeof(double)); // valid C and C++ (C-style casts discouraged in C++)

If you choose to move from using C ``malloc`` and ``free`` to using C++ ``new``
and ``delete`` then remember that you cannot mix and match ``new`` with ``free``
and so on.

.. note::
  Explicit memory management with ``new`` and ``delete`` is discouraged in C++
  (`R.11: Avoid calling new and delete explicitly <https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#Rr-newdelete>`_).
  If you do not need to support older versions of NEURON, you may be able to
  use standard library containers such as ``std::vector<T>``.

.. _local-non-prototype-function-declaration:

Local non-prototype function declarations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In C, the function declaration

.. code-block:: c

  void some_function();

is a `non-prototype function declaration
<https://en.cppreference.com/w/c/language/function_declaration>`_: it declares
that ``some_function`` exists, but it does not specify the number of arguments
that it takes, or their types.
In C++, the same code declares that ``some_function`` takes zero arguments (the
C equivalent of this is ``void some_function(void)``).

If such a declaration occurs in a top-level ``VERBATIM`` block then it is
likely to be harmless: it will add a zero-parameter overload of the function,
which will never be called.
It **will**, however, cause a problem if the declaration is included **inside**
an NMODL construct

.. code-block:: c

  PROCEDURE procedure() {
  VERBATIM
    void some_method_taking_an_int(); // problematic in C++
    some_method_taking_an_int(42);
  ENDVERBATIM
  }

because in this case the local declaration hides the real declaration, which is
something like

.. code-block:: c

  void some_method_taking_an_int(int);

in a NEURON header that is included when the translated MOD file is compiled.
In this case, the problematic local declaration can simply be removed.

.. _function-decls-with-incorrect-types:

Function declarations with incorrect types
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
In older MOD files and versions of NEURON, API methods were often accessed by
declaring them in the MOD file and not by including a correct declaration from
NEURON itself.
In NEURON 8.2+, more declarations are implicitly included when MOD files are
compiled.
This can lead to problems if the declaration in the MOD file did not specify
the correct argument and return types.

.. code-block:: cpp

  Rand* nrn_random_arg(int); // correct declaration from NEURON header, only in new NEURON
  /* ... */
  void* nrn_random_arg(int); // incorrect declaration in MOD file, clashes with previous line

The fix here is simply to remove the incorrect declaration from the MOD file.

If the argument types are incorrect, the situation is slightly more nuanced.
This is because C++ supports overloading functions by argument type, but not by
return type.

.. code-block:: cpp

  void sink(Thing*); // correct declaration from NEURON header, only in new NEURON
  /* ... */
  void sink(void*); // incorrect declaration in MOD file, adds an overload, not a compilation error
  /* ... */
  void* void_ptr;
  sink(void_ptr);  // probably used to work, now causes a linker error
  Thing* thing_ptr;
  sink(thing_ptr); // works with both old and new NEURON

Here the incorrect declaration ``sink(void*)`` declares a second overload of
the ``sink`` function, which is not defined anywhere.
With |neuron_with_cpp_mechanisms| the ``sink(void_ptr)`` line will select the
``void*`` second overload, based on the argument type, and this will fail during
linking because this overload is not defined (only ``sink(Thing*)`` has a
definition).

In contrast, ``sink(thing_ptr)`` will select the correct overload in
|neuron_with_cpp_mechanisms|, and it also works in older NEURON versions
because ``Thing*`` can be implicitly converted to ``void*``.

The fix here is, again, to remove the incorrect declaration from the MOD file.

See also the section below, :ref:`deprecated-overloads-taking-void`, for cases
where NEURON **does** provide a (deprecated) definition of the ``void*``
overload.

K&R function definitions
^^^^^^^^^^^^^^^^^^^^^^^^
C supports (`until C23
<https://en.cppreference.com/w/c/language/function_definition>`_) a legacy
("K&R") syntax for function declarations.
This is not valid C++.

There is no advantage to the legacy syntax. If you have legacy definitions such
as

.. code-block:: c

  void foo(a, b) int a, b; { /* ... */ }

then simply replace them with

.. code-block:: cpp

  void foo(int a, int b) { /* ... */ }

which is valid in both C and C++.

Legacy patterns that are considered deprecated
----------------------------------------------
As noted above (:ref:`local-non-prototype-function-declaration`), declarations
such as

.. code-block:: c

  VERBATIM
  extern void vector_resize();
  ENDVERBATIM

at the global scope in MOD files declare C++ function overloads that take no
parameters.
If such declarations appear at the global scope then they do not hide the
correct declarations, so this can be harmless, but it is not necessary.
In |neuron_with_cpp_mechanisms| the full declarations of the NEURON API methods
that can be used in MOD files are implicitly included via the ``mech_api.h``
header, so this explicit declaration of a zero-parameter overload is not needed
and can safely be removed.

.. _deprecated-overloads-taking-void:

Deprecated overloads taking ``void*``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
As noted above (:ref:`function-decls-with-incorrect-types`),
|neuron_with_cpp_mechanisms| provides extra overloads for some API methods that
aim to smooth the transition from C to C++.
These overloads are provided for widely used methods, and where overloading on
return type would not be required.

An example is the ``vector_capacity`` function, which in
|neuron_with_cpp_mechanisms| has two overloads

.. code-block:: cpp

  int vector_capacity(IvocVect*);
  [[deprecated("non-void* overloads are preferred")]] int vector_capacity(void*);

The second one simply casts its argument to ``IvocVect*`` and calls the first
one.
The ``[[deprecated]]`` attribute means that MOD files that use the second
overload emit compilation warnings when they are compiled using ``nrnivmodl``.

If your MOD files produce these deprecation warnings, make sure that the
relevant method (``vector_capacity`` in this example) is being called with an
argument of the correct type (``IvocVect*``), and not a type that is implicitly
converted to ``void*``.

Legacy random number generators and API
---------------------------------------

Various changes have also been done in the API of NEURON functions related to random 
number generators.

First, in |neuron_with_cpp_mechanisms| parameters passed to the functions need to be
of the correct type as it was already mentioned in :ref:`function-decls-with-incorrect-types`.
The most usual consequence of that is that NEURON random API functions that were taking as
an argument a ``void*`` now need to called with a ``Rand*``. An example of the changes needed
to fix this issue is given in `182129 <https://github.com/ModelDBRepository/182129/pull/1>`_.

Another related change is with ``scop_rand()`` function that is usually used for defining a
``URAND`` ``FUNCTION`` in mod files to return a number based on a uniform distribution from 0 to 1.
This function now takes no argument anymore. An example of this change can also be found in
`182129 <https://github.com/ModelDBRepository/182129/pull/1>`_.

Finally, the preferred random number generator is ``Random123``. You can find more information
about that in :meth:`Random.Random123` and :ref:`Randomness in NEURON models`. An example of the
usage of ``Random123`` can be seen in `netstim.mod <https://github.com/neuronsimulator/nrn/blob/master/src/nrnoc/netstim.mod>`_
and its `corresponding test <https://github.com/neuronsimulator/nrn/blob/master/test/coreneuron/test_psolve.py#L60>`_.`
Another important aspect of ``Random123`` is that it's supported in CoreNEURON as well. For more
information about this see :ref:`Random Number Generators: Random123 vs MCellRan4`.