/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

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
    void append_dir(const std::string& path);
    void append_env_var(const std::string& env_var);
    /** \} */

    /**
     * \name current directory
     * \{
     */
    void push_current_directory(const std::string& path);
    void pop_current_directory();
    /** \} */

    /**
     * \brief Search a file.
     * Determine real path of \a file
     * \return Directory containing \a file, or "" if not found.
     */
    std::string find_file(const std::string& file);

  private:
    /// push the working directory in the directories stack
    void push_cwd();

    /// inclusion path list
    std::vector<std::string> paths_;
};

}  // namespace nmodl
