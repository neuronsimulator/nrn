#include "nrnconf.h"
// A model built using NEURON is heavyweight in memory usage and that
// prevents maximizing the number of cells that can be simulated on
// a process. On the other hand, a tiny version of NEURON that contains
// only the cache efficient structures, minimal memory usage arrays,
// needed to do a simulation (no interpreter, hoc Objects, Section, etc.)
// lacks the model building flexibility of NEURON.
// Ideally, the only arrays needed for a tiny version simulation are those
// enumerated in the NrnThread structure in src/nrnoc/multicore.h up to,
// but not including, the Node** arrays. Also tiny versions of POINT_PROCESS,
// PreSyn, NetCon, and SUFFIX mechanisms will be stripped down from
// their full NEURON definitions and, it seems certain, many of the
// double fields will be converted to some other, less memory using, types.
// With this in mind, we envision that NEURON will incrementally construct
// cache efficient whole cell structures which can be saved and read with
// minimal processing into the tiny simulator. Note that this is a petabyte
// level of data volume. Consider, for example, 128K cores each
// preparing model data for several thousand cells using full NEURON where
// there is not enough space for the simultaneous existence of
// those several thousand cells --- but there is with the tiny version.

// Several assumptions with regard to the nrnbbcore_read reader.
// Since memory is filled with cells, whole cell
// load balance should be adequate and so there is no provision for
// multisplit. A process gets a list of the gid owned by that process
// and allocates the needed
// memory based on size variables for each gid, i.e.
// number of nodes, number of instances of each mechanism type, and number
// of NetCon instances. Also the offsets are calculated for where the
// cell information is to reside in the cache efficient arrays.
// The rest of the cell information is then copied
// into memory with the proper offsets. Pointers to data, used in the old
// NEURON world are converted to integer indices into a common data array.

// A good deal of conceptual confusion resulted in earlier implementations
// with regard to ordering of synapses and
// artificial cells with and without gids. The ordering of the property
// data for those is defined by the order in the NrnThread.tml list where
// every Memb_list.data has an easily found index relative to its 'nodecount'.
// (For artificial cells, since those are not ordered in a cache efficient
// array, we get the index using int nrncore_art2index(double* param)
// which looks up the index in a hash table. Earlier implementations
// handled 'artificial cells without gids' specially which also
// necessitated special handling of their NetCons and disallowed artificial
// cells with gids. We now handle all artificial cells in a thread
// in the same way as any other synapse (the assumption still holds that
// any artificial cell without a gid in a thread can connect only to
// targets in the same thread. Thus, a single NrnThread.synapses now contains
// all synapses and all artificial cells belonging to that thread. All
// the synapses and artificial cells are in NrnThread.tml order. So there
// are no exceptions in filling Point_process pointers from the data indices
// on the coreneuron side. PreSyn ordering is a bit more delicate.
// From netpar.cpp, the gid2out_ hash table defines an output_gid
// ordering and gives us all the PreSyn
// associated with real and artificial cells having gids. But those are
// randomly ordered and interleaved with 'no gid instances'
// relative to the tml ordering.
// Since the number of output PreSyn >= number of output_gid it makes sense
// to order the PreSyn in the same way as defined by the tml ordering.
// Thus, even though artificial cells with and without gids are mixed,
// at least it is convenient to fill the PreSyn.psrc field.
// Synapses are first but the artificial cells with and without gids are
// mixed. The problem that needs to
// be explicitly overcome is associating output gids with the proper PreSyn
// and that can be done with a list parallel to the acell part of the
// output_gid list that specifies the PreSyn list indices.
// Note that allocation of large arrays allows considerable space savings
// by eliminating overhead involved in allocation of many individual
// instances.
/*
Assumptions regarding the scope of possible models.(Incomplete list)
All real cells have gids (possibly multiple, but no more than one gid
for a PreSyn instance.)
Artificial cells without gids connect only to cells in the same thread.
No POINTER to data outside of NrnThread.
No POINTER to data in ARTIFICIAL_CELL (that data is not cache_efficient)
nt->tml->pdata is not cache_efficient
*/
// See coreneuron/nrniv/nrn_setup.cpp for a description of
// the file format written by this file.

/*
Support direct transfer of model to dynamically loaded coreneuron library.
To do this we factored all major file writing components into a series
of functions that return data that can be called from the coreneuron
library. The file writing functionality is kept by also calling those
functions here as well.
Direct transfer mode disables error checking with regard to every thread
having a real cell with a gid. Of course real and artificial cells without
gids do not have spike information in the output raster file. Trajectory
correctness has not been validated for cells without gids.
*/
#include <cstdlib>

#include "section.h"
#include "parse.hpp"
#include "nrnmpi.h"
#include "netcon.h"
#include "nrncvode.h"

#include "vrecitem.h"  // for nrnbbcore_vecplay_write
#include "nrnsection_mapping.h"

#include "nrncore_write.h"
#include "nrncore_write/utils/nrncore_utils.h"
#include "nrncore_write/io/nrncore_io.h"
#include "nrncore_write/callbacks/nrncore_callbacks.h"
#include <map>

#include "nrnwrap_dlfcn.h"


extern NetCvode* net_cvode_instance;


extern int* nrn_prop_dparam_size_;
int* bbcore_dparam_size;  // cvodeieq not present
extern double t;          // see nrncore_psolve

/* not NULL, need to write gap information */
extern void (*nrnthread_v_transfer_)(NrnThread*);
extern size_t nrnbbcore_gap_write(const char* path, int* group_ids);

extern size_t nrncore_netpar_bytes();
extern short* nrn_is_artificial_;

int (*nrnpy_nrncore_enable_value_p_)();
int (*nrnpy_nrncore_file_mode_value_p_)();

char* (*nrnpy_nrncore_arg_p_)(double tstop);

CellGroup* cellgroups_;
/** mapping information */
NrnMappingInfo mapinfo;


// direct transfer or via files? The latter makes use of group_gid for
// filename construction.
bool corenrn_direct;

// name of coreneuron mpi library to load
std::string corenrn_mpi_library;

struct part1_ret {
    std::size_t rankbytes{};
    neuron::model_sorted_token sorted_token;
};

static part1_ret part1();
static void part2(const char*);

/// dump neuron model to given directory path
size_t write_corenrn_model(const std::string& path) {
    // if writing to disk then in-memory mode is false
    corenrn_direct = false;

    // make sure model is ready to transfer
    model_ready();

    // directory to write model
    create_dir_path(path);

    // calculate size of the model
    auto const rankbytes = part1().rankbytes;

    // mechanism and global variables
    write_memb_mech_types(get_filename(path, "bbcore_mech.dat").c_str());
    write_globals(get_filename(path, "globals.dat").c_str());

    // write main model data
    part2(path.c_str());

    return rankbytes;
}

// accessible from ParallelContext.total_bytes()
size_t nrncore_write() {
    const std::string& path = get_write_path();
    return write_corenrn_model(path);
}

static part1_ret part1() {
    // Need the NEURON model to be frozen and sorted in order to transfer it to
    // CoreNEURON
    auto sorted_token = nrn_ensure_model_data_are_sorted();

    size_t rankbytes = 0;
    static int bbcore_dparam_size_size = -1;

    // In nrn/test/pynrn, "python -m pytest ." calls this with
    // n_memb_func of 27 and then with 29. I don't see any explicit
    // intervening h.nrn_load_dll in that folder but ...
    if (bbcore_dparam_size_size != n_memb_func) {
        if (bbcore_dparam_size) {
            delete[] bbcore_dparam_size;
        }
        bbcore_dparam_size = new int[n_memb_func];
    }

    for (int i = 0; i < n_memb_func; ++i) {
        int sz = nrn_prop_dparam_size_[i];
        bbcore_dparam_size[i] = sz;
        Memb_func* mf = memb_func + i;
        if (mf && mf->dparam_semantics && sz && mf->dparam_semantics[sz - 1] == -3) {
            // cvode_ieq in NEURON but not CoreNEURON
            bbcore_dparam_size[i] = sz - 1;
        }
    }
    CellGroup::setup_nrn_has_net_event();
    cellgroups_ = new CellGroup[nrn_nthread];  // here because following needs mlwithart
    auto const cache_token = nrn_ensure_model_data_are_sorted();
    CellGroup::mk_tml_with_art(cache_token, cellgroups_);

    rankbytes += CellGroup::get_mla_rankbytes(cellgroups_);
    rankbytes += nrncore_netpar_bytes();
    // printf("%d bytes %ld\n", nrnmpi_myid, rankbytes);
    CellGroup::mk_cellgroups(cache_token, cellgroups_);
    CellGroup::datumtransform(cellgroups_);
    return {rankbytes, std::move(sorted_token)};
}

static void part2(const char* path) {
    CellGroup* cgs = cellgroups_;
    for (int i = 0; i < nrn_nthread; ++i) {
        chkpnt = 0;
        write_nrnthread(path, nrn_threads[i], cgs[i]);
    }

    /** write mapping information */
    if (mapinfo.size()) {
        int gid = cgs[0].group_id;
        nrn_write_mapping_info(path, gid, mapinfo);
        mapinfo.clear();
    }

    if (nrnthread_v_transfer_) {
        // see partrans.cpp. nrn_nthread files of path/icg_gap.dat
        int* group_ids = new int[nrn_nthread];
        for (int i = 0; i < nrn_nthread; ++i) {
            group_ids[i] = cgs[i].group_id;
        }
        nrnbbcore_gap_write(path, group_ids);
        delete[] group_ids;
    }

    // filename data might have to be collected at hoc level since
    // pc.nrncore_write might be called
    // many times per rank since model may be built as series of submodels.
    if (ifarg(2) && hoc_is_object_arg(2) && is_vector_arg(2)) {
        // Legacy style. Interpreter collects groupgids and writes files.dat
        Vect* cgidvec = vector_arg(2);
        vector_resize(cgidvec, nrn_nthread);
        double* px = vector_vec(cgidvec);
        for (int i = 0; i < nrn_nthread; ++i) {
            px[i] = double(cgs[i].group_id);
        }
    } else {
        bool append = false;
        if (ifarg(2)) {
            if (hoc_is_double_arg(2)) {
                append = (*getarg(2) != 0);
            } else {
                hoc_execerror("Second arg must be Vector or double.", NULL);
            }
        }
        write_nrnthread_task(path, cgs, append);
    }

    part2_clean();
}


#if defined(HAVE_DLFCN_H)
/** Launch CoreNEURON in direct memory mode */
int nrncore_run(const char* arg) {
    // using direct memory mode
    corenrn_direct = true;

    // check that model can be transferred
    model_ready();

    // get coreneuron library handle
    void* handle = [] {
        try {
            return get_coreneuron_handle();
        } catch (std::runtime_error const& e) {
            hoc_execerror(e.what(), nullptr);
        }
    }();

    // make sure coreneuron & neuron are compatible
    check_coreneuron_compatibility(handle);

    // setup the callback functions between neuron & coreneuron
    map_coreneuron_callbacks(handle);

    // lookup symbol from coreneuron for launching
    using launcher_t = int (*)(int, int, int, int, const char*, const char*);
    auto* const coreneuron_launcher = reinterpret_cast<launcher_t>(
        dlsym(handle, "corenrn_embedded_run"));
    if (!coreneuron_launcher) {
        hoc_execerror("Could not get symbol corenrn_embedded_run from", NULL);
    }

    // prepare the model, the returned token will keep the NEURON-side copy of
    // the model frozen until the end of nrncore_run.
    auto sorted_token = part1().sorted_token;

    int have_gap = nrnthread_v_transfer_ ? 1 : 0;
#if !NRNMPI
#define nrnmpi_use 0
#endif

    // launch coreneuron
    int result = coreneuron_launcher(
        nrn_nthread, have_gap, nrnmpi_use, nrn_use_fast_imem, corenrn_mpi_library.c_str(), arg);

    // close handle and return result
    dlclose(handle);

    // Note: possibly non-empty only if nrn_nthread > 1
    CellGroup::clean_deferred_type2artml();

    // Huge memory waste
    CellGroup::clean_deferred_netcons();

    return result;
}

/** Return neuron.coreneuron.enable */
int nrncore_is_enabled() {
    if (nrnpy_nrncore_enable_value_p_) {
        int b = (*nrnpy_nrncore_enable_value_p_)();
        return b;
    }
    return 0;
}

/** Return value of neuron.coreneuron.file_mode flag */
int nrncore_is_file_mode() {
    if (nrnpy_nrncore_file_mode_value_p_) {
        int result = (*nrnpy_nrncore_file_mode_value_p_)();
        return result;
    }
    return 0;
}

/** Run coreneuron with arg string from neuron.coreneuron.nrncore_arg(tstop)
 *  Return 0 on success
 */
int nrncore_psolve(double tstop, int file_mode) {
    if (nrnpy_nrncore_arg_p_) {
        char* arg = (*nrnpy_nrncore_arg_p_)(tstop);
        if (arg) {
            // if file mode is requested then write model to a directory
            // note that CORENRN_DATA_DIR name is also used in module
            // file coreneuron.py
            if (file_mode) {
                const char* CORENRN_DATA_DIR = "corenrn_data";
                write_corenrn_model(CORENRN_DATA_DIR);
            }
            nrncore_run(arg);
            // data return nt._t so copy to t
            t = nrn_threads[0]._t;
            free(arg);
            // Really just want to get NetParEvent back onto queue.
            nrn_spike_exchange_init();
            return 0;
        }
    }
    return -1;
}

#else  // !HAVE_DLFCN_H

int nrncore_run(const char*) {
    return -1;
}

int nrncore_is_enabled() {
    return 0;
}

int nrncore_is_file_mode() {
    return 0;
}

int nrncore_psolve(double tstop, int file_mode) {
    return 0;
}

#endif  //! HAVE_DLFCN_H
