#ifndef hocstr_h
#define hocstr_h
#include <stdio.h>
/* too many time char* buf overruns its storage */

typedef struct HocStr {
	char* buf;
	int size;
}HocStr;

extern HocStr* hoc_tmpbuf; /* highly volatile, copy immediately */
extern HocStr* hocstr_create();
extern char* fgets_unlimited(HocStr* s, FILE* f);

#endif

