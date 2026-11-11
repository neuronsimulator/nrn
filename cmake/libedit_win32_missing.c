/*
 * Definitions the larkmjc/libedit Win32 headers declare and readline.c calls, and this pin does not
 * ship. MSVC has no mkstemp or fseeko. MinGW has those in its C library and has no pwd.h.
 * nrniv already provides dlopen (src/mswin/dlfcn.c); see NRN_LIBEDIT_SKIP_DLFCN.
 * W32_COMPAT_IMPL keeps the shim from turning UCRT's rename into a second w32_rename.
 */
#define W32_COMPAT_IMPL
#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _MSC_VER
#include <fcntl.h>
#include <io.h>
#include <share.h>
#include <sys/stat.h>
#endif

static struct passwd nrn_pw;
static char nrn_pw_name[256];
static char nrn_pw_dir[260];
static char nrn_pw_empty[] = "";
static int nrn_pwent_armed;
static int nrn_pwent_yielded;

static void nrn_fill_passwd(void) {
    DWORD namelen = (DWORD) sizeof(nrn_pw_name);
    if (!GetUserNameA(nrn_pw_name, &namelen) || nrn_pw_name[0] == '\0') {
        strcpy(nrn_pw_name, "user");
    }
    if (GetEnvironmentVariableA("USERPROFILE", nrn_pw_dir, (DWORD) sizeof(nrn_pw_dir)) == 0) {
        char drive[8];
        char path[240];
        if (GetEnvironmentVariableA("HOMEDRIVE", drive, (DWORD) sizeof(drive)) > 0 &&
            GetEnvironmentVariableA("HOMEPATH", path, (DWORD) sizeof(path)) > 0) {
            snprintf(nrn_pw_dir, sizeof(nrn_pw_dir), "%s%s", drive, path);
        } else {
            strcpy(nrn_pw_dir, ".");
        }
    }
    nrn_pw.pw_name = nrn_pw_name;
    nrn_pw.pw_passwd = nrn_pw_empty;
    nrn_pw.pw_gecos = nrn_pw_name;
    nrn_pw.pw_uid = 0;
    nrn_pw.pw_gid = 0;
    nrn_pw.pw_dir = nrn_pw_dir;
    nrn_pw.pw_shell = nrn_pw_empty;
}

uid_t w32_getuid(void) {
    return 0;
}

struct passwd* w32_getpwuid(uid_t uid) {
    (void) uid;
    nrn_fill_passwd();
    return &nrn_pw;
}

struct passwd* w32_getpwnam(const char* username) {
    nrn_fill_passwd();
    if (username != NULL && _stricmp(username, nrn_pw_name) == 0) {
        return &nrn_pw;
    }
    return NULL;
}

void setpwent(void) {
    nrn_pwent_armed = 1;
    nrn_pwent_yielded = 0;
}

struct passwd* getpwent(void) {
    if (!nrn_pwent_armed || nrn_pwent_yielded) {
        return NULL;
    }
    nrn_pwent_yielded = 1;
    nrn_fill_passwd();
    return &nrn_pw;
}

void endpwent(void) {
    nrn_pwent_armed = 0;
}

#ifdef _MSC_VER
int fseeko(FILE* stream, off_t offset, int whence) {
    return _fseeki64(stream, (__int64) offset, whence);
}

off_t ftello(FILE* stream) {
    return (off_t) _ftelli64(stream);
}

int mkstemp(char* tmpl) {
    size_t len;
    char* xs;
    int i;
    if (tmpl == NULL) {
        errno = EINVAL;
        return -1;
    }
    len = strlen(tmpl);
    if (len < 6) {
        errno = EINVAL;
        return -1;
    }
    xs = tmpl + len - 6;
    for (i = 0; i < 6; i++) {
        if (xs[i] != 'X') {
            errno = EINVAL;
            return -1;
        }
    }
    for (i = 0; i < 100; i++) {
        unsigned r = (unsigned) GetTickCount() + (unsigned) i * 1664525u;
        int fd = -1;
        int k;
        errno_t err;
        for (k = 0; k < 6; k++) {
            xs[k] = "abcdefghijklmnopqrstuvwxyz0123456789"[r % 36u];
            r /= 36u;
        }
        err = _sopen_s(
            &fd, tmpl, _O_RDWR | _O_CREAT | _O_EXCL | _O_BINARY, _SH_DENYRW, _S_IREAD | _S_IWRITE);
        if (err == 0) {
            return fd;
        }
        if (err != EEXIST) {
            errno = err;
            return -1;
        }
    }
    errno = EEXIST;
    return -1;
}
#endif
