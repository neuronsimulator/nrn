/*
Copyright (c) 2014 EPFL-BBP, All rights reserved.

THIS SOFTWARE IS PROVIDED BY THE BLUE BRAIN PROJECT "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE BLUE BRAIN PROJECT
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/**
 * @file main1.cpp
 * @date 26 Oct 2014
 * @brief File containing main driver routine for CoreNeuron
 */

#include "coreneuron/nrnconf.h"
#include "coreneuron/nrnoc/multicore.h"
#include "coreneuron/nrnoc/nrnoc_decl.h"
#include "coreneuron/nrnmpi/nrnmpi.h"
#include "coreneuron/nrniv/nrniv_decl.h"
#include "coreneuron/nrniv/output_spikes.h"
#include "coreneuron/utils/endianness.h"
#include "coreneuron/utils/memory_utils.h"
#include "coreneuron/nrniv/nrnoptarg.h"
#include "coreneuron/utils/randoms/nrnran123.h"
#include "coreneuron/utils/sdprintf.h"
#include "coreneuron/nrniv/nrn_stats.h"

#if 0
#include <fenv.h>
#define NRN_FEEXCEPT (FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW )
int nrn_feenableexcept() {
  int result = -1;
  result = feenableexcept(NRN_FEEXCEPT);
  return result;
}
#endif

int main1( int argc, char **argv, char **env )
{
    char prcellname[1024], filesdat_buf[1024];

    ( void )env; /* unused */

#if defined(NRN_FEEXCEPT)
    nrn_feenableexcept();
#endif

    // mpi initialisation
    nrnmpi_init( 1, &argc, &argv );

    // initialise default coreneuron parameters
    initnrn();

    // create mutex for nrn123, protect instance_count_
    nrnran123_mutconstruct();

    // handles coreneuron configuration parameters
    cn_input_params input_params;

    // read command line parameters
    input_params.read_cb_opts( argc, argv );

    // if multi-threading enabled, make sure mpi library supports it
    if ( input_params.threading ) {
        nrnmpi_check_threading_support();
    }

    // set global variables for start time, timestep and temperature
    t = input_params.tstart;
    dt = input_params.dt;
    celsius = input_params.celsius;

    // full path of files.dat file
    sd_ptr filesdat=input_params.get_filesdat_path(filesdat_buf,sizeof(filesdat_buf));

    // memory footprint after mpi initialisation
    report_mem_usage( "After MPI_Init" );

    // reads mechanism information from bbcore_mech.dat
    mk_mech( input_params.datpath );

    report_mem_usage( "After mk_mech" );

    // create net_cvode instance
    mk_netcvode();

    // One part done before call to nrn_setup. Other part after.
    if ( input_params.patternstim ) {
        nrn_set_extra_thread0_vdata();
    }

    report_mem_usage( "Before nrn_setup" );

    // reading *.dat files and setting up the data structures
    nrn_setup( input_params.datpath, filesdat, nrn_need_byteswap, input_params.threading );

    report_mem_usage( "After nrn_setup " );

    // Invoke PatternStim
    if ( input_params.patternstim ) {
        nrn_mkPatternStim( input_params.patternstim );
    }

    /// Setting the timeout
    nrn_set_timeout(200.);

    // find mindelay and set configuration parameter
    double mindelay = BBS_netpar_mindelay( input_params.maxdelay );

    input_params.set_mindelay( mindelay );

    // show all configuration parameters for current run
    input_params.show_cb_opts();

    // alloctae buffer for mpi communication
    mk_spikevec_buffer( input_params.spikebuf );

    report_mem_usage( "After mk_spikevec_buffer" );

    nrn_finitialize( 1, input_params.voltage );

    report_mem_usage( "After nrn_finitialize" );

    // call prcellstae for prcellgid
    if ( input_params.prcellgid >= 0 ) {
        sprintf( prcellname, "t%g", t );
        prcellstate( input_params.prcellgid, prcellname );
    }

    // handle forwardskip
    if ( input_params.forwardskip > 0.0 ) {
        handle_forward_skip( input_params.forwardskip, input_params.prcellgid );
    }

    /// Solver execution
    BBS_netpar_solve( input_params.tstop );

    // Report global cell statistics
    report_cell_stats();

    // prcellstate after end of solver
    if ( input_params.prcellgid >= 0 ) {
        sprintf( prcellname, "t%g", t );
        prcellstate( input_params.prcellgid, prcellname );
    }

    // write spike information to input_params.outpath
    output_spikes( input_params.outpath );

    // Cleaning the memory
    nrn_cleanup();

    // mpi finalize
    nrnmpi_finalize();

    return 0;
}


/* perform forwardskip and call prcellstate for prcellgid */
void handle_forward_skip( double forwardskip, int prcellgid )
{
    double savedt = dt;
    double savet = t;

    dt = forwardskip * 0.1;
    t = -1e9;

    for ( int step = 0; step < 10; ++step ) {
        nrn_fixed_step_minimal();
    }

    if ( prcellgid >= 0 ) {
        prcellstate( prcellgid, "fs" );
    }

    dt = savedt;
    t = savet;
    dt2thread(-1.);
}


const char *nrn_version( int )
{
    return "version id unimplemented";
}
