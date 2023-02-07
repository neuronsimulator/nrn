#pragma once

extern int nrnmusic;

extern "C" {

// Note: MPI_Comm nrnmusic_comm; in nrnmpi.cpp and nrnmusic.cpp

extern void nrnmusic_runtime_phase();
extern void nrnmusic_injectlist(void*, double);
extern void nrnmusic_init(int* parg, char*** pargv);
extern void nrnmusic_terminate();

}  // end of extern "C"