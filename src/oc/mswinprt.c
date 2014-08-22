#ifndef CYGWIN
#include <../../nrnconf.h>
#endif

#if defined(MINGW)
#define CYGWIN
#endif

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "hoc.h"
#include "../mswin/extra/d2upath.c"

extern char* neuron_home;
extern char* neuron_home_dos;

#if !defined(CYGWIN)
extern HWND hCurrWnd;
#endif
static HCURSOR wait_cursor;
static HCURSOR old_cursor;
#if HAVE_IV
extern int bad_install_ok;
#else
int bad_install_ok;
#endif // HAVE_IV
extern FILE* hoc_redir_stdout;
setneuronhome(p) char* p; {
	// if the program lives in .../bin/neuron.exe
	// and .../lib exists then use ... as the
	// NEURONHOME
	char buf[256]; char *s;
	int i, j;
//	printf("p=|%s|\n", p);
    bad_install_ok = 1;
#if 0
	if (p[0] == '"') {
   	strcpy(buf, p+1);
   }else{
		strcpy(buf, p);
   }
#endif
	GetModuleFileName(NULL, buf, 256);
	for (i=strlen(buf); i >= 0 && buf[i] != '\\'; --i) {;}
   buf[i] = '\0'; // /neuron.exe gone
//	printf("setneuronhome |%s|\n", buf);
   for (j=strlen(buf); j >= 0 && buf[j] != '\\'; --j) {;}
   buf[j] = '\0'; // /bin gone
   neuron_home_dos = emalloc(strlen(buf) + 1);
   strcpy(neuron_home_dos, buf);
   neuron_home = hoc_dos2unixpath(buf);
   return;
#if 0
   // but make sure it was bin Bin or BIN -- damn you bill gates
//	printf("i=%d j=%d buf=|%s|\n",i, j, buf);
   if (i == j+4
    	&&(buf[--i] == 'n' || buf[i] == 'N')
    	&&(buf[--i] == 'i' || buf[i] == 'I')
   	&&(buf[--i] == 'b' || buf[i] == 'B')
      ) {
			char buf1[256], *nh_old;
			// check for nrn.def or nrn.defaults
			// if it exists assume valid installation
			FILE* f;
			sprintf(buf1, "%s/lib/nrn.def", buf);
			if ((f = fopen(buf1, "r")) == (FILE*)0) {
				sprintf(buf1, "%s/lib/nrn.defaults", buf);
				if ((f = fopen(buf1, "r")) == (FILE*)0) {
//					printf("couldn't open %s\n", buf1);
//					printf("%s not valid neuronhome\n", buf);
					return;
				}
			}
			fclose(f);
			sprintf(buf1, "NEURONHOME=%s", buf);
			nh_old = getenv("NEURONHOME");
//			if (nh_old){ printf("nh_old from first getenv is %s\n", nh_old);
//			}else{ printf("first getenv of NEURONHOME returns nil\n");}
			if (!nh_old || stricmp(buf, nh_old) != 0) {
				printf("Setting  %s", buf1);
				if (nh_old) {
					printf(" from old value of %s\n", nh_old);
				}else{
					printf("\n");
				}
				s = emalloc(strlen(buf1)+1);
				strcpy(s, buf1);
				i = putenv(s); // arg must be global
//				printf("putenv of %s returned %d\n", s, i);
			}
   }
	neuron_home = getenv("NEURONHOME");
//			if (neuron_home){ printf("neuron_home from second getenv is %s\n", neuron_home);
//			}else{ printf("second getenv of NEURONHOME returns nil\n");}
		if (!neuron_home) MessageBox(NULL, p, "Can't compute NEURONHOME from", MB_OK);
#endif
}
void HandleOutput(char* s) {
	printf("%s", s);
}
static long exception_filter(p) LPEXCEPTION_POINTERS p; {
//	hoc_execerror("unhandled exception", "");
//	return EXCEPTION_CONTINUE_EXECUTION;
	static n = 0;
	++n;
	if (n == 1) {
		hoc_execerror("\nUnhandled Exception. This usually means a bad memory \n\
address.", "It is not possible to make a judgment as to whether it is safe\n\
to continue. If this happened while compiling a template, you will have to\n\
quit.");
	}
	if (n == 2) {
MessageBox(NULL, "Second Unhandled Exception: Quitting NEURON. You will be asked to save \
any unsaved em buffers before exiting.", "NEURON Internal ERROR", MB_OK);
		hoc_quit();
	}
	return EXCEPTION_EXECUTE_HANDLER;
}

hoc_set_unhandled_exception_filter() {
	SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)exception_filter);
}
BOOL hoc_copyfile(const char* src, const char* dest) {
	return CopyFile(src, dest, FALSE);
}

static FILE* dll_stdio_[] = {(FILE*)0x0, (FILE*)0x20, (FILE*)0x40};

void nrn_mswindll_stdio(i,o,e) FILE* i, *o, *e; {
	if (o != dll_stdio_[1]) {
		printf("nrn_mswindll_stdio stdio in dll = %p but expected %p\n", o, dll_stdio_[1]);
	}
	dll_stdio_[0] = i;
	dll_stdio_[1] = o;
	dll_stdio_[2] = e;
}

#if defined(CYGWIN)
/* the cygwin nrnmech.dll will use these pointers for stdin, stdout, stderr */
/* and when we come back to ncyg_fprintf we compare the stream to them */
FILE __files[4];
#else
#define ncyg_fprintf fprintf
#endif

int ncyg_fprintf(FILE /*_FAR*/ *stream, const char * strFmt, ...)
{
	int len;
	static char s[4096] = {0};
	va_list marker;
	va_start(marker, strFmt);
#if defined(CYGWIN)
#if 0
	printf("ncyg stdin=%lx\n", (long)stdin);
	printf("ncyg stdout=%lx\n", (long)stdout);
	printf("ncyg stderr=%lx\n", (long)stderr);
	printf("ncyg dll_stdio[0]=%lx\n", (long)dll_stdio_[0]);
	printf("ncyg dll_stdio[1]=%lx\n", (long)dll_stdio_[1]);
	printf("ncyg dll_sdtio[2]=%lx\n", (long)dll_stdio_[2]);
	printf("ncyg stream=%lx\n", (long)stream);
#endif
	if (stream == dll_stdio_[1]) {
		stream = stdout;
	}else if (stream == dll_stdio_[2]) {
		stream = stderr;
	}
	len = vsprintf(s, strFmt, marker);
	fputs(s, stream);
	return len;
#endif
}

void hoc_forward2back(char* s) {
	char* cp;
		for (cp = s; *cp; ++cp) {
			if (*cp == '/') {
				*cp = '\\';
			}
		}
}

char* hoc_back2forward(char* s) {
	char* cp = s;
	while(*cp) {
		if (*cp == '\\') {
			*cp = '/';
		}
		++cp;
	}
	return s;
}

#if HAVE_IV
void ivoc_win32_cleanup();
#endif

void hoc_win32_cleanup() {
	char buf[256];
	char* path;
#if HAVE_IV
	ivoc_win32_cleanup();
#endif
	path = getenv("TEMP");
	if (path) {
		sprintf(buf, "%s/oc%d.hl", path, getpid());
		unlink(buf);
//      DebugMessage("unlinked %s\n", buf);
	}
}

void hoc_win_exec(void) {
	int i;
	i = SW_SHOW;
	if (ifarg(2)) {
		i = (int)chkarg(2, -1000, 1000);
	}
	i = WinExec(gargstr(1), i);
	ret();
	pushx((double)i);
}

#if !defined(CYGWIN)

FILE* popen(char* s1, char* s2) {
	printf("no popen\n");
	return 0;
}

pclose(FILE* p) {
	printf("no pclose\n");
}

void hoc_check_intupt(int intupt) {
#if !OCSMALL
	MSG msg;
	 while (PeekMessage(&msg, hCurrWnd, 0, 0, PM_REMOVE))
		  {
		  TranslateMessage(&msg);
		  DispatchMessage(&msg);
		  }
#endif
}

#if defined(__MWERKS__)
void __assertfail() { printf("assertfail\n");}
#endif

#if 0
char* dos_neuronhome() {
	static char* dnrnhome;
	char* nrnhome, *cp;
	if (!dnrnhome) {
		nrnhome = getenv("NEURONHOME");
		dnrnhome = (char*)emalloc(strlen(nrnhome) + 1);
		strcpy(dnrnhome, nrnhome);
		for (cp = dnrnhome; *cp; ++cp) {
			if (*cp == '/') {
				*cp = '\\';
			}
		}
	}
	return dnrnhome;
}
#endif

#define HOCXDOS "lib/nrnsys.sh"
#define SEMA1 "tmpdos1.tmp"
#define SEMA2 "tmpdos2.tmp"

int system(const char* s) {
	char buf[256], stin[128];
	char* redirect;
	FILE* fin;

	unlink(SEMA1);
	unlink(SEMA2);
	errno = 0;
	redirect = strchr(s, '>');
	if (redirect) {/* redirection filename is first arg */
		strcpy(stin, redirect+1);
		sprintf(buf, "%s\\bin\\sh %s/%s %s %s %s",
			neuron_home, neuron_home, HOCXDOS, neuron_home, stin, expand_env_var(s));
		redirect = strchr(buf, '>');
		*redirect = '\0';
	}else{
		sprintf(buf, "%s\\bin\\sh %s/%s %s %s %s",
			neuron_home, neuron_home, HOCXDOS, neuron_home, SEMA2, expand_env_var(s));
	}
//printf("%s\n", buf);
	if (WinExec(buf, 0) < 32) {
		hoc_execerror("WinExec failed:", buf);
	}
	while((fin = fopen(SEMA1, "r")) == (FILE*)0) {
		Sleep(1);
      wmhandler_yield();
	}
	fclose(fin);
	unlink(SEMA1);
	if (!redirect && (fin = fopen(SEMA2, "r")) != (FILE*)0) {
		while(fgets(buf, 256, fin)) {
			printf("%s", buf);
		}
		fclose(fin);
		unlink(SEMA2);
	}
	return 0;
}

hoc_win_normal_cursor() {
	if (old_cursor) {
		(HCURSOR)SetClassLong(hCurrWnd, GCL_HCURSOR, (long)old_cursor);
		SetCursor(old_cursor);
		old_cursor = 0;
	}
}

hoc_win_wait_cursor() {
	static int ready = 0;
	if (!ready) {
		wait_cursor = LoadCursor(NULL, IDC_WAIT);
	}
	if (!old_cursor) {
//DebugMessage("set the wait cursor\n");
		old_cursor = (HCURSOR)SetClassLong(hCurrWnd, GCL_HCURSOR, (long)wait_cursor);
		SetCursor(wait_cursor);
	}
}

#endif /* not CYGWIN */

void hoc_winio_show(int b) {
#ifndef CYGWIN
		ShowWindow(hCurrWnd, b ? SW_SHOW : SW_HIDE);
#endif
}

#if 0
void plprint(const char* s) {
	printf("%s", s);
}
#endif
int hoc_plttext;
#if !defined(__MWERKS__)
int getpid() {
#if 0
	extern int __hInst;
	return __hInst;
#else
	return 1;
#endif
}
#endif
//hoc_close_plot(){}
//hoc_Graphmode(){ret();pushx(0.);}
//hoc_Graph(){ret();pushx(0.);}
//hoc_regraph(){ret();pushx(0.);}
//hoc_plotx(){ret();pushx(0.);}
//hoc_ploty(){ret();pushx(0.);}
hoc_Plt() {ret(); pushx(0.);}
hoc_Setcolor(){ret(); pushx(0.);}
hoc_Lw(){ret(); pushx(0.);}
hoc_settext(){ret(); pushx(0.);}
//Plot(){ret();pushx(0.);}
//axis(){ret();pushx(0.);}
//hoc_fmenu() {ret();pushx(0.);}

//int gethostname() {printf("no gethostname\n");}


//plt(int mode, double x, double y) {}
#if 0
hoc_menu_cleanup() {
#if OCSMALL
	ShowWindow(hCurrWnd, SW_SHOW);
#endif
}
#endif

//initplot() {}
#if 0
Fig_file(){}
#endif

