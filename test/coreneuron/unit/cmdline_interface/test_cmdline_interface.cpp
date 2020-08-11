/*
Copyright (c) 2016, Blue Brain Project
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
THE POSSIBILITY OF SUCH DAMAGE.
*/

#define BOOST_TEST_MODULE cmdline_interface
#define BOOST_TEST_MAIN

#include <boost/test/unit_test.hpp>
#include <cfloat>
#include "coreneuron/apps/corenrn_parameters.hpp"

using namespace coreneuron;

BOOST_AUTO_TEST_CASE(cmdline_interface) {

    const char* argv[] = {

        "nrniv-core",

        "--mpi",

        "--dt",
        "0.02",

        "--tstop",
        "0.1",

        "--gpu",

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
        "0.2"
        };

    int argc = 0;

    for (; strcmp(argv[argc], "0.2"); argc++);

    argc++;
    
    corenrn_parameters corenrn_param_test;

    corenrn_param_test.parse(argc, const_cast<char**>(argv)); //discarding const as CLI11 interface is not const
    
    BOOST_CHECK(corenrn_param_test.seed == -1);            // testing default value

    BOOST_CHECK(corenrn_param_test.spikebuf == 100);

    BOOST_CHECK(corenrn_param_test.threading == true);

    BOOST_CHECK(corenrn_param_test.dt == 0.02);

    BOOST_CHECK(corenrn_param_test.tstop == 0.1);

    BOOST_CHECK(corenrn_param_test.prcellgid == 12);

    BOOST_CHECK(corenrn_param_test.gpu == true);

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

    // check if default flags are false
    const char* argv_empty[] = {"nrniv-core"};
    argc = 1;

    corenrn_param_test.dt = 18.1;
    
    BOOST_CHECK(corenrn_param_test.dt == 18.1);

}
