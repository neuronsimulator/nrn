/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <cstdlib>
#include <dlfcn.h>

#include "pybind/pyembed.hpp"
#include "utils/logger.hpp"

namespace nmodl {

namespace pybind_wrappers {

bool EmbeddedPythonLoader::have_wrappers() {
#if defined(NMODL_STATIC_PYWRAPPER)
    static auto wrapper_api = nmodl::pybind_wrappers::init_pybind_wrap_api();
    wrappers = &wrapper_api;
    return true;
#else
    wrappers = static_cast<pybind_wrap_api*>(dlsym(RTLD_DEFAULT, "nmodl_wrapper_api"));
    return wrappers != nullptr;
#endif
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
    const auto pybind_wraplib_env = std::getenv("NMODL_WRAPLIB");
    if (!pybind_wraplib_env) {
        logger->critical(
            "NMODL_WRAPLIB environment variable must be set to load the pybind wrapper library");
        throw std::runtime_error("NMODL_WRAPLIB not set");
    }
    pybind_wrapper_handle = dlopen(pybind_wraplib_env, dlopen_opts);
    if (!pybind_wrapper_handle) {
        const auto errstr = dlerror();
        logger->critical("Tried but failed to load {}", pybind_wraplib_env);
        logger->critical(errstr);
        throw std::runtime_error("Failed to dlopen");
    }
}

void EmbeddedPythonLoader::populate_symbols() {
    wrappers = static_cast<pybind_wrap_api*>(dlsym(pybind_wrapper_handle, "nmodl_wrapper_api"));
    if (!wrappers) {
        const auto errstr = dlerror();
        logger->critical("Tried but failed to load pybind wrapper symbols");
        logger->critical(errstr);
        throw std::runtime_error("Failed to dlsym");
    }
}

void EmbeddedPythonLoader::unload() {
    if (pybind_wrapper_handle) {
        dlclose(pybind_wrapper_handle);
    }
    if (pylib_handle) {
        dlclose(pylib_handle);
    }
}

const pybind_wrap_api* EmbeddedPythonLoader::api() {
    return wrappers;
}


}  // namespace pybind_wrappers

}  // namespace nmodl
