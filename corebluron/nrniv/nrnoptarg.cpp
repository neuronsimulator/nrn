#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "corebluron/nrniv/nrnoptarg.h"
#include "corebluron/utils/sdprintf.h"

extern "C" void nrn_exit( int );
extern int nrnmpi_myid;

cb_parameters::cb_parameters()
{
    tstart = 0.0;
    tstop = 100.0;
    dt = 0.025;

    celsius = 34.0;
    voltage = -65.0;
    maxdelay = 10.0;

    forwardskip = 0;

    spikebuf = 100000;
    prcellgid = -1;

    threading = 0;

    patternstim = NULL;

    datpath = ".";
    outpath = ".";
    filesdat = "files.dat";

    multiple = 1;
    extracon = 0;
}

sd_ptr cb_parameters::get_filesdat_path( char *path_buf, size_t bufsz )
{
    // shouldn't we check if filesdat is absolute or relative? -- sgy 20150119
    return sdprintf( path_buf, bufsz, "%s/%s", datpath, filesdat );
}

void cb_parameters::show_cb_opts()
{
    if ( nrnmpi_myid == 0 ) {
        printf( "\n Configuration Parameters" );

        printf( "\n tstart: %g, tstop: %g, dt: %g", tstart, tstop, dt );
        printf( " celsius: %g, voltage: %g, maxdelay: %g", celsius, voltage, maxdelay );

        printf( "\n forwardskip: %g, spikebuf: %d, prcellgid: %d, multiple: %d, extracon: %d, threading : %d, mindelay : %g", \
                forwardskip, spikebuf, prcellgid, multiple, extracon, threading, mindelay);

        printf( "\n patternstim: %s, datpath: %s, filesdat: %s, outpath: %s", \
                patternstim, datpath, filesdat, outpath );
        if ( prcellgid >= 0 ) {
            printf( "\n prcellstate will be called for gid %d", prcellgid );
        }

        printf( "\n\n" );
    }
}


void cb_parameters::show_cb_opts_help()
{
    printf( "\nWelcome to CoreBluron!\n\nOPTIONS\n\
       -h, -?, --help Print a usage message briefly summarizing these command-line options \
		and the bug-reporting address, then exit.\n\n\
       -s TIME, --tstart=TIME\n\
              Set the start time to TIME (double). The default value is '0.'\n\n\
       -e TIME, --tstop=TIME\n\
              Set the stop time to TIME (double). The default value is '100.'\n\n\
       -t TIME, --dt=TIME\n\
              Set the dt time to TIME (double). The default value is '0.025'.\n\n\
       -l NUMBER, --celsius=NUMBER\n\
              Set the celsius temperature to NUMBER (double). The default value is '34.'.\n\n\
       -p FILE, --pattern=FILE\n\
              Apply patternstim with the spike file FILE (char*). The default value is 'NULL'.\n\n\
       -b SIZE, --spikebuf=SIZE\n\
              Set the spike buffer to be of the size SIZE (int). The default value is '100000'.\n\n\
       -g NUMBER, --prcellgid=NUMBER\n\
              Output prcellstate information for the gid NUMBER (int). The default value is '-1'.\n\n\
       -c, --threading\n\
              Optiong to enable threading. The default implies no threading.\n\n\
       -d PATH, --datpath=PATH\n\
              Set the path with required CoreBluron data to PATH (char*). The default value is '.'.\n\n\
       -f FILE, --filesdat=FILE\n\
              Absolute path with the name for the required file FILE (char*). The default value is 'files.dat'.\n\
       -o PATH, --outpath=PATH\n\
              Set the path for the output data to PATH (char*). The default value is '.'.\n\
       -k TIME, --forwardskip=TIME\n\
              Set forwardskip to TIME (double). The default value is '0.'.\n\
       -z MULTIPLE, --multiple=MULTIPLE\n\
              Model size is normal size * MULTIPLE (int). The default value is '1'.\n\
       -x EXTRACON, --extracon=EXTRACON\n\
              Number of extra random connections in each thread to other duplicate models (int). The default value is '0'.\n\
       -mpi\n\
              Enable MPI. In order to initialize MPI environment this argument must be specified.\n" );
}

void cb_parameters::read_cb_opts( int argc, char **argv )
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
            {"celsius",   required_argument, 0, 'l'},
            {"pattern",   required_argument, 0, 'p'},
            {"spikebuf",  required_argument, 0, 'b'},
            {"prcellgid", required_argument, 0, 'g'},
            {"threading", no_argument,       0, 'c'},
            {"datpath",   required_argument, 0, 'd'},
            {"filesdat",  required_argument, 0, 'f'},
            {"outpath",   required_argument, 0, 'o'},
            {"forwardskip", required_argument, 0, 'k'},
            {"multiple", required_argument, 0, 'z'},
            {"extracon", required_argument, 0, 'x'},
            {"mpi",       optional_argument, 0, 'm'},
            {"help",      no_argument,       0, 'h'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        c = getopt_long( argc, argv, "s:e:t:l:p:b:g:c:d:f:o:k:z:x:m:h",
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

            case 'l':
                celsius = atof(optarg);
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
