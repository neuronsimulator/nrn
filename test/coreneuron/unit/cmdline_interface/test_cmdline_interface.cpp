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
#include "nrniv/nrnoptarg.h"
#include <float.h>

BOOST_AUTO_TEST_CASE(cmdline_interface) {

    const char* argv[] = {

        "coreneuron_exec",

        "--spikebuf",
        "100",

        "--threading",

        "--datpath",
        "/this/is/the/data/path",

        "--checkpoint",
        "/this/is/the/chkp/path",

        "--dt",
        "0.02",

        "--tstop",
        "0.1",

        "--filesdat",
        "/this/is/the/file/path",

        "--prcellgid",
        "12",

        "--gpu",

        "--dt_io",
        "0.2",

        "--forwardskip",
        "0.02",

        "--celsius",
        "25.12",

        "-mpi",

        "--outpath",
        "/this/is/the/output/path",

        "--pattern",
        "filespike.dat",

        "--report-conf",
        "report.conf",

        "--cell-permute",
        "2",

        "--voltage",
        "-32",

        "--nwarp",
        "8",

        "--extracon",
        "1000",

        "--multiple",
        "3",

        "--binqueue",

        "--mindelay",
        "0.1",

        "--ms-phases",
        "1",

        "--ms-subintervals",
        "2",

        "--spkcompress",
        "32",

        "--multisend"};

    int argc = 0;

    for (; strcmp(argv[argc], "--multisend"); argc++)
        ;

    argc++;

    nrnopt_parse(argc, argv);

    BOOST_CHECK(nrnopt_get_int("--spikebuf") == 100);

    BOOST_CHECK(nrnopt_get_flag("--threading") == true);

    BOOST_CHECK(!strcmp(nrnopt_get_str("--datpath").c_str(), "/this/is/the/data/path"));

    BOOST_CHECK(!strcmp(nrnopt_get_str("--checkpoint").c_str(), "/this/is/the/chkp/path"));

    BOOST_CHECK(nrnopt_get_dbl("--dt") == 0.02);

    BOOST_CHECK(nrnopt_get_dbl("--tstop") == 0.1);

    BOOST_CHECK(!strcmp(nrnopt_get_str("--filesdat").c_str(), "/this/is/the/file/path"));

    BOOST_CHECK(nrnopt_get_int("--prcellgid") == 12);

    BOOST_CHECK(nrnopt_get_flag("--gpu") == true);

    BOOST_CHECK(nrnopt_get_dbl("--dt_io") == 0.2);

    BOOST_CHECK(nrnopt_get_dbl("--forwardskip") == 0.02);

    BOOST_CHECK(nrnopt_get_dbl("--celsius") == 25.12);

    BOOST_CHECK(nrnopt_get_flag("-mpi") == true);

    BOOST_CHECK(!strcmp(nrnopt_get_str("--outpath").c_str(), "/this/is/the/output/path"));

    BOOST_CHECK(!strcmp(nrnopt_get_str("--pattern").c_str(), "filespike.dat"));

    BOOST_CHECK(!strcmp(nrnopt_get_str("--report-conf").c_str(), "report.conf"));

    BOOST_CHECK(nrnopt_get_int("--cell-permute") == 2);

    BOOST_CHECK(nrnopt_get_dbl("--voltage") == -32);

    BOOST_CHECK(nrnopt_get_int("--nwarp") == 8);

    BOOST_CHECK(nrnopt_get_int("--extracon") == 1000);

    BOOST_CHECK(nrnopt_get_int("--multiple") == 3);

    BOOST_CHECK(nrnopt_get_flag("--binqueue") == true);

    BOOST_CHECK(nrnopt_get_dbl("--mindelay") == 0.1);

    BOOST_CHECK(nrnopt_get_int("--ms-phases") == 1);

    BOOST_CHECK(nrnopt_get_int("--ms-subintervals") == 2);

    BOOST_CHECK(nrnopt_get_int("--spkcompress") == 32);

    BOOST_CHECK(nrnopt_get_flag("--multisend") == true);


    // check if nrnopt_modify_dbl works properly
    nrnopt_modify_dbl("--dt", 18.1);
    BOOST_CHECK(nrnopt_get_dbl("--dt") == 18.1);

    // check if default flags are false
    const char* argv_empty[] = {"coreneuron_exec"};
    argc = 1;

    nrnopt_parse(argc, argv_empty);

    BOOST_CHECK(nrnopt_get_flag("--threading") == false);
    BOOST_CHECK(nrnopt_get_flag("--gpu") == false);
    BOOST_CHECK(nrnopt_get_flag("-mpi") == false);
    BOOST_CHECK(nrnopt_get_flag("--binqueue") == false);
    BOOST_CHECK(nrnopt_get_flag("--multisend") == false);
}
