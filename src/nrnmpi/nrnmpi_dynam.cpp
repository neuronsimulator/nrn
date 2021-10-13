#include <../../nrnconf.h>
#include "nrnmpiuse.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#if NRNMPI_DYNAMICLOAD /* to end of file */

#ifdef MINGW
#define RTLD_NOW 0
#define RTLD_GLOBAL 0
#define RTLD_NOLOAD 0
extern "C" {
extern void* dlopen_noerr(const char* name, int mode);
#define dlopen dlopen_noerr
extern void* dlsym(void* handle, const char* name);
extern int dlclose(void* handle);
extern char* dlerror();
}
#else
#include <dlfcn.h>
#endif

#include "nrnmpi.h"

extern char* cxx_char_alloc(size_t);
extern std::string corenrn_mpi_library;

#if DARWIN || defined(__linux__)
extern const char* path_prefix_to_libnrniv();
#endif

#include "mpispike.h"
#include "nrnmpi_def_cinc" /* nrnmpi global variables */
extern "C" {
#include "nrnmpi_dynam_cinc" /* autogenerated file */
}
#include "nrnmpi_dynam_wrappers.inc" /* autogenerated file */
#include "nrnmpi_dynam_stubs.cpp"

static void* load_mpi(const char* name, char* mes) {
	int flag = RTLD_NOW | RTLD_GLOBAL;
	void* handle = dlopen(name, flag);
	if (!handle) {
		sprintf(mes, "load_mpi: %s\n", dlerror());
	}else{
		sprintf(mes, "load_mpi: %s successful\n", name);
	}
	return handle;
}

static void* load_nrnmpi(const char* name, char* mes) {
	int i;
	int flag = RTLD_NOW | RTLD_GLOBAL;
	void* handle = dlopen(name, flag);
	if (!handle) {
		sprintf(mes, "load_nrnmpi: %s\n", dlerror());
		return 0;
	}	
	sprintf(mes, "load_nrnmpi: %s successful\n", name);
	for (i = 0; ftable[i].name; ++i) {
		void* p = dlsym(handle, ftable[i].name);
		if (!p) {
			sprintf(mes+strlen(mes), "load_nrnmpi: %s\n", dlerror());
			return 0;
		}	
		*ftable[i].ppf = p;
	}
	{
		char* (**p)(size_t) = (char* (**)(size_t))dlsym(handle, "p_cxx_char_alloc");
		if (!p) {
			sprintf(mes+strlen(mes), "load_nrnmpi: %s\n", dlerror());
			return 0;
		}
		*p = cxx_char_alloc;
	}
	return handle;
}

char* nrnmpi_load(int is_python) {
	int ismes=0;
	char* pmes;
	void* handle = NULL;
	pmes = static_cast<char*>(malloc(4096));
	assert(pmes);
	pmes[0]='\0';
#if DARWIN
	sprintf(pmes, "Try loading libmpi\n");
	handle = load_mpi("libmpi.dylib", pmes+strlen(pmes));
    /**
     * If libmpi.dylib is not in the standard location and dlopen fails
     * then try to use user provided or ctypes.find_library() provided
     * mpi library path.
     */
    if(!handle) {
        const char* mpi_lib_path = getenv("MPI_LIB_NRN_PATH");
        if (mpi_lib_path) {
            handle = load_mpi(mpi_lib_path, pmes+strlen(pmes));
            if (!handle) {
                sprintf(pmes, "Can not load libmpi.dylib and %s", mpi_lib_path);
            }
        }
    }
	if (handle) {
		/* loaded but is it openmpi or mpich */
		if (dlsym(handle, "ompi_mpi_init")) { /* it is openmpi */
		/* see man dyld */
			if (!load_nrnmpi("@loader_path/libnrnmpi_ompi.dylib", pmes+strlen(pmes))) {
				return pmes;
			}
			corenrn_mpi_library = "@loader_path/libcorenrnmpi_ompi.dylib";
		}else{ /* must be mpich. Could check for MPID_nem_mpich_init...*/
			if (!load_nrnmpi("@loader_path/libnrnmpi_mpich.dylib", pmes+strlen(pmes))) {
				return pmes;
			}
			corenrn_mpi_library = "@loader_path/libcorenrnmpi_mpich.dylib";
		}
	}else{
		ismes = 1;
sprintf(pmes+strlen(pmes), "Is openmpi or mpich installed? If not in default location, "
                           "need a LD_LIBRARY_PATH on Linux or DYLD_LIBRARY_PATH on Mac OS. "
                           "On Mac OS, full path to a MPI library can be provided via "
                           "environmental variable MPI_LIB_NRN_PATH\n");
	}
#else /*not DARWIN*/
#if defined(MINGW)
	sprintf(pmes, "Try loading msmpi\n");
	handle = load_mpi("msmpi.dll", pmes+strlen(pmes));
	if (handle) {
		if (!load_nrnmpi("libnrnmpi_msmpi.dll", pmes+strlen(pmes))){
			return pmes;
		}
		corenrn_mpi_library = std::string(prefix) + "libcorenrnmpi_msmpi.dll";
	}else{
		ismes = 1;
		return pmes;
	}
#else /*not MINGW so must be __linux__*/

	/**
	 * libmpi.so is not standard but used by most of the implemenntation
	 * (mpich, openmpi, intel-mpi, parastation-mpi, hpe-mpt) but not cray-mpich.
	 * we first load libmpi and then libmpich.so as a fallaback for cray system.
	 */
	sprintf(pmes, "Try loading libmpi\n");
	handle = load_mpi("libmpi.so", pmes+strlen(pmes));

    // like osx, check if user has provided library via MPI_LIB_NRN_PATH
    if(!handle) {
        const char* mpi_lib_path = getenv("MPI_LIB_NRN_PATH");
        if (mpi_lib_path) {
            handle = load_mpi(mpi_lib_path, pmes+strlen(pmes));
            if (!handle) {
                sprintf(pmes, "Can not load libmpi.so and %s", mpi_lib_path);
            }
        }
    }

	if (!handle) {
	    sprintf(pmes, "Try loading libmpi and libmpich\n");
	    handle = load_mpi("libmpich.so", pmes+strlen(pmes));
	}

	if (handle) {
		/* with CMAKE the problem of Python launch on LINUX not resolving
		   variables from already loaded shared libraries has returned.
		*/
		if (!dlopen("libnrniv.so", RTLD_NOW | RTLD_NOLOAD | RTLD_GLOBAL)) {
			fprintf(stderr, "Did not promote libnrniv.so to RTLD_GLOBAL: %s\n", dlerror());
		}

		/* safest to use full path for libnrnmpi... */
		const char* prefix = path_prefix_to_libnrniv();
		/* enough space for prefix + "libnrnmpi..." */
		char* lname = static_cast<char*>(malloc(strlen(prefix) + 50));
		assert(lname);
		/* loaded but is it openmpi or mpich */
		if (dlsym(handle, "ompi_mpi_init")) { /* it is openmpi */
			sprintf(lname, "%slibnrnmpi_ompi.so", prefix);
			corenrn_mpi_library = std::string(prefix) + "libcorenrnmpi_ompi.so";
		}else if (dlsym(handle, "MPI_SGI_init")) { /* it is sgi-mpt */
			sprintf(lname, "%slibnrnmpi_mpt.so", prefix);
			corenrn_mpi_library = std::string(prefix) + "libcorenrnmpi_mpt.so";
		}else{ /* must be mpich. Could check for MPID_nem_mpich_init...*/
			sprintf(lname, "%slibnrnmpi_mpich.so", prefix);
			corenrn_mpi_library = std::string(prefix) + "libcorenrnmpi_mpich.so";
		}
		if (!load_nrnmpi(lname, pmes+strlen(pmes))) {
			free(lname);
			return pmes;
		}
		free(lname);
	}else{
		ismes = 1;
sprintf(pmes+strlen(pmes), "Is openmpi, mpich, intel-mpi, sgi-mpt etc. installed? If not in default location, need a LD_LIBRARY_PATH or MPI_LIB_NRN_PATH.\n");
	}
#endif /*not MINGW*/
#endif /* not DARWIN */
	if (!handle) {
		sprintf(pmes+strlen(pmes), "could not dynamically load libmpi.so or libmpich.so\n");
		return pmes;
	}	
	free(pmes);
	return 0;
}
#endif
