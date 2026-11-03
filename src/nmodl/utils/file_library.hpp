/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <filesystem>
#include <string>
#include <vector>

/**
 *
 * \dir
 * \brief Utility classes and function
 *
 * \file
 * \brief Manage search path.
 */

namespace nmodl {

/**
 * \brief Manage search path.
 *
 * Store search path used for handling paths when processing include NMODL directive
 */
class FileLibrary {
  public:
    /// An empty library
    FileLibrary() = default;
    /**
     *  Initialize the library with the following path:
     *  - current working directory
     *  - paths in the NMODL_PATH environment variable
     */
    static FileLibrary default_instance();

    /**
     * \name Managing inclusion paths.
     * \{
     */
    void append_env_var(const std::string& env_var);
    /** \} */

    /**
     * \name current directory
     * \{
     */
    void push_current_directory(const std::filesystem::path& path);
    void pop_current_directory();
    /** \} */

    /**
     * \brief Search a file.
     * Determine real path of \a file
     * \return Directory containing \a file, or "" if not found.
     */
    std::string find_file(const std::filesystem::path& file);

  private:
    /// push the working directory in the directories stack
    void push_cwd();

    /// inclusion path list
    std::vector<std::filesystem::path> paths_;
};

}  // namespace nmodl
