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

typedef struct cb_parameters {

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
    cb_parameters();

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

} cb_input_params;

#endif

