/*
input dos path, output unix (or mingw) path. The output
string may range from the same size as the input string
up to 11 characters longer.
the output string should be freed with free() when no longer needed.
*/

#include <stdlib.h>
#include <string.h>
#include <assert.h>

char* hoc_dos2unixpath(const char* d) {
    /* translate x: and x:/ and x:\, to /cygdrive/x/ */
    /* and all backslashes to forward slashes */
    /* or, for mingw, just backslashes to forward slashes */
    char* cp;
    auto* const u = static_cast<char*>(malloc(strlen(d) + 12));
    assert(u);
    int i{}, j{};
    if (d[0] && d[1] == ':') {
        strcpy(u, "/cygdrive/");
        i = strlen(u);
        u[i++] = d[0];
        j += 2;
        u[i++] = '/';
        if (d[j] == '/' || d[j] == '\\') {
            j++;
        }
    }
    strcpy(u + i, d + j);
    for (cp = u + i; *cp; ++cp) {
        if (*cp == '\\') {
            *cp = '/';
        }
    }
    return u;
}
