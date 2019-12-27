#include <../../nrnconf.h>
#include <errno.h>
#include <unistd.h>
#if defined(CYGWIN) || defined(MINGW)

#if !defined(__MINGW32__)
#include "system.c"
#define my_off64_t loff_t
#else
#define my_off64_t off64_t
#endif

#include "mswinprt.c"

my_off64_t lseek64(int fd, my_off64_t offset, int whence) {
	fprintf(stderr, "called lseek64\n");
	abort();
}

/* mingw does not have dlfcn.h */
#if !defined(HAVE_DLFCN_H) || defined(__MINGW32__)

void* dlopen(const char *name, int mode) {
	void *ret;
	/* handle for the named library */
	ret = (void *) LoadLibrary(name);
        if (ret == NULL) {
		DWORD dw = GetLastError();
		fprintf(stderr, "LoadLibrary(\"%s\") failed with error %d\n", name, dw);
	}
	return ret;
}

void* dlopen_noerr(const char *name, int mode) {return (void*)LoadLibrary(name);}

void* dlsym(void *handle, const char *name) {
	void *ret = (void *) GetProcAddress ((HMODULE) handle, name);
	if (!ret) {
		fprintf(stderr, "Could not GetProcAddress for \"%s\"\n", name);
	}
	return ret;
}

int dlclose(void* handle) {
}

static char* dler_="";
char* dlerror() {
	return dler_;
}
#endif /* HAVE_DLFCN_H */

#endif

