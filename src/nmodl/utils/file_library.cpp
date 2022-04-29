/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "file_library.hpp"

#include <cassert>
#include <sys/param.h>
#include <unistd.h>

#include "utils/common_utils.hpp"
#include "utils/string_utils.hpp"

namespace nmodl {


FileLibrary FileLibrary::default_instance() {
    FileLibrary library;
    library.append_env_var("NMODL_PATH");
    library.push_cwd();
    return library;
}

void FileLibrary::append_dir(const std::string& path) {
    paths_.insert(paths_.begin(), path);
}

void FileLibrary::append_env_var(const std::string& env_var) {
    const auto value = getenv(env_var.c_str());
    if (value != nullptr) {
        for (const auto& path: stringutils::split_string(value, utils::envpathsep)) {
            if (!path.empty()) {
                append_dir(path);
            }
        }
    }
}

void FileLibrary::push_current_directory(const std::string& path) {
    paths_.push_back(path);
}

void FileLibrary::push_cwd() {
    push_current_directory(utils::cwd());
}

void FileLibrary::pop_current_directory() {
    assert(!paths_.empty());
    paths_.erase(--paths_.end());
}

std::string FileLibrary::find_file(const std::string& file) {
    if (utils::file_is_abs(file)) {
        if (utils::file_exists(file)) {
            return "";
        }
    }
    for (auto paths_it = paths_.rbegin(); paths_it != paths_.rend(); ++paths_it) {
        auto file_abs = *paths_it + utils::pathsep + file;
        if (utils::file_exists(file_abs)) {
            return *paths_it;
        }
    }
    return "";
}

}  // namespace nmodl
