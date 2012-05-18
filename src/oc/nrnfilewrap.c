#include <../../nrnconf.h>

/*
On the BG/P when 128K or more processors are used we need to avoid
huge numbers of replicated reads of hoc files. The NrnFileWrap
provides a framework for this. In normal circumstances corresponding
fopen, fclose , getc, fall through to the FILE functions.
Otherwise, the file will be read by rank 0 into a char buffer and
broadcast to all the other ranks.
*/

#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "hoc.h"
#include "nrnmpi.h"
#include "nrnfilewrap.h"

extern int nrn_xopen_broadcast_;

#if USE_NRNFILEWRAP /* to end of file */

static NrnFILEWrap* fwstdin;
static NrnFILEWrap* nrn_fw_read(const char* path, const char* mode);

NrnFILEWrap* nrn_fw_wrap(FILE* f) {
	NrnFILEWrap* fw;
	fw = (NrnFILEWrap*)emalloc(sizeof(NrnFILEWrap));
	fw->f = f;
	fw->buf = (char*)0;
	fw->ip = 0;
	fw->cnt = 0;
	return fw;
}

void nrn_fw_delete(NrnFILEWrap* fw){
	if (fw->buf) {
		free(fw->buf);
	}
	free(fw);
}

int nrn_fw_fclose(NrnFILEWrap* fw){
	int e = 0;
	if (fw->f) {
		e = fclose(fw->f);
	}
	if (fw->buf) {
		nrn_fw_delete(fw);
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
	NrnFILEWrap* fw = (NrnFILEWrap*)0;
	FILE* f;
	if (nrn_xopen_broadcast_ == 0 || nrn_xopen_broadcast_ > nrnmpi_numprocs) {
		f = fopen(path, mode);
		if (f) {
			fw = nrn_fw_wrap(f);
		}
	}else{
		fw = nrn_fw_read(path, mode);
	}
	return fw;
}

int nrn_fw_readaccess(const char* path) {
	int r;
	if (nrn_xopen_broadcast_ == 0 || nrn_xopen_broadcast_ > nrnmpi_numprocs) {
		r = access(path, R_OK);
	} else {
		if ( nrnmpi_myid == 0) {
			r = access(path, R_OK);
#if 0
			printf("access %s %d\n", path, r);
#endif
		}
		nrnmpi_int_broadcast(&r, 1, 0);
	}
	r = ((r == 0) ? 1 : 0);
	return r;
}

int nrn_fw_fseek(NrnFILEWrap* fw, long offset, int whence){
	int r = 0;
	int e = 0;
	if (fw->f) {
		r = fseek(fw->f, offset, whence);
	}else if (fw->buf) {
		long i = -1;
		switch(whence) {
		case SEEK_SET:
			i = offset; break;
		case SEEK_CUR:
			i = offset + fw->ip; break;
		case SEEK_END:
			i = offset + fw->cnt; break;
		}
		if (i >= 0 && i <= fw->cnt) {
			fw->ip = i;
		}else{
			errno = EINVAL;
			r = -1;
		}
	}else{
		errno = EBADF;
		r = -1;
	}
	return r;
}

int nrn_fw_getc(NrnFILEWrap* fw){
	int c = EOF;
	if (fw->f) {
		c = getc(fw->f);
	}else if (fw->buf) {
		if (fw->ip < fw->cnt) {
			c = (int)fw->buf[fw->ip++];
		}
	}
	return c;
}

int nrn_fw_ungetc(int c, NrnFILEWrap* fw){
	int e = EOF;
	if (fw->f) {
		e = ungetc(c, fw->f);
	}else if (fw->buf) {
		if (fw->ip > 0) {
			fw->buf[--fw->ip] = (unsigned char)c;
			e = c;
		}
	}
	return e;
}

static int nrn_vsscanf(const char *str, const char** rs, const char *format, va_list args);

int nrn_fw_fscanf(NrnFILEWrap* fw, const char* fmt, ...){
	int i = 0;
	va_list ap;
	va_start(ap, fmt);
	if (fw->f) {
		i = vfscanf(fw->f, fmt, ap);
	}else if (fw->buf) {
		const char* rs;
		i = nrn_vsscanf(fw->buf + fw->ip, &rs, fmt, ap);
		fw->ip = (unsigned char*)rs - fw->buf;
	}
	va_end(ap);
	return i;
}

NrnFILEWrap* nrn_fw_read(const char* path, const char* mode){
	NrnFILEWrap* fw = (NrnFILEWrap*)0;
	size_t sz = 0;
	int isz = -1;
	unsigned char* buf = (char*)0;
	if (nrnmpi_myid == 0) {
		FILE* f = fopen(path, mode);
		if (f) {
			struct stat bs;
			assert(!fstat(fileno(f), &bs));
			sz = bs.st_size;
			isz = sz;
			if (sz > 0) {
				buf = (unsigned char*)emalloc(sz+2);
				assert(fread(buf, 1, sz, f) == sz);
			}
			fclose(f);
#if 1
			printf("load %s %d\n", path, isz);
#endif
		}
	}
	if (nrnmpi_numprocs > 1) {
		nrnmpi_int_broadcast(&isz, 1, 0);
		if (nrnmpi_myid > 0) {
			if (isz > 0) {
				sz = isz;
				buf = (unsigned char*)emalloc(sz+2);
			}
		}
		if (isz > 0) {
			nrnmpi_char_broadcast((char*)buf, isz, 0);
		}
	}
	if (isz >= 0) {
		fw = nrn_fw_wrap((FILE*)0);
		fw->cnt = sz;
		buf[sz] = '\0';
		fw->buf = (unsigned char*)buf;
	}
	return fw;
}

#include "nrn_vsscanf.c"

#endif /* USE_NRNFILEWRAP */
