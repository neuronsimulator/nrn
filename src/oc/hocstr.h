#ifndef hocstr_h
#define hocstr_h
#include <stdio.h>
/* too many time char* buf overruns its storage */

typedef struct HocStr {
	char* buf;
	size_t size;
}HocStr;

extern HocStr* hoc_tmpbuf; /* highly volatile, copy immediately */
extern HocStr* hocstr_create(size_t);
extern void hocstr_delete(HocStr*);
void hocstr_resize(HocStr*, size_t);
void hocstr_copy(HocStr*, const char*);

#endif

