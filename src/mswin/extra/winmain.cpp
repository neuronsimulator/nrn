
#include "windows.h"
#include <stdlib.h>
#include <string.h>

extern int main(int argc, char **argv);

/* one arg only. go to first space and then all the rest is the arg */
static void msgetarg(int* pargc, char*** pargv) {
	static char buf[1024];
	static char* av[20];
	int space, ac;
	char* p;
	*pargv = av;
	strcpy(buf, GetCommandLine());
#if 0
	MessageBox(NULL, buf, "msgetarg", MB_OK);
#endif
	space = 0;
	ac = 0;
	av[ac++] = buf;
#if 0
	for (p = buf; *p; ++p) {
		 while (*p && (*p == ' ' || *p == '\t')) {
			*p++ = '\0';
			space = 1;
		 }
		 if (space && *p) {
			av[ac++] = p;
		 }
		 space = 0;
	}
#else
	for (p = buf; *p; ++p) {
		if (*p == ' ' || *p == '\t') {
			*p++ = '\0';
			av[ac++] = p;
			break;
		}
	}
#endif
	*pargc = ac;
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	 LPSTR lpCmdLine, int nCmdShow)
	 {
	 int ret;
	 int argc;
	 char** argv;
	 msgetarg(&argc, &argv);
	 ret = main(argc, argv);
	 return ret;
	 }

