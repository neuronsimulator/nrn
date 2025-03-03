/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "wrapper.hpp"

namespace nmodl {
namespace pybind_wrappers {

/**
 * A singleton class handling access to the pybind_wrap_api struct
 *
 * This class manages the runtime loading of the libpython so/dylib file and the python binding
 * wrapper library and provides access to the API wrapper struct that can be used to access the
 * pybind11 embedded python functionality.
 */
class EmbeddedPythonLoader {
  public:
    /**
     * Construct (if not already done) and get the only instance of this class
     *
     * @return the EmbeddedPythonLoader singleton instance
     */
    static EmbeddedPythonLoader& get_instance() {
        static EmbeddedPythonLoader instance;

        return instance;
    }

    EmbeddedPythonLoader(const EmbeddedPythonLoader&) = delete;
    void operator=(const EmbeddedPythonLoader&) = delete;


    /**
     * Get a pointer to the pybind_wrap_api struct
     *
     * Get access to the container struct for the pointers to the functions in the wrapper library.
     * @return a pybind_wrap_api pointer
     */
    const pybind_wrap_api& api();

    ~EmbeddedPythonLoader() {
        unload();
    }

  private:
    pybind_wrap_api wrappers;

    void* pylib_handle = nullptr;
    void* pybind_wrapper_handle = nullptr;

    bool have_wrappers();
    void load_libraries();
    void populate_symbols();
    void unload();

    EmbeddedPythonLoader() {
        if (!have_wrappers()) {
            load_libraries();
            populate_symbols();
        }
    }
};


}  // namespace pybind_wrappers
}  // namespace nmodl
