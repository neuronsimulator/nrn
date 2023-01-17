VERBATIM
~~~~~~~~

Description:
    Sections of code surrounded by ``VERBATIM`` and ``ENDVERBATIM`` blocks are
    interpreted as literal C/C++ code.
    This feature is typically used to interface with external C/C++ libraries,
    or to use NEURON features (such as random number generation) that are not
    explicitly supported in the NMODL language.

    .. code-block::

      PROCEDURE set_foo() {
      VERBATIM
      /* literal C/C++ */
      ENDVERBATIM
        foo = 42
      }

    This is, by its nature, more fragile than exclusively using NMODL language
    constructs, but it can be necessary.
    NEURON versions newer than 8.1
    (`#1762 <https://github.com/neuronsimulator/nrn/pull/1762>`_) provide some
    C/C++ preprocessor macros that make it easier to follow incompatible changes
    in external libraries or the internal workings of NEURON.

    .. code-block:: c++

      #if NRN_VERSION_EQ(9, 0, 0)
      /* NEURON version is exactly 9.0.0 */
      #endif
      #if NRN_VERSION_NE(8, 2, 3)
      /* NEURON version is not 8.2.3 */
      #endif
      #if NRN_VERSION_GT(9, 1, 0)
      /* NEURON version is >9.1.0 */
      #endif
      #if NRN_VERSION_LT(10, 2, 0)
      /* NEURON version is <10.2.0 */
      #endif
      #if NRN_VERSION_GTEQ(8, 2, 1)
      /* NEURON version is >=8.2.1 */
      #endif
      #if NRN_VERSION_LTEQ(8, 2, 2)
      /* NEURON version is <=8.2.2 */
      #endif
      #ifndef NRN_VERSION_GTEQ_8_2_0
      /* NEURON version is <8.2.0 */
      #else
      /* NEURON version is >=8.2.0 so NRN_VERSION_{EQ,NE,GT,LT,GTEQ,LTEQ}(...)
       * are defined. */
      #endif

    ``VERBATIM`` should be used with caution and restraint, as it is very easy
    to introduce dependencies on the implementation details of NEURON and the
    NMODL language compilers and end up with MOD files that are only compatible
    with a limited range of NEURON versions.
