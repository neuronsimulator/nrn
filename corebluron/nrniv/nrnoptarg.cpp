#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "corebluron/nrniv/nrnoptarg.h"

extern "C" void nrn_exit(int);
extern int nrnmpi_myid;

void Get_cb_opts(int argc, char** argv, cb_input_params* input_params)
{

  /// Setting the defaults
  /// Can be changed
  input_params->tstart = 0.;
  input_params->tstop = 100.;
  input_params->dt = 0.025;
  input_params->spikebuf = 100000;
  input_params->patternstim = NULL;
  input_params->prcellgid = -1;
  input_params->threading = 0;
  input_params->filesdat = NULL;
  input_params->datpath = NULL;
  input_params->outpath = NULL;
  input_params->forwardskip = 0.;

  /// Do not currently change
  input_params->celsius = 34.;
  input_params->voltage = -65.;
  input_params->maxdelay = 10.;

  int c;

  while (1)
    {
      static struct option long_options[] =
        {
          /* These options don’t set a flag.
 *              We distinguish them by their indices. */
          {"tstart",    required_argument, 0, 's'},
          {"tstop",     required_argument, 0, 'e'},
          {"dt",        required_argument, 0, 't'},
          {"pattern",   required_argument, 0, 'p'},
          {"spikebuf",  required_argument, 0, 'b'},
          {"prcellgid", required_argument, 0, 'g'},
          {"threading", no_argument,       0, 'c'},
          {"datpath",   required_argument, 0, 'd'},
          {"filesdat",  required_argument, 0, 'f'},
          {"outpath",   required_argument, 0, 'o'},
          {"forwardskip",required_argument, 0,'k'},
          {"help",      no_argument,       0, 'h'},
          {0, 0, 0, 0}
        };
      /* getopt_long stores the option index here. */
      int option_index = 0;

      c = getopt_long (argc, argv, "s:e:t:p:b:g:c:d:f:o:k:h",
                       long_options, &option_index);

      /* Detect the end of the options. */
      if (c == -1)
        break;

      switch (c)
        {
        case 0:
          /* If this option set a flag, do nothing else now. */
          if (long_options[option_index].flag != 0)
            break;
          printf ("option %s", long_options[option_index].name);
          if (optarg)
            printf (" with arg %s", optarg);
          printf ("\n");
          break;

        case 's':
          input_params->tstart = atof(optarg);
          break;

        case 'e':
          input_params->tstop = atof(optarg);
          break;

        case 't':
          input_params->dt = atof(optarg);
          break;

        case 'p':
          input_params->patternstim = optarg;
          break;

        case 'b':
          input_params->spikebuf = atoi(optarg);
          break;

        case 'g':
          input_params->prcellgid = atoi(optarg);
          break;

        case 'c':
          input_params->threading = 1;
          break;

        case 'd':
          input_params->datpath = optarg;
          break;

        case 'f':
          input_params->filesdat = optarg;
          break;

        case 'o':
          input_params->outpath = optarg;
          break;

        case 'k':
          input_params->forwardskip = atof(optarg);
          break;

        case 'h':
        case '?':
          if (nrnmpi_myid == 0)
            printf ("\nWelcome to CoreBluron!\n\nOPTIONS\n\
       -h, -?, --help Print a usage message briefly summarizing these command-line options and the bug-reporting address, then exit.\n\n\
       -s TIME, --tstart=TIME\n\
              Set the start time to TIME (double). The default value is '0.'\n\n\
       -e TIME, --tstop=TIME\n\
              Set the stop time to TIME (double). The default value is '100.'\n\n\
       -t TIME, --dt=TIME\n\
              Set the dt time to TIME (double). The default value is '0.025'.\n\n\
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
       -k NUMBER, --forwardskip=TIME\n\
              Set forwardskip to TIME (double). The default value is '0.'.\n");
          nrn_exit(0);

        default:
          printf ("Option %s", long_options[option_index].name);
          if (optarg)
            printf (" with arg %s", optarg);
          printf ("is not recognized. Ignoring...\n");
          break;
        }
    }

  /* Print any remaining command line arguments (not options). */
  if (optind < argc)
    {
      printf ("non-option ARGV-elements: ");
      while (optind < argc)
        printf ("%s ", argv[optind++]);
      putchar ('\n');
    }
}
