#include "nrncore_utils.h"
#include "nrncore_write/callbacks/nrncore_callbacks.h"

#include "nrnconf.h"
#include "nrniv_mf.h"
#include <cstdlib>
#include "nrndae_c.h"
#include "section.h"
#include "hocdec.h"
#include "nrnsection_mapping.h"
#include "vrecitem.h"  // for nrnbbcore_vecplay_write
#include "parse.hpp"
#include <string>
#include <unistd.h>
#include <algorithm>
#include <cerrno>
#if __has_include(<filesystem>)
#include <filesystem>
namespace neuron::std {
namespace filesystem = ::std::filesystem;
}
#else
#include <experimental/filesystem>
namespace neuron::std {
namespace filesystem = ::std::experimental::filesystem;
}
#endif

#include "nrnwrap_dlfcn.h"

// RTLD_NODELETE is used with dlopen
// if not defined it's safe to define as 0
#ifndef RTLD_NODELETE
#define RTLD_NODELETE 0
#endif

extern bool corenrn_direct;
extern const char* bbcore_write_version;
extern NrnMappingInfo mapinfo;
extern void (*nrnthread_v_transfer_)(NrnThread*);
extern short* nrn_is_artificial_;


// prerequisites for a NEURON model to be transferred to CoreNEURON.
void model_ready() {
    // Do the model type checks first as some of them prevent the success
    // of cvode.cache_efficient(1) and the error message associated with
    // !use_cachevec would be misleading.
    if (!nrndae_list_is_empty()) {
        hoc_execerror(
            "CoreNEURON cannot simulate a model that contains extra LinearMechanism or RxD "
            "equations",
            NULL);
    }
    if (nrn_threads[0]._ecell_memb_list) {
        hoc_execerror(
            "CoreNEURON cannot simulate a model that contains the extracellular mechanism", NULL);
    }
    if (corenrn_direct) {
        if (cvode_active_) {
            hoc_execerror("CoreNEURON can only use fixed step method.", NULL);
        }
    }

    if (!use_cachevec) {
        hoc_execerror("NEURON model for CoreNEURON requires cvode.cache_efficient(1)", NULL);
    }
    if (tree_changed || v_structure_change || diam_changed) {
        hoc_execerror(
            "NEURON model internal structures for CoreNEURON are out of date. Make sure call to "
            "finitialize(...) is after cvode.cache_efficient(1))",
            NULL);
    }
}

/** @brief Count number of unique elements in the array.
 *  there is a copy of the vector but we are primarily
 *  using it for small section list vectors.
 */
int count_distinct(double* data, int len) {
    if (len == 0)
        return 0;
    std::vector<double> v;
    v.assign(data, data + len);
    std::sort(v.begin(), v.end());
    return std::unique(v.begin(), v.end()) - v.begin();
}


/** @brief For BBP use case, we want to write section-segment
 *  mapping to gid_3.dat file. This information will be
 *  provided through neurodamus HOC interface with following
 *  format:
 *      gid : number of non-empty neurons in the cellgroup
 *      name : name of section list (like soma, axon, apic)
 *      nsec : number of sections
 *      sections : list of sections
 *      segments : list of segments
 *      seg_lfp_factors: list of lfp factors
 */
void nrnbbcore_register_mapping() {
    // gid of a cell
    int gid = *hoc_getarg(1);

    // name of section list
    std::string name = std::string(hoc_gargstr(2));

    // hoc vectors: sections and segments
    Vect* sec = vector_arg(3);
    Vect* seg = vector_arg(4);
    Vect* lfp = vector_arg(5);
    int electrodes_per_segment = *hoc_getarg(6);

    double* sections = vector_vec(sec);
    double* segments = vector_vec(seg);
    double* seg_lfp_factors = vector_vec(lfp);

    int nsec = vector_capacity(sec);
    int nseg = vector_capacity(seg);
    int nlfp = vector_capacity(lfp);

    if (nsec != nseg) {
        std::cout << "Error: Section and Segment mapping vectors should have same size!\n";
        abort();
    }

    // number of unique sections
    nsec = count_distinct(sections, nsec);

    SecMapping* smap = new SecMapping(nsec, name);
    smap->sections.assign(sections, sections + nseg);
    smap->segments.assign(segments, segments + nseg);
    smap->seglfp_factors.assign(seg_lfp_factors, seg_lfp_factors + nlfp);
    smap->num_electrodes = electrodes_per_segment;

    // store mapping information
    mapinfo.add_sec_mapping(gid, smap);
}

// This function is related to stdindex2ptr in CoreNeuron to determine which values should
// be transferred from CoreNeuron. Types correspond to the value to be transferred based on
// mech_type enum or non-artificial cell mechanisms.
// Limited to pointers to voltage, nt._nrn_fast_imem->_nrn_sav_rhs (fast_imem value) or
// data of non-artificial cell mechanisms.
// Requires cache_efficient mode.
// Input double* and NrnThread. Output type and index.
// type == 0 means could not determine index.
extern "C" int nrn_dblpntr2nrncore(double* pd, NrnThread& nt, int& type, int& index) {
    assert(use_cachevec);
    int nnode = nt.end;
    type = 0;
    if (pd >= nt._actual_v && pd < (nt._actual_v + nnode)) {
        type = voltage;  // signifies an index into voltage array portion of _data
        index = pd - nt._actual_v;
    } else if (nt._nrn_fast_imem && pd >= nt._nrn_fast_imem->_nrn_sav_rhs &&
               pd < (nt._nrn_fast_imem->_nrn_sav_rhs + nnode)) {
        type = i_membrane_;  // signifies an index into i_membrane_ array portion of _data
        index = pd - nt._nrn_fast_imem->_nrn_sav_rhs;
    } else {
        for (NrnThreadMembList* tml = nt.tml; tml; tml = tml->next) {
            if (nrn_is_artificial_[tml->index]) {
                continue;
            }
            Memb_list* ml1 = tml->ml;
            int nn = nrn_prop_param_size_[tml->index] * ml1->nodecount;
            if (pd >= ml1->_data[0] && pd < (ml1->_data[0] + nn)) {
                type = tml->index;
                index = pd - ml1->_data[0];
                break;
            }
        }
    }
    return type == 0 ? 1 : 0;
}


#if defined(HAVE_DLFCN_H)

extern int nrn_use_fast_imem;
extern char* neuron_home;

/** Check if coreneuron is loaded into memory */
bool is_coreneuron_loaded() {
    bool is_loaded = false;
    // check if corenrn_embedded_run symbol can be found
    void* handle = dlopen(NULL, RTLD_NOW | RTLD_GLOBAL);
    if (handle) {
        void* fn = dlsym(handle, "corenrn_embedded_run");
        is_loaded = fn == NULL ? false : true;
        dlclose(handle);
    }
    return is_loaded;
}


/** Open library with given path and return dlopen handle **/
void* get_handle_for_lib(neuron::std::filesystem::path const& path) {
    // On windows path.c_str() is wchar_t*
    void* handle = dlopen(path.string().c_str(), RTLD_NOW | RTLD_GLOBAL | RTLD_NODELETE);
    if (!handle) {
        fputs(dlerror(), stderr);
        fputs("\n", stderr);
        hoc_execerror("Could not dlopen CoreNEURON mechanism library : ", path.string().c_str());
    }
    return handle;
}

/** Get CoreNEURON mechanism library */
void* get_coreneuron_handle() {
    // if already loaded into memory, directly return handle
    if (is_coreneuron_loaded()) {
        return dlopen(NULL, RTLD_NOW | RTLD_GLOBAL);
    }

    // record what we tried so we can give a helpful error message
    std::vector<neuron::std::filesystem::path> paths_tried;
    paths_tried.reserve(3);

    // env variable get highest preference
    const char* corenrn_lib = std::getenv("CORENEURONLIB");
    if (corenrn_lib) {
        neuron::std::filesystem::path const corenrn_lib_path{corenrn_lib};
        paths_tried.push_back(corenrn_lib_path);
        if (neuron::std::filesystem::exists(corenrn_lib_path)) {
            return get_handle_for_lib(corenrn_lib_path);
        }
    }

    auto const corenrn_lib_name = std::string{neuron::config::shared_library_prefix}
                                      .append("corenrnmech")
                                      .append(neuron::config::shared_library_suffix);

    // first check if coreneuron specific library exist in <arch>/.libs
    // note that we need to get full path especially for OSX
    {
        auto const corenrn_lib_path = neuron::std::filesystem::current_path() /
                                      neuron::config::system_processor / corenrn_lib_name;
        paths_tried.push_back(corenrn_lib_path);
        if (neuron::std::filesystem::exists(corenrn_lib_path)) {
            return get_handle_for_lib(corenrn_lib_path);
        }
    }

    // last fallback is minimal library with internal mechanisms
    // named libcorenrnmech_internal
    neuron::std::filesystem::path corenrn_lib_path{neuron_home};
    auto const corenrn_internal_lib_name = std::string{neuron::config::shared_library_prefix}
                                               .append("corenrnmech_internal")
                                               .append(neuron::config::shared_library_suffix);
#ifndef MINGW
    (corenrn_lib_path /= "..") /= "..";
#endif
    (corenrn_lib_path /= "lib") /= corenrn_internal_lib_name;
    paths_tried.push_back(corenrn_lib_path);
    if (neuron::std::filesystem::exists(corenrn_lib_path)) {
        return get_handle_for_lib(corenrn_lib_path);
    }
    // Nothing worked => error
    std::ostringstream oss;
    oss << "Could not find CoreNEURON library, tried:";
    for (auto const& path: paths_tried) {
        oss << ' ' << path;
    }
    throw std::runtime_error(oss.str());
}

/** Check if neuron & coreneuron are compatible */
void check_coreneuron_compatibility(void* handle) {
    // get handle to function in coreneuron
    void* cn_version_sym = dlsym(handle, "corenrn_version");
    if (!cn_version_sym) {
        hoc_execerror("Could not get symbol corenrn_version from CoreNEURON", NULL);
    }
    // call coreneuron function and get version string
    const char* cn_bbcore_read_version = (*(const char* (*) ()) cn_version_sym)();

    // make sure neuron and coreneuron version are same; otherwise throw an error
    if (strcmp(bbcore_write_version, cn_bbcore_read_version) != 0) {
        std::stringstream s_path;
        s_path << bbcore_write_version << " vs " << cn_bbcore_read_version;
        hoc_execerror("Incompatible NEURON and CoreNEURON versions :", s_path.str().c_str());
    }

    // Make sure legacy vs modern units are consistent.
    // Would be nice to check in coreneuron set_globals but that would abort
    // if inconsistent.
    void* cn_nrnunit_use_legacy_sym = dlsym(handle, "corenrn_units_use_legacy");
    if (!cn_nrnunit_use_legacy_sym) {
        hoc_execerror("Could not get symbol corenrn_units_use_legacy from CoreNEURON", NULL);
    }
    bool cn_nrnunit_use_legacy = (*(bool (*)()) cn_nrnunit_use_legacy_sym)();
    if (cn_nrnunit_use_legacy != (_nrnunit_use_legacy_ == 1)) {
        hoc_execerror("nrnunit_use_legacy() inconsistent with CORENRN_ENABLE_LEGACY_UNITS", NULL);
    }
}

#endif  //! HAVE_DLFCN_H
