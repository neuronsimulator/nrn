#pragma once

extern "C" {

extern int nrnmusic;
// Note: MPI_Comm nrnmusic_comm; in nrnmpi.cpp and nrnmusic.cpp

#if !defined(NRNMUSIC_DYNAMIC) || defined(IN_NRNMUSIC_CPP)
// resolved at link time
extern void nrnmusic_runtime_phase();
extern void nrnmusic_injectlist(void*, double);
extern void nrnmusic_init(int* parg, char*** pargv);
extern void nrnmusic_terminate();

#else  // NRNMUSIC_DYNAMIC

//  resolved at runtime dynamic loading
extern void (*p_nrnmusic_runtime_phase)();
extern void (*p_nrnmusic_injectlist)(void*, double);
extern void (*p_nrnmusic_init)(int* parg, char*** pargv);
extern void (*p_nrnmusic_terminate)();

// But everywhere we use the standard api names
#define nrnmusic_runtime_phase()       \
    if (p_nrnmusic_runtime_phase) {    \
        (*p_nrnmusic_runtime_phase)(); \
    }
#define nrnmusic_injectlist(a, b)       \
    if (p_nrnmusic_injectlist) {        \
        (*p_nrnmusic_injectlist)(a, b); \
    }
#define nrnmusic_init(a, b)       \
    if (p_nrnmusic_init) {        \
        (*p_nrnmusic_init)(a, b); \
    }
#define nrnmusic_terminate()       \
    if (p_nrnmusic_terminate) {    \
        (*p_nrnmusic_terminate)(); \
    }

#endif  // NRNMUSIC_DYNAMIC

}  // end of extern "C"
