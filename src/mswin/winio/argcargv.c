/*
ARGCARGV.C -- WinMain->main startup for WINIO library
Changed considerably since the version in MSJ (May 1991)

from "Undocumented Windows"  (Addison-Wesley, 1992)
by Andrew Schulman, Dave Maxey and Matt Pietrek.

Copyright (c) Dave Maxey and Andrew Schulman 1991-1992

MSC/C++ 7.0 has QuickWin library.  It's not adequate for our
purposes, but its main() gets in the way of ours.  Need to
link with ARGCARGV.OBJ; can't put ARGCARGV.OBJ in a library
*/

#include "windows.h"
#include <stdlib.h>
#include <string.h>
#include "winio.h"

#define MAIN_BUFFER 32760

#ifdef __BORLANDC__
// Borland must have followed (incorrect) doc in SDK Guide, p. 14-3
#define argc _argc
#define argv _argv
// #define argc _C0argc
// #define argv _C0argv

extern int _argc;
extern char **_argv;
// extern int _C0argc;
// extern char **_C0argv;
#else
// Microsoft C code per MSJ, May 1991, pp. 135-6
#define argc __argc
#define argv __argv

extern int __argc;
extern char **__argv;
#endif

// weird! couldn't find environ
// oh well, nobody ever uses it!
#if defined(__MWERKS__) || (defined(_MSC_VER) && (_MSC_VER >= 700))
extern int main(int argc, char **argv);
#else
extern int main(int argc, char **argv, char **envp);
#endif

void getexefilename(HANDLE hInst, char *strName);
#if __WIN32__
static void msgetarg(int* pargc, char*** pargv) {
	static char buf[1024];
	static char* av[20];
	int ac, instr;
	char* p;
	*pargv = av;
	strcpy(buf, GetCommandLine());
	ac = 0;
	av[ac++] = buf;
	// strings enclosed in "" are a single arg.
	// no backslash escapes since a backslash is a path char 
	instr = 0;
	for (p = buf; *p; ++p) {
		if ((instr == 0) && (*p == ' ' || *p == '\t')) {
			*p = '\0';
			// now skip multiple spaces
			while (p[1] == ' ' || p[1] == '\t') {
				++p;
			}
			if (p[1]) {
				av[ac++] = p+1;
			}
		}else if (*p == '"') {
			instr = (instr == 1) ? 0 : 1;
		}
	}
	*pargc = ac;
#if 0
	if (ac > 1) {
		MessageBox(NULL, av[1], "msgetarg", MB_OK);
	}
#endif
}
#endif
HANDLE __hInst;
HANDLE __hPrevInst;
LPSTR __lpCmdLine;
int __nCmdShow;
HWND __hMainWnd;
UINT __hAppTimer;
char __szModule[9] = {0};

#if 1 && defined(__MWERKS__)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
#else
int PASCAL WinMain(HANDLE hInstance, HANDLE hPrevInstance,
#endif
    LPSTR lpCmdLine, int nCmdShow)
    {
	 int ret;
#if __WIN32__
	 int argc;
	 char** argv;
	 msgetarg(&argc, &argv);
#endif
    
    __hInst = hInstance;
    __hPrevInst = hPrevInstance;
    __lpCmdLine = lpCmdLine;
    __nCmdShow = nCmdShow;
    
    getexefilename(__hInst, __szModule);
    winio_about(__szModule);    // default; can override
    
    if (! winio_init())
        {
        winio_warn(FALSE, __szModule, "Could not initialize");
        return 1;
        }
    
	 if ((__hMainWnd = winio_window((LPSTR) NULL, MAIN_BUFFER,
					 WW_HASMENU | WW_EXITALLOWED)) != 0)
        {
        // App timer to allow multitasking
		  __hAppTimer = SetTimer(NULL, 0xABCD, 100, NULL);
    
        winio_setcurrent(__hMainWnd);
        
#if defined(__MWERKS__) || (defined(_MSC_VER) && (_MSC_VER >= 700))
        ret = main(argc, argv);
#else   
        ret = main(argc, argv, environ);
#endif  
//MessageBox(NULL, "calling winio_end", "ss", MB_OK);
        winio_end();
//MessageBox(NULL, "back from winio_end", "ss", MB_OK);
        if (__hAppTimer)
            KillTimer(NULL, __hAppTimer);
        }
    else
        {
        winio_warn(FALSE, __szModule, "Could not create main window");
        ret = -1;
        }
// MessageBox(NULL, "return", "sss", MB_OK);
    return ret;
    }


void getexefilename(HANDLE hInst, char *strName)
    {
    char str[128];
    char *p;

    // can use hInst as well as hMod (GetExePtr does the trick)
    GetModuleFileName(hInst, str, 128);
    p = &str[strlen(str) - 1];
    
    for ( ; (p != str) && (*p != '\\'); p--)
        if (*p == '.') *p = 0;
        
    strcpy(strName, *p == '\\' ? ++p : p);
    }
