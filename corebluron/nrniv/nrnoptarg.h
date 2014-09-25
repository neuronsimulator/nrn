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

extern bool nrn_optarg_on(const char* opt, int* argc, char** argv);
extern const char* nrn_optarg(const char* opt, int* argc, char** argv);
extern int nrn_optargint(const char* opt, int* argc, char** argv, int dflt);

#endif

