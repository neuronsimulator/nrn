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


/**
 * @file nrnoptarg.h
 * @date 26 Oct 2014
 *
 * @brief structure storing all command line arguments for coreneuron
 */

#ifndef nrnoptarg_h
#define nrnoptarg_h

#include <getopt.h>
#include "coreneuron/utils/sdprintf.h"

typedef struct cn_parameters {

    double tstart; 		/**< start time of simulation in msec*/
    double tstop;		/**< stop time of simulation in msec*/
    double dt;			/**< timestep to use in msec*/
    double dt_io;               /**< i/o timestep to use in msec*/

    double celsius;
    double voltage;
    double maxdelay;

    double forwardskip;

    int spikebuf;		/**< internal buffer used on evry rank for spikes */
    int prcellgid; 		/**< gid of cell for prcellstate */

    int threading;		/**< enable pthread/openmp  */

    const char *patternstim;
    const char *datpath;		/**< directory path where .dat files */
    const char *outpath; 		/**< directory where spikes will be written */
    const char *filesdat; 		/**< name of file containing list of gids dat files read in */
   
    double mindelay;

    /** default constructor */ 
    cn_parameters();

    /** show help message for command line args */ 
    void show_cb_opts_help();

    /** show all parameter values */ 
    void show_cb_opts();

    /** read options from command line */ 
    void read_cb_opts( int argc, char **argv );

    /** return full path of files.dat file */ 
    sd_ptr get_filesdat_path( char *path_buf, size_t bufsz );

    /** store/set computed mindelay argument */ 
    void set_mindelay(double mdelay) {
        mindelay = mdelay;
    }

} cn_input_params;

#endif

