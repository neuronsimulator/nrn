#include <stdio.h>
#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include "nrnmusicapi.h"

void (*p_nrnmusic_runtime_phase)();
void (*p_nrnmusic_injectlist)(void*, double);
void (*p_nrnmusic_init)(int* parg, char*** pargv);
void (*p_nrnmusic_terminate)();

#define NRN_LIBDIR "/Users/hines/neuron/nrnmusic/builddynam/install/lib/"

void nrnmusic_load() {
    printf("nrnmusic_load\n");

    void* handle = dlopen(NRN_LIBDIR "libnrnmusic_ompi.dylib", RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        printf("%s\n", dlerror());
    }
    assert(handle);
    p_nrnmusic_runtime_phase = (void (*)()) dlsym(handle, "nrnmusic_runtime_phase");
    assert(p_nrnmusic_runtime_phase);
    p_nrnmusic_injectlist = (void (*)(void*, double)) dlsym(handle, "nrnmusic_injectlist");
    assert(p_nrnmusic_injectlist);
    p_nrnmusic_init = (void (*)(int*, char***)) dlsym(handle, "nrnmusic_init");
    assert(p_nrnmusic_init);
    p_nrnmusic_terminate = (void (*)()) dlsym(handle, "nrnmusic_terminate");
    assert(p_nrnmusic_terminate);
}
