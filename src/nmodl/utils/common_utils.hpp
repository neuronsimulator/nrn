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

/**
 * Return non-const iterator corresponding to the const_iterator in a vector
 *
 * Some old compilers like GCC v4.8.2  has C++11 support but missing erase and insert
 * with const_iterator implementation. This is a workaround to handle build issues with
 * such compilers especially on manylnux1 platform.
 *
 * See bug report : https://gcc.gnu.org/bugzilla/show_bug.cgi?id=57158
 *
 * \todo Remove this after move to manylinux2010 platform.
 */
#if (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) < 40900
template <typename T>
typename std::vector<T>::iterator const_iter_cast(std::vector<T>& v,
                                                  typename std::vector<T>::const_iterator iter) {
    return v.begin() + (iter - v.cbegin());
}
#else
template <typename T>
typename std::vector<T>::const_iterator const_iter_cast(
    const std::vector<T>& /*v*/,
    typename std::vector<T>::const_iterator iter) {
    return iter;
}
#endif


/// Given directory path, create sub-directories
bool make_path(const std::string& path);

/// Check if directory with given path exist
bool is_dir_exist(const std::string& path);

/// Enum to wrap bool variable to select if random string
/// should have numbers or not
enum UseNumbersInString : bool { WithNumbers = true, WithoutNumbers = false };

/// Generate random std::string of length len based on a
/// uniform distribution
std::string generate_random_string(int len, UseNumbersInString use_numbers);

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
    static SingletonRandomString& instance() {
        static SingletonRandomString srs;
        return srs;
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
     * @param use_numbers control whether random string can include numeric characters or not
     * @return Random string assigned to var_name
     */
    std::string reset_random_string(const std::string& var_name, UseNumbersInString use_numbers) {
        if (random_string_exists(var_name)) {
            random_strings.erase(var_name);
            random_strings.insert({var_name, generate_random_string(SIZE, use_numbers)});
        } else {
            random_strings.insert({var_name, generate_random_string(SIZE, use_numbers)});
        }
        return random_strings[var_name];
    }

  private:
    /// \name Ctor & dtor
    /// \{

    /// Constructor used by instance()
    SingletonRandomString() = default;

    /// \}

    /// std::map that keeps the random strings assigned to variables as suffix
    std::map<std::string, std::string> random_strings;
};

/** @} */  // end of utils

}  // namespace utils
}  // namespace nmodl
