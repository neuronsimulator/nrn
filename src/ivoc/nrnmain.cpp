#include "nrnconf.h"
#include "nrnmpi.h"
#include "../nrncvode/nrnneosm.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

extern int ivocmain(int, const char**, const char**);
extern int nrn_main_launch;
extern int nrn_noauto_dlopen_nrnmech;
#if NRNMPI_DYNAMICLOAD
void nrnmpi_stubs();
void nrnmpi_load_or_exit();
#if NRN_MUSIC
void nrnmusic_load();
#endif  // NRN_MUSIC
#endif  // NRNMPI_DYNAMICLOAD
#if NRNMPI
extern "C" void nrnmpi_init(int nrnmpi_under_nrncontrol, int* pargc, char*** pargv);
#endif

int main(int argc, char** argv, char** env) {
    nrn_main_launch = 1;

#if defined(AUTO_DLOPEN_NRNMECH) && AUTO_DLOPEN_NRNMECH == 0
    nrn_noauto_dlopen_nrnmech = 1;
#endif

#if NRNMPI
#if NRNMPI_DYNAMICLOAD
    nrnmpi_stubs();
    bool mpi_loaded = false;
    for (int i = 0; i < argc; ++i) {
        if (strcmp("-mpi", argv[i]) == 0) {
            nrnmpi_load_or_exit();
            mpi_loaded = true;
            break;
        }
    }
#if NRN_MUSIC
    // There are three ways that a future call to nrnmusic_init sets
    // nrnmusic = 1. 1) arg0 ends with "music", 2) there is a -music arg,
    // 3) there is a _MUSIC_CONFIG_ environment variable. Use those here to
    // decide whether to call nrnmusic_load() (which exits if it does not
    // succeed.)
    bool load_music = false;
    if (strlen(argv[0]) >= 5 && strcmp(argv[0] + strlen(argv[0]) - 5, "music") == 0) {
        load_music = true;
    }
    for (int i = 0; i < argc; ++i) {
        if (strcmp("-music", argv[i]) == 0) {
            load_music = true;
            break;
        }
    }
    if (getenv("_MUSIC_CONFIG_")) {
        load_music = true;
    }
    if (load_music) {
        if (!mpi_loaded) {
            nrnmpi_load_or_exit();
        }
        nrnmusic_load();
    }
#endif                             // NRNMUSIC
#endif                             // NRNMPI_DYNAMICLOAD
    nrnmpi_init(1, &argc, &argv);  // may change argc and argv
#endif                             // NRNMPI
    errno = 0;
    return ivocmain(argc, (const char**) argv, (const char**) env);
}

#if USENCS
void nrn2ncs_outputevent(int, double) {}
#endif
