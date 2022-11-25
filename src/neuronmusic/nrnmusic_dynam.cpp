#include <../../nrnconf.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include "oc_ansi.h"
#include "nrnmusicapi.h"
#include <array>
#include <cstdlib>

#include "nrnwrap_dlfcn.h"

void (*p_nrnmusic_runtime_phase)();
void (*p_nrnmusic_injectlist)(void*, double);
void (*p_nrnmusic_init)(int* parg, char*** pargv);
void (*p_nrnmusic_terminate)();

#if DARWIN || defined(__linux__)
extern const char* path_prefix_to_libnrniv();
#else
#error MUSIC dynamic loading not implemented for this architecture.
#endif

static void load_music() {
    void* handle = nullptr;
    using const_char_ptr = const char*;
    // would be nice to figure out path from `which music` but not on
    // every rank if nhost is large. So demand ...
    const_char_ptr music_path = const_char_ptr{std::getenv("NRN_LIBMUSIC_PATH")};
    if (!music_path) {
        Fprintf(stderr, "No NRN_LIBMUSIC_PATH environment variable for full path to libmusic\n");
        exit(1);
    }
    handle = dlopen(music_path, RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        Fprintf(stderr, "%s", dlerror());
        exit(1);
    }
}

void nrnmusic_load() {
    static bool is_loaded{false};
    if (is_loaded) {
        return;
    }

    load_music();

    // use nrnmpi/nrnmpi_dynam.cpp strategies to figure out paths.
    auto const nrnlib_prefix = []() -> std::string {
        if (const char* nrn_home = std::getenv("NRNHOME")) {
            // TODO: what about windows path separators?
            return std::string{nrn_home} + "/lib/";
        } else {
            // Use the directory libnrniv.so is in
            return path_prefix_to_libnrniv();
        }
    }();

    auto const nrnlib_path = [&](std::string_view middle) {
        std::string name{nrnlib_prefix};
        name.append(neuron::config::shared_library_prefix);
        name.append(middle);
        name.append(neuron::config::shared_library_suffix);
        return name;
    };
    auto const nrnmusic_library = nrnlib_path("nrnmusic");

    void* handle = dlopen(nrnmusic_library.c_str(), RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        Fprintf(stderr, "%s\n", dlerror());
        exit(1);
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
    is_loaded = true;
}
