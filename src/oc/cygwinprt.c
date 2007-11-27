#include <../../nrnconf.h>
#include <errno.h>
#if defined(CYGWIN)

#if !defined(__MINGW32__)
#include "system.c"
#endif

#include "mswinprt.c"

/* mingw does not have dlfcn.h */
#if !defined(HAVE_DLFCN_H) || defined(__MINGW32__)

void* dlopen(char *name, int mode) {
	void *ret;
	/* handle for the named library */
	ret = (void *) LoadLibrary(name);
        if (ret == NULL) {
		DWORD dw = GetLastError();
		fprintf(stderr, "LoadLibrary(\"%s\") failed with error %d\n", name, dw);
	}
	return ret;
}

void* dlsym(void *handle, char *name) {
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

