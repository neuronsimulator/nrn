/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <cerrno>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>


namespace nmodl {

bool is_dir_exist(const std::string& path) {
    struct stat info {};
    if (stat(path.c_str(), &info) != 0) {
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;
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

}  // namespace nmodl