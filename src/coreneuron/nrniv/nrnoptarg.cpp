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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "coreneuron/nrniv/nrnoptarg.h"
#include "coreneuron/utils/sdprintf.h"

extern "C" void nrn_exit( int );
extern int nrnmpi_myid;

cn_parameters::cn_parameters()
{
    tstart = 0.0;
    tstop = 100.0;
    dt = -1000.;

    dt_io = 0.1;
    dt_report = 0.1;

    celsius = -1000.0; // precedence: set by user, globals.dat, 34.0
    voltage = -65.0;
    maxdelay = 10.0;

    forwardskip = 0;

    spikebuf = 100000;
    prcellgid = -1;

    threading = 0;
    compute_gpu = 0;
    cell_interleave_permute = 0;
    nwarp = 0; /* 0 means not specified */
    report = 0;

    patternstim = NULL;

    datpath = ".";
    outpath = ".";
    filesdat = "files.dat";

    multiple = 1;
    extracon = 0;
}

sd_ptr cn_parameters::get_filesdat_path( char *path_buf, size_t bufsz )
{
    // shouldn't we check if filesdat is absolute or relative? -- sgy 20150119
    return sdprintf( path_buf, bufsz, "%s/%s", datpath, filesdat );
}

void cn_parameters::show_cb_opts()
{
    if ( nrnmpi_myid == 0 ) {
        printf( "\n Configuration Parameters" );

        printf( "\n tstart: %g, tstop: %g, dt: %g, dt_io: %g", tstart, tstop, dt, dt_io );
        printf( " celsius: %g, voltage: %g, maxdelay: %g", celsius, voltage, maxdelay );

        printf( "\n forwardskip: %g, spikebuf: %d, prcellgid: %d, multiple: %d, extracon: %d", \
                forwardskip, spikebuf, prcellgid, multiple, extracon);
        printf( "\n threading : %d, mindelay : %g, cell_permute: %d, nwarp: %d", \
		threading, mindelay, cell_interleave_permute, nwarp);

        printf( "\n patternstim: %s, datpath: %s, filesdat: %s, outpath: %s", \
                patternstim, datpath, filesdat, outpath );

        printf( "\n report: %d, report dt: %lf ", report, dt_report);

        if ( prcellgid >= 0 ) {
            printf( "\n prcellstate will be called for gid %d", prcellgid );
        }

        printf( "\n\n" );
    }
}


void cn_parameters::show_cb_opts_help()
{
    printf( "\nWelcome to CoreNeuron!\n\nOPTIONS\n\
       -h, -?, --help Print a usage message briefly summarizing these command-line options \
		and the bug-reporting address, then exit.\n\n\
       -s TIME, --tstart=TIME\n\
              Set the start time to TIME (double). The default value is '0.'\n\n\
       -e TIME, --tstop=TIME\n\
              Set the stop time to TIME (double). The default value is '100.'\n\n\
       -t TIME, --dt=TIME\n\
              Set the dt time to TIME (double). The default value is set by defaults.dat, otherwise '0.025'.\n\n\
       -i TIME, --dt_io=TIME\n\
              Set the dt of I/O to TIME (double). The default value is '0.1'.\n\n\
       -v FLOAT, --voltage=v_init\n\
              Value used for nrn_finitialize(1, v_init). If 1000, then nrn_finitialize(0,...)\n\
       -l NUMBER, --celsius=NUMBER\n\
              Set the celsius temperature to NUMBER (double). The default value set by defaults.dat, othewise '34.0'.\n\n\
       -p FILE, --pattern=FILE\n\
              Apply patternstim with the spike file FILE (char*). The default value is 'NULL'.\n\n\
       -b SIZE, --spikebuf=SIZE\n\
              Set the spike buffer to be of the size SIZE (int). The default value is '100000'.\n\n\
       -g NUMBER, --prcellgid=NUMBER\n\
              Output prcellstate information for the gid NUMBER (int). The default value is '-1'.\n\n\
       -c, --threading\n\
              Option to enable threading. The default implies no threading.\n\n\
       -a, --gpu\n\
              Option to enable use of GPUs. The default implies cpu only run.\n\n\
       -R NUMBER, --cell_permute=NUMBER\n\
              Cell permutation and interleaving for efficiency\n\n\
       -W NUMBER, --nwarp=NUMBER\n\
              number of warps to balance\n\n\
       -d PATH, --datpath=PATH\n\
              Set the path with required CoreNeuron data to PATH (char*). The default value is '.'.\n\n\
       -f FILE, --filesdat=FILE\n\
              Absolute path with the name for the required file FILE (char*). The default value is 'files.dat'.\n\
       -o PATH, --outpath=PATH\n\
              Set the path for the output data to PATH (char*). The default value is '.'.\n\
       -k TIME, --forwardskip=TIME\n\
              Set forwardskip to TIME (double). The default value is '0.'.\n\
       -r TYPE --report=TYPE\n\
              Enable voltage report with specificied type (0 for disable, 1 for soma, 2 for full compartment).\n\
       -w, --dt_report=TIME\n\
              Set the dt for soma reports (using ReportingLib) to TIME (double). The default value is '0.1'.\n\n\
       -z MULTIPLE, --multiple=MULTIPLE\n\
              Model duplication factor. Model size is normal size * MULTIPLE (int). The default value is '1'.\n\
       -x EXTRACON, --extracon=EXTRACON\n\
              Number of extra random connections in each thread to other duplicate models (int). The default value is '0'.\n\
       -mpi\n\
              Enable MPI. In order to initialize MPI environment this argument must be specified.\n" );
}

void cn_parameters::read_cb_opts( int argc, char **argv )
{
    optind = 1;
    int c;

    while ( 1 ) {
        static struct option long_options[] = {
            /* These options don't set a flag.
             *  we distinguish them by their indices. */
            {"tstart",    required_argument, 0, 's'},
            {"tstop",     required_argument, 0, 'e'},
            {"dt",        required_argument, 0, 't'},
            {"dt_io",     required_argument, 0, 'i'},
            {"celsius",   required_argument, 0, 'l'},
            {"voltage",   required_argument, 0, 'v'},
            {"pattern",   required_argument, 0, 'p'},
            {"spikebuf",  required_argument, 0, 'b'},
            {"prcellgid", required_argument, 0, 'g'},
            {"threading", no_argument,       0, 'c'},
            {"gpu",       no_argument,       0, 'a'},
            {"cell_permute",optional_argument,     0, 'R'},
            {"nwarp",     required_argument,     0, 'W'},
            {"datpath",   required_argument, 0, 'd'},
            {"filesdat",  required_argument, 0, 'f'},
            {"outpath",   required_argument, 0, 'o'},
            {"forwardskip", required_argument, 0, 'k'},
            {"multiple", required_argument, 0, 'z'},
            {"extracon", required_argument, 0, 'x'},
            {"mpi",       optional_argument, 0, 'm'},
            {"report",    required_argument, 0, 'r'},
            {"dt_report", required_argument, 0, 'w'},
            {"help",      no_argument,       0, 'h'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long( argc, argv, "s:e:t:i:l:p:b:g:c:d:f:o:k:z:x:m:h:r:w:a:v:R:W",
                         long_options, &option_index );

        /* Detect the end of the options. */
        if ( c == -1 ) {
            break;
        }

        switch ( c ) {
            case 0:

                /* If this option set a flag, do nothing else now. */
                if ( long_options[option_index].flag != 0 ) {
                    break;
                }

                printf( "option %s", long_options[option_index].name );

                if ( optarg ) {
                    printf( " with arg %s", optarg );
                }

                printf( "\n" );
                break;

            case 's':
                tstart = atof( optarg );
                break;

            case 'e':
                tstop = atof( optarg );
                break;

            case 't':
                dt = atof( optarg );
                break;

            case 'i':
                dt_io = atof(optarg );
                break;

            case 'l':
                celsius = atof(optarg);
                break;

            case 'v':
                voltage = atof(optarg);
                break;

            case 'p':
                patternstim = optarg;
                break;

            case 'b':
                spikebuf = atoi( optarg );
                break;

            case 'g':
                prcellgid = atoi( optarg );
                break;

            case 'c':
                threading = 1;
                break;

            case 'a':
                compute_gpu = 1;
                break;

            case 'R':
		if (optarg == NULL) {
			cell_interleave_permute = 1;
		}else{
	                cell_interleave_permute = atoi( optarg );
		}
                break;

            case 'W':
                nwarp = atoi( optarg );
                break;

            case 'd':
                datpath = optarg;
                break;

            case 'f':
                filesdat = optarg;
                break;

            case 'o':
                outpath = optarg;
                break;

            case 'k':
                forwardskip = atof( optarg );
                break;

            case 'z':
                multiple = atoi( optarg );
                break;

            case 'x':
                extracon = atoi( optarg );
                break;

            case 'm':
                /// Reserved for "--mpi", which by this time should be taken care of
                break;

            case 'r':
                report = atoi( optarg );
                break;

            case 'w':
                dt_report = atof( optarg );
                break;

            case 'h':
            case '?':
                if ( nrnmpi_myid == 0 ) {
                    show_cb_opts_help();
                }

                nrn_exit( 0 );

            default:
                printf( "Option %s", long_options[option_index].name );

                if ( optarg ) {
                    printf( " with arg %s", optarg );
                }

                printf( "is not recognized. Ignoring...\n" );
                break;
        }
    }

    /* Print any remaining command line arguments (not options). */
    if ( optind < argc ) {
        printf( "non-option ARGV-elements: " );

        while ( optind < argc ) {
            printf( "%s ", argv[optind++] );
        }

        putchar( '\n' );
    }
}
