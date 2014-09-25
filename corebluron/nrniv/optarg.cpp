#include <stdio.h>
#include <string.h>
#include "corebluron/nrniv/nrnoptarg.h"

bool nrn_optarg_on(const char* opt, int* pargc, char** argv) {
	int i;
	for (i=0; i < *pargc; ++i) {
		if (strcmp(opt, argv[i]) == 0) {
			*pargc -= 1;
			for (; i < *pargc; ++i) {
				argv[i] = argv[i+1];
			}
//			printf("nrn_optarg_on %s  return true\n", opt);
			return true;
		}
	}
	return false;
}

const char* nrn_optarg(const char* opt, int* pargc, char** argv) {
	const char* a;
	int i;
	for (i=0; i < *pargc - 1; ++i) {
		if (strcmp(opt, argv[i]) == 0) {
			a = argv[i+1];
			*pargc -= 2;
			for (; i < *pargc; ++i) {
				argv[i] = argv[i+2];
			}
//			printf("nrn_optarg %s  return %s\n", opt, a);
			return a;
		}
	}
	return NULL;
}

int nrn_optargint(const char* opt, int* pargc, char** argv, int dflt) {
	const char* a;
	int i;
	i = dflt;
	a = nrn_optarg(opt, pargc, argv);
	if (a) {
		sscanf(a, "%d", &i);
	}
//	printf("nrn_optargint %s return %d\n", opt, i);
	return i;
}

