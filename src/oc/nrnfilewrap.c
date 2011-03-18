#include <../../nrnconf.h>

/*
On the BG/P when more that 64K processors are used we need to avoid
huge numbers of replicated reads of hoc files. The NrnFileWrap
provides a framework for this. In normal circumstances corresponding
fopen, fclose , getc, fall through to the FILE functions.
Otherwise, the file will be read by rank 0 into a char buffer and
broadcast to all the other ranks.
*/

#include <stdarg.h>
#include "hoc.h"
#include "nrnfilewrap.h"

#if USE_NRNFILEWRAP /* to end of file */

static NrnFILEWrap* fwstdin;

NrnFILEWrap* nrn_fw_wrap(FILE* f) {
	NrnFILEWrap* fw;
	fw = (NrnFILEWrap*)emalloc(sizeof(NrnFILEWrap));
	fw->f = f;
	fw->mf = (void*)0;
	return fw;
}

void nrn_fw_delete(NrnFILEWrap* fw){
	free(fw);
}

int nrn_fw_fclose(NrnFILEWrap* fw){
	int e = 0;
	if (fw->f) {
		e = fclose(fw->f);
	}
	return e;
}

NrnFILEWrap* nrn_fw_set_stdin(){
	if (!fwstdin) {
		fwstdin = nrn_fw_wrap(stdin);
	}
	return fwstdin;
}

NrnFILEWrap* nrn_fw_fopen(const char* path, const char* mode){
	NrnFILEWrap* fw;
	FILE* f;
	f = fopen(path, mode);
	fw = nrn_fw_wrap(f);
	return fw;
}

int nrn_fw_fseek(NrnFILEWrap* fw, long offset, int whence){
	int r = 0;
	if (fw->f) {
		r = fseek(fw->f, offset, whence);
	}
	return r;
}

int nrn_fw_getc(NrnFILEWrap* fw){
	int c = 0;
	if (fw->f) {
		c = getc(fw->f);
	}
	return c;
}

int nrn_fw_ungetc(int c, NrnFILEWrap* fw){
	int e = EOF;
	if (fw->f) {
		e = ungetc(c, fw->f);
	}
	return e;
}

int nrn_fw_fscanf(NrnFILEWrap* fw, const char* fmt, ...){
	int i = 0;
	va_list ap;
	va_start(ap, fmt);
	if (fw->f) {
		i = vfscanf(fw->f, fmt, ap);
	}
	va_end(ap);
	return i;
}

#endif /* USE_NRNFILEWRAP */
