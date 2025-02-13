/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "pybind/pyembed.hpp"

#include <cstdlib>
#include <dlfcn.h>
#include <filesystem>
#include <pybind11/embed.h>


#include "config/config.h"
#include "utils/logger.hpp"

#define STRINGIFY(x) #x
#define TOSTRING(x)  STRINGIFY(x)

namespace fs = std::filesystem;

namespace nmodl {

namespace pybind_wrappers {


using nmodl_init_pybind_wrapper_api_fpointer = decltype(&nmodl_init_pybind_wrapper_api);

bool EmbeddedPythonLoader::have_wrappers() {
#if defined(NMODL_STATIC_PYWRAPPER)
    auto* init = &nmodl_init_pybind_wrapper_api;
#else
    auto* init = (nmodl_init_pybind_wrapper_api_fpointer) (dlsym(RTLD_DEFAULT,
                                                                 "nmodl_init_pybind_wrapper_api"));
#endif

    if (init != nullptr) {
        wrappers = init();
    }

    return init != nullptr;
}

void assert_compatible_python_versions() {
    // This code is imported and slightly modified from PyBind11 because this
    // is primarly in details for internal usage
    // License of PyBind11 is BSD-style

    std::string compiled_ver = fmt::format("{}.{}", PY_MAJOR_VERSION, PY_MINOR_VERSION);
    auto pPy_GetVersion = (const char* (*) (void) ) dlsym(RTLD_DEFAULT, "Py_GetVersion");
    if (pPy_GetVersion == nullptr) {
        throw std::runtime_error("Unable to find the function `Py_GetVersion`");
    }
    const char* runtime_ver = pPy_GetVersion();
    std::size_t len = compiled_ver.size();
    if (std::strncmp(runtime_ver, compiled_ver.c_str(), len) != 0 ||
        (runtime_ver[len] >= '0' && runtime_ver[len] <= '9')) {
        throw std::runtime_error(
            fmt::format("Python version mismatch. nmodl has been compiled with python {} and is "
                        "being run with python {}",
                        compiled_ver,
                        runtime_ver));
    }
}

void EmbeddedPythonLoader::load_libraries() {
    const auto pylib_env = std::getenv("NMODL_PYLIB");
    if (!pylib_env) {
        logger->critical("NMODL_PYLIB environment variable must be set to load embedded python");
        throw std::runtime_error("NMODL_PYLIB not set");
    }
    const auto dlopen_opts = RTLD_NOW | RTLD_GLOBAL;
    dlerror();  // reset old error conditions
    pylib_handle = dlopen(pylib_env, dlopen_opts);
    if (!pylib_handle) {
        const auto errstr = dlerror();
        logger->critical("Tried but failed to load {}", pylib_env);
        logger->critical(errstr);
        throw std::runtime_error("Failed to dlopen");
    }

    assert_compatible_python_versions();

    if (std::getenv("NMODLHOME") == nullptr) {
        logger->critical("NMODLHOME environment variable must be set to load embedded python");
        throw std::runtime_error("NMODLHOME not set");
    }
    auto pybind_wraplib_env = fs::path(std::getenv("NMODLHOME")) / "lib" / "libpywrapper";
    pybind_wraplib_env.concat(CMakeInfo::SHARED_LIBRARY_SUFFIX);
    if (!fs::exists(pybind_wraplib_env)) {
        logger->critical("NMODLHOME doesn't contain libpywrapper{} library",
                         CMakeInfo::SHARED_LIBRARY_SUFFIX);
        throw std::runtime_error("NMODLHOME doesn't have lib/libpywrapper library");
    }
    std::string env_str = pybind_wraplib_env.string();
    pybind_wrapper_handle = dlopen(env_str.c_str(), dlopen_opts);
    if (!pybind_wrapper_handle) {
        const auto errstr = dlerror();
        logger->critical("Tried but failed to load {}", pybind_wraplib_env.string());
        logger->critical(errstr);
        throw std::runtime_error("Failed to dlopen");
    }
}

void EmbeddedPythonLoader::populate_symbols() {
#if defined(NMODL_STATIC_PYWRAPPER)
    auto* init = &nmodl_init_pybind_wrapper_api;
#else
    // By now it's been dynamically loaded with `RTLD_GLOBAL`.
    auto* init = (nmodl_init_pybind_wrapper_api_fpointer) (dlsym(RTLD_DEFAULT,
                                                                 "nmodl_init_pybind_wrapper_api"));
#endif

    if (!init) {
        const auto errstr = dlerror();
        logger->critical("Tried but failed to load pybind wrapper symbols");
        logger->critical(errstr);
        throw std::runtime_error("Failed to dlsym");
    }

    wrappers = init();
}

void EmbeddedPythonLoader::unload() {
    if (pybind_wrapper_handle) {
        dlclose(pybind_wrapper_handle);
        pybind_wrapper_handle = nullptr;
    }
    if (pylib_handle) {
        dlclose(pylib_handle);
        pylib_handle = nullptr;
    }
}

const pybind_wrap_api& EmbeddedPythonLoader::api() {
    return wrappers;
}


}  // namespace pybind_wrappers

}  // namespace nmodl
