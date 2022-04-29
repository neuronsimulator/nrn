/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <map>
#include <memory>
#include <string>
#include <sys/param.h>
#include <unistd.h>
#include <vector>


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

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
/// The character used by the operating system to separate pathname components
static constexpr char pathsep{'\\'};
/// The character conventionally used by the operating system to separate search path components
static constexpr char envpathsep{';'};
/// Maximum size of a directory path
static constexpr int max_path_len{_MAX_DIR};
#else
/// The character used by the operating system to separate pathname components
static constexpr char pathsep{'/'};
/// The character conventionally used by the operating system to separate search path components
static constexpr char envpathsep{':'};
/// Maximum size of a directory path
static constexpr int max_path_len{MAXPATHLEN};
#endif

/// Given directory path, create sub-directories
bool make_path(const std::string& path);

/// Check if directory with given path exists
bool is_dir_exist(const std::string& path);

/// Check if specified file path exists
bool file_exists(const std::string& path);

/// Check if specified file path is absolute
bool file_is_abs(const std::string& path);

/// get current working directory
std::string cwd();

/**
 * \brief Create an empty file which is then removed when the C++ object is destructed
 */
struct TempFile {
    explicit TempFile(std::string path);
    TempFile(std::string path, const std::string& content);
    ~TempFile();

  private:
    std::string path_;
};

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
    const std::string& get_random_string(const std::string& var_name) const {
        return random_strings.at(var_name);
    }

    /** If var_name has already got a random string assigned remove it from map
     * and assign a new one, else simply insert a new random string for var_name
     *
     * @param var_name Variable name for which to reset the random string
     * @param use_numbers control whether random string can include numeric characters or not
     * @return Random string assigned to var_name
     */
    const std::string& reset_random_string(const std::string& var_name,
                                           UseNumbersInString use_numbers) {
        const auto new_string = generate_random_string(SIZE, use_numbers);
        auto status = random_strings.emplace(var_name, new_string);
        if (!status.second) {
            // insertion didn't take place since element was already there.
            status.first->second = new_string;
        }
        return status.first->second;
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
