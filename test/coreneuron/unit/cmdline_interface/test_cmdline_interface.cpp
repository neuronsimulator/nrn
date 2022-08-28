/*
# =============================================================================
# Copyright (c) 2016 - 2022 Blue Brain Project/EPFL
#
# See top-level LICENSE file for details.
# =============================================================================.
*/
#include "coreneuron/apps/corenrn_parameters.hpp"

#define BOOST_TEST_MODULE cmdline_interface
#include <boost/test/included/unit_test.hpp>

#include <cfloat>

using namespace coreneuron;

BOOST_AUTO_TEST_CASE(cmdline_interface) {
    const char* argv[] = {

        "nrniv-core",

        "--mpi",

        "--dt",
        "0.02",

        "--tstop",
        "0.1",
#ifdef CORENEURON_ENABLE_GPU
        "--gpu",
#endif
        "--cell-permute",
        "2",

        "--nwarp",
        "8",

        "-d",
        "./",

        "--voltage",
        "-32",

        "--threading",

        "--ms-phases",
        "1",

        "--ms-subintervals",
        "2",

        "--multisend",

        "--spkcompress",
        "32",

        "--binqueue",

        "--spikebuf",
        "100",

        "--prcellgid",
        "12",

        "--forwardskip",
        "0.02",

        "--celsius",
        "25.12",

        "--mindelay",
        "0.1",

        "--dt_io",
        "0.2"};
    constexpr int argc = sizeof argv / sizeof argv[0];

    corenrn_parameters corenrn_param_test;

    corenrn_param_test.parse(argc, const_cast<char**>(argv));  // discarding const as CLI11
                                                               // interface is not const

    BOOST_CHECK(corenrn_param_test.seed == -1);  // testing default value

    BOOST_CHECK(corenrn_param_test.spikebuf == 100);

    BOOST_CHECK(corenrn_param_test.threading == true);

    BOOST_CHECK(corenrn_param_test.dt == 0.02);

    BOOST_CHECK(corenrn_param_test.tstop == 0.1);

    BOOST_CHECK(corenrn_param_test.prcellgid == 12);
#ifdef CORENEURON_ENABLE_GPU
    BOOST_CHECK(corenrn_param_test.gpu == true);
#else
    BOOST_CHECK(corenrn_param_test.gpu == false);
#endif
    BOOST_CHECK(corenrn_param_test.dt_io == 0.2);

    BOOST_CHECK(corenrn_param_test.forwardskip == 0.02);

    BOOST_CHECK(corenrn_param_test.celsius == 25.12);

    BOOST_CHECK(corenrn_param_test.mpi_enable == true);

    BOOST_CHECK(corenrn_param_test.cell_interleave_permute == 2);

    BOOST_CHECK(corenrn_param_test.voltage == -32);

    BOOST_CHECK(corenrn_param_test.nwarp == 8);

    BOOST_CHECK(corenrn_param_test.multisend == true);

    BOOST_CHECK(corenrn_param_test.mindelay == 0.1);

    BOOST_CHECK(corenrn_param_test.ms_phases == 1);

    BOOST_CHECK(corenrn_param_test.ms_subint == 2);

    BOOST_CHECK(corenrn_param_test.spkcompress == 32);

    BOOST_CHECK(corenrn_param_test.multisend == true);

    // Reset all parameters to their default values.
    corenrn_param_test.reset();

    // Should match a default-constructed set of parameters.
    BOOST_CHECK_EQUAL(corenrn_param_test.voltage, corenrn_parameters{}.voltage);

    // Everything has its default value, and the first `false` says not to
    // include default values in the output, so this should be empty
    BOOST_CHECK(corenrn_param_test.config_to_str(false, false).empty());
}
