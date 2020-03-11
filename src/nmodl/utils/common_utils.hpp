/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <map>
#include <memory>

/**
 *
 * \dir
 * \brief Utility classes and function
 *
 * \file
 * \brief Common utility functions for file/dir manipulation
 */

namespace nmodl {
/// file/string manipulation functions
namespace utils {

/**
 * @defgroup utils Utility Implementation
 * @brief Utility classes and function implementation
 * @{
 */

/// Check if the iterator is pointing to last element in the container
template <typename Iter, typename Cont>
bool is_last(Iter iter, const Cont& cont) {
    return ((iter != cont.end()) && (next(iter) == cont.end()));
}

/// Given full file path, returns only name of the file
template <class T>
T base_name(T const& path, T const& delims = "/\\") {
    return path.substr(path.find_last_of(delims) + 1);
}

/// Given the file name, returns name of the file without extension
template <class T>
T remove_extension(T const& filename) {
    typename T::size_type const p(filename.find_last_of('.'));
    return p > 0 && p != T::npos ? filename.substr(0, p) : filename;
}

/// Given directory path, create sub-directories
bool make_path(const std::string& path);

/// Check if directory with given path exist
bool is_dir_exist(const std::string& path);

/// Generate random std::string of length len based on a
/// uniform distribution
std::string generate_random_string(int len);

/**
 * \class SingletonRandomString
 * \brief Singleton class for random strings
 *
 * Singleton class for random strings that are appended to the
 * Eigen matrices names that are used in the solutions of
 * nmodl::visitor::SympySolverVisitor and need to be the same to
 * be printed by the nmodl::codegen::CodegenCVisitor
 */
template <unsigned int SIZE = 4>
class SingletonRandomString {
  public:
    /// Delete public constructor needed for the singleton pattern to work
    SingletonRandomString(SingletonRandomString const&) = delete;
    /// Delete public "=" operator
    SingletonRandomString& operator=(SingletonRandomString const&) = delete;

    /// Function to instantiate the SingletonRandomString class
    static std::shared_ptr<SingletonRandomString> instance() {
        static std::shared_ptr<SingletonRandomString> s{new SingletonRandomString};
        return s;
    }

    /** Check if there is a random string assigned as suffix for the var_name variable
     *
     * @param var_name Variable name to check if exists in the std::map of random strings
     * @return true if it exists, false if not
     */
    bool random_string_exists(const std::string& var_name) const {
        return (random_strings.find(var_name) != random_strings.end());
    }

    /** Get the random string of the var_name variable
     *
     * @param var_name Variable name for which to get the random string
     * @return Random string assigned to var_name
     */
    std::string get_random_string(const std::string& var_name) const {
        return random_strings.at(var_name);
    }

    /** If var_name has already got a random string assigned remove it from map
     * and assign a new one, else simply insert a new random string for var_name
     *
     * @param var_name Variable name for which to reset the random string
     * @return Random string assigned to var_name
     */
    std::string reset_random_string(const std::string& var_name) {
        if (random_string_exists(var_name)) {
            random_strings.erase(var_name);
            random_strings.insert({var_name, generate_random_string(SIZE)});
        } else {
            random_strings.insert({var_name, generate_random_string(SIZE)});
        }
        return random_strings[var_name];
    }

  private:
    /// \name Ctor & dtor
    /// \{

    /// Constructor used by instance()
    SingletonRandomString() {}

    /// \}

    /// std::map that keeps the random strings assigned to variables as suffix
    std::map<std::string, std::string> random_strings;
};

/** @} */  // end of utils

}  // namespace utils
}  // namespace nmodl
