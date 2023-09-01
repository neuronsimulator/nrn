/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
/// The character conventionally used by the operating system to separate search path components
static constexpr char envpathsep{';'};
#else
/// The character conventionally used by the operating system to separate search path components
static constexpr char envpathsep{':'};
#endif

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
 * be printed by the nmodl::codegen::CodegenCppVisitor
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
