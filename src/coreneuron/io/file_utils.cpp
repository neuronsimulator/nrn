/*
# =============================================================================
# Copyright (C) 2016-2021 Blue Brain Project
#
# See top-level LICENSE file for details.
# =============================================================================
*/

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>
#include <errno.h>

#if defined(MINGW)
#define mkdir(dir_name, permission) _mkdir(dir_name)
#endif

/* adapted from : gist@jonathonreinhart/mkdir_p.c */
int mkdir_p(const char* path) {
    const int path_len = strlen(path);
    if (path_len == 0) {
        printf("Warning: Empty path for creating directory");
        return -1;
    }

    char* dirpath = new char[path_len + 1];
    strcpy(dirpath, path);
    errno = 0;

    /* iterate from outer upto inner dir */
    for (char* p = dirpath + 1; *p; p++) {
        if (*p == '/') {
            /* temporarily truncate to sub-dir */
            *p = '\0';

            if (mkdir(dirpath, S_IRWXU) != 0) {
                if (errno != EEXIST)
                    return -1;
            }
            *p = '/';
        }
    }

    if (mkdir(dirpath, S_IRWXU) != 0) {
        if (errno != EEXIST) {
            return -1;
        }
    }

    delete[] dirpath;
    return 0;
}
