#include "corenrn_utils.h"
#include "nrnbbcore_write/callbacks/bbcore_callbacks.h"

#include "nrnconf.h"
#include <cstdlib>
#include "nrndae_c.h"
#include "section.h"
#include "hocdec.h"
#include "nrnsection_mapping.h"
#include "vrecitem.h" // for nrnbbcore_vecplay_write
#include "parse.h"
#include <string>
#include <unistd.h>
#include <algorithm>
#include <cerrno>

#if defined(HAVE_DLFCN_H)
#include <dlfcn.h>
#endif

extern "C" {

extern bool corenrn_direct;
extern int diam_changed, v_structure_change, tree_changed;
extern const char *bbcore_write_version;
extern NrnMappingInfo mapinfo;
extern void (*nrnthread_v_transfer_)(NrnThread*);


// prerequisites for a NEURON model to be transferred to CoreNEURON.
void model_ready() {
    // Do the model type checks first as some of them prevent the success
    // of cvode.cache_efficient(1) and the error message associated with
    // !use_cachevec would be misleading.
    if (!nrndae_list_is_empty()) {
        hoc_execerror("CoreNEURON cannot simulate a model that contains extra LinearMechanism or RxD equations", NULL);
    }
    if (nrn_threads[0]._ecell_memb_list) {
        hoc_execerror("CoreNEURON cannot simulate a model that contains the extracellular mechanism", NULL);
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
        hoc_execerror("NEURON model internal structures for CoreNEURON are out of date. Make sure call to finitialize(...) is after cvode.cache_efficient(1))", NULL);
    }
}

/** @brief Count number of unique elements in the array.
 *  there is a copy of the vector but we are primarily
 *  using it for small section list vectors.
 */
int count_distinct(double *data, int len) {
    if( len == 0)
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
 */
void nrnbbcore_register_mapping() {

    // gid of a cell
    int gid = *hoc_getarg(1);

    // name of section list
    std::string name = std::string(hoc_gargstr(2));

    // hoc vectors: sections and segments
    Vect* sec = vector_arg(3);
    Vect* seg = vector_arg(4);

    double* sections  = vector_vec(sec);
    double* segments  = vector_vec(seg);

    int nsec = vector_capacity(sec);
    int nseg = vector_capacity(seg);

    if( nsec != nseg ) {
        std::cout << "Error: Section and Segment mapping vectors should have same size!\n";
        abort();
    }

    // number of unique sections
    nsec = count_distinct(sections, nsec);

    SecMapping *smap = new SecMapping(nsec, name);
    smap->sections.assign(sections, sections+nseg);
    smap->segments.assign(segments, segments+nseg);

    // store mapping information
    mapinfo.add_sec_mapping(gid, smap);
}

/** Check if file with given path exist */
bool file_exist(const std::string& path) {
    std::ifstream f(path.c_str());
    return f.good();
}

#if defined(HAVE_DLFCN_H)

extern int nrn_use_fast_imem;
extern char* neuron_home;

/** Check if coreneuron is loaded into memory */
bool is_coreneuron_loaded() {
    bool is_loaded = false;
    // check if corenrn_embedded_run symbol can be found
    void * handle = dlopen(NULL, RTLD_NOW | RTLD_GLOBAL);
    if (handle) {
        void* fn = dlsym(handle, "corenrn_embedded_run");
        is_loaded = fn == NULL ? false : true;
        dlclose(handle);
    }
    return is_loaded;
}


/** Open library with given path and return dlopen handle **/
void* get_handle_for_lib(const char* path) {
    void* handle = dlopen(path, RTLD_NOW|RTLD_GLOBAL|RTLD_NODELETE);
    if (!handle) {
      fputs(dlerror(), stderr);
      fputs("\n", stderr);
      hoc_execerror("Could not dlopen CoreNEURON mechanism library : ", path);
    }
    return handle;
}

/** Get CoreNEURON mechanism library */
void* get_coreneuron_handle() {
    // if already loaded into memory, directly return handle
    if (is_coreneuron_loaded()) {
        return dlopen(NULL, RTLD_NOW|RTLD_GLOBAL);
    }

    // env variable get highest preference
    const char* corenrn_lib = getenv("CORENEURONLIB");
    if (corenrn_lib && file_exist(corenrn_lib)) {
        return get_handle_for_lib(corenrn_lib);
    }

    // name of coreneuron library based on platform
#if defined(MINGW)
    std::string corenrn_mechlib_name("libcorenrnmech.dll");
#elif defined(DARWIN)
    std::string corenrn_mechlib_name("libcorenrnmech.dylib");
#else
    std::string corenrn_mechlib_name("libcorenrnmech.so");
#endif

    // first check if coreneuron specific library exist in <arch>/.libs
    // note that we need to get full path especially for OSX
    char pwd[FILENAME_MAX];
    if (getcwd(pwd, FILENAME_MAX) == NULL) {
        hoc_execerror("getcwd failed:", strerror(errno));
    }
    std::stringstream s_path;
    s_path << pwd << "/" << NRNHOSTCPU << "/" << corenrn_mechlib_name;
    std::string path = s_path.str();

    if (file_exist(path)) {
        return get_handle_for_lib(path.c_str());
    }

    // last fallback is minimal library with internal mechanisms
    s_path.str("");
#if defined(MINGW)
    s_path << neuron_home << "/lib/" << corenrn_mechlib_name;
#else
    s_path << neuron_home << "/../../lib/" << corenrn_mechlib_name;
#endif
    path = s_path.str();

    // if this last path doesn't exist then it's an error
    if (!file_exist(path)) {
        hoc_execerror("Could not find CoreNEURON library", NULL);
    }

    return get_handle_for_lib(path.c_str());
}

/** Check if neuron & coreneuron are compatible */
void check_coreneuron_compatibility(void* handle) {
    // get handle to function in coreneuron
    void* cn_version_sym = dlsym(handle, "corenrn_version");
    if (!cn_version_sym) {
      hoc_execerror("Could not get symbol corenrn_version from CoreNEURON", NULL);
    }
    // call coreneuron function and get version string
    const char* cn_bbcore_read_version = (*(const char*(*)())cn_version_sym)();

    // make sure neuron and coreneuron version are same; otherwise throw an error
    if (strcmp(bbcore_write_version, cn_bbcore_read_version) != 0) {
      std::stringstream s_path;
      s_path << bbcore_write_version << " vs " << cn_bbcore_read_version;
      hoc_execerror("Incompatible NEURON and CoreNEURON versions :", s_path.str().c_str());
    }
}

#endif //!HAVE_DLFCN_H
}
