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

BOOST_AUTO_TEST_CASE(cmdline_interface) 
{
  cn_input_params input_params;
  int argc = 14; 
  char ** argv = new char*[argc];
  argv[0] = (char*)"executable";
  argv[1] = (char*)"--tstart=0.001";
  argv[2] = (char*)"--tstop=0.1";
  argv[3] = (char*)"--dt=0.02";
  argv[4] = (char*)"--dt_io=0.2";
  argv[5] = (char*)"--celsius=25.12";
  argv[6] = (char*)"--pattern=filespike.dat";
  argv[7] = (char*)"--spikebuf=100";
  argv[8] = (char*)"--prcellgid=12";
  argv[9] = (char*)"--threading";
  argv[10] = (char*)"--datpath=/this/is/the/path";
  argv[11] = (char*)"--filesdat=/this/is/the/path";
  argv[12] = (char*)"--outpath=/this/is/the/path";
  argv[13] = (char*)"--forwardskip=0.02";

  input_params.read_cb_opts(argc,argv);

  BOOST_CHECK_CLOSE(input_params.tstart,0.001,DBL_EPSILON);
  BOOST_CHECK_CLOSE(input_params.tstop,0.1, DBL_EPSILON);
  BOOST_CHECK_CLOSE(input_params.dt,0.02, DBL_EPSILON);
  BOOST_CHECK_CLOSE(input_params.dt_io,0.2, DBL_EPSILON);
  BOOST_CHECK_CLOSE(input_params.celsius,25.12, DBL_EPSILON);
  BOOST_CHECK(input_params.prcellgid==12);
  BOOST_CHECK_CLOSE(input_params.forwardskip,0.02, DBL_EPSILON);
  BOOST_CHECK(!strcmp(input_params.outpath,(char*)"/this/is/the/path"));
  BOOST_CHECK(!strcmp(input_params.datpath,(char*)"/this/is/the/path"));
  BOOST_CHECK(input_params.threading==1);
  BOOST_CHECK(!strcmp(input_params.patternstim,(char*)"filespike.dat"));
  BOOST_CHECK(!strcmp(input_params.filesdat,(char*)"/this/is/the/path"));
  BOOST_CHECK(input_params.spikebuf==100);

  delete [] argv;
}
    
