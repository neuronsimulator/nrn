#include <stdio.h>
#include "nrnmusicapi.h"

void (*p_nrnmusic_runtime_phase)();
void (*p_nrnmusic_injectlist)(void*, double);
void (*p_nrnmusic_init)(int* parg, char*** pargv);
void (*p_nrnmusic_terminate)();

void nrnmusic_load() {
    printf("nrnmusic_load\n");
}
