/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "common_utils.hpp"

#include <cassert>
#include <cerrno>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <sys/stat.h>

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#define IS_WINDOWS
#endif

namespace nmodl {
namespace utils {

bool is_dir_exist(const std::string& path) {
    struct stat info {};
    if (stat(path.c_str(), &info) != 0) {
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;
}

bool file_exists(const std::string& path) {
    struct stat info;
    return stat(path.c_str(), &info) == 0;
}

bool file_is_abs(const std::string& path) {
#ifdef IS_WINDOWS
    return path.find(":\\") != std::string::npos;
#else
    return path.find(pathsep) == 0;
#endif
}

std::string cwd() {
    char cwd[MAXPATHLEN + 1];

    if (nullptr == getcwd(cwd, MAXPATHLEN + 1)) {
        throw std::runtime_error("working directory name too long");
    }
    return {cwd};
}
bool make_path(const std::string& path) {
    mode_t mode = 0755;
    int ret = mkdir(path.c_str(), mode);
    if (ret == 0) {
        return true;
    }

    switch (errno) {
    case ENOENT:
        // parent didn't exist, try to create it
        {
            int pos = path.find_last_of('/');
            if (pos == std::string::npos) {
                return false;
            }
            if (!make_path(path.substr(0, pos))) {
                return false;
            }
        }
        // now, try to create again
        return 0 == mkdir(path.c_str(), mode);

    case EEXIST:
        // done!
        return is_dir_exist(path);

    default:
        auto msg = "Can not create directory " + path;
        throw std::runtime_error(msg);
    }
}

TempFile::TempFile(std::string path)
    : path_(std::move(path)) {
    std::ofstream output(path_);
}

TempFile::TempFile(std::string path, const std::string& content)
    : path_(std::move(path)) {
    std::ofstream output(path_);
    output << content;
}

TempFile::~TempFile() {
    if (remove(path_.c_str()) != 0) {
        perror("Cannot delete temporary file");
    }
}

std::string generate_random_string(const int len, UseNumbersInString use_numbers) {
    std::string s(len, 0);
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> dist(use_numbers ? 0 : 10,
                                                                  sizeof(alphanum) - 2);
    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[dist(rng)];
    }
    return s;
}

}  // namespace utils
}  // namespace nmodl
