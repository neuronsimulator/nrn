NEURON	{
    SUFFIX VersionMacros
    RANGE eq8_2_0_result, ne9_0_1_result, gt9_0_0_result, lt42_1_2_result, gteq10_4_7_result, lteq8_1_0_result, explicit_gteq8_2_0_result
    THREADSAFE
}

ASSIGNED {
    eq8_2_0_result
    ne9_0_1_result
    gt9_0_0_result
    lt42_1_2_result
    gteq10_4_7_result
    lteq8_1_0_result
    explicit_gteq8_2_0_result
}

BREAKPOINT {
VERBATIM
#if NRN_VERSION_EQ(8, 2, 0)
ENDVERBATIM
    eq8_2_0_result = 1
VERBATIM
#else
ENDVERBATIM
    eq8_2_0_result = 0
VERBATIM
#endif
#if NRN_VERSION_NE(9, 0, 1)
ENDVERBATIM
    ne9_0_1_result = 1
VERBATIM
#else
ENDVERBATIM
    ne9_0_1_result = 0
VERBATIM
#endif
#if NRN_VERSION_GT(9, 0, 0)
ENDVERBATIM
    gt9_0_0_result = 1
VERBATIM
#else
ENDVERBATIM
    gt9_0_0_result = 0
VERBATIM
#endif
#if NRN_VERSION_LT(42, 1, 2)
ENDVERBATIM
    lt42_1_2_result = 1
VERBATIM
#else
ENDVERBATIM
    lt42_1_2_result = 0
VERBATIM
#endif
#if NRN_VERSION_GTEQ(10, 4, 7)
ENDVERBATIM
    gteq10_4_7_result = 1
VERBATIM
#else
ENDVERBATIM
    gteq10_4_7_result = 0
VERBATIM
#endif
#if NRN_VERSION_LTEQ(8, 1, 0)
ENDVERBATIM
    lteq8_1_0_result = 1
VERBATIM
#else
ENDVERBATIM
    lteq8_1_0_result = 0
VERBATIM
#endif
#ifdef NRN_VERSION_GTEQ_8_2_0
ENDVERBATIM
    explicit_gteq8_2_0_result = 1
VERBATIM
#else
ENDVERBATIM
    explicit_gteq8_2_0_result = 0
VERBATIM
#endif
ENDVERBATIM
}
