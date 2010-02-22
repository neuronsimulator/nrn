/*
input dos path, output unix path. The output
string may range from the same size as the input string
up to 11 characters longer.
the output string should be freed with free() when no longer needed.
*/

#include <stdlib.h>
#include <assert.h>

char* hoc_dos2unixpath(const char* d) {
	/* translate x: and x:/ and x:\, to /cygdrive/x/ */
	/* and all backslashes to forward slashes */
	char *u;
	char *cp;
	int i, j;
#if 0
   	u = new char[strlen(d) + 12];
#else
	u = (char*)malloc(strlen(d) + 12);
	assert(u);
#endif
	i = j = 0;
	if (d[0] && d[1] == ':') {
		strcpy(u, "/cygdrive/");
		i = strlen(u);
		u[i++] = d[0]; j += 2;
		u[i++] = '/';
		if (d[j] == '/' || d[j] == '\\'){
			j++;
		}
	}
	strcpy(u+i, d+j);
	for (cp = u+i; *cp; ++cp) {
		if (*cp == '\\') {
			*cp = '/';
		}
	}
	return u;
}

