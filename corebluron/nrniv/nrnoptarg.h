#ifndef nrnoptarg_h
#define nrnoptarg_h

/*
  Optional arguments of form
  name 
  name string 
  name integer

  nrn_optarg_on(name, ...) returns true if name is in the arg list
    and false otherwise
  nrn_optarg(name, ...) returns the next arg string after the name
     or else NULL.
  nrn_optargint(name, ..., default) returns the next arg (must be
    an int) after the name or else the default
  If the name exists it is removed from the argv list (as well as
  the following arg if relevant) and argc is decremented.
*/

#include <getopt.h>

typedef struct cb_parameters
{
  double tstart, tstop, dt, celsius, voltage, maxdelay;
  int spikebuf, prcellgid, threading;
  char *patternstim, *filesdat, *datpath, *outpath;
} cb_input_params;

/// Get CoreBluron input parameters from the command line
void Get_cb_opts(int argc, char** argv, cb_input_params* input_params);

#endif

