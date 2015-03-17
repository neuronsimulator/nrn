// this is the wrapper that allows one to create shortcuts and
// associate hoc files with neuron.exe without having to consider
// the issues regarding shells and the rxvt console. The latter
// can be given extra arguments in the lib/neuron.sh file which
// finally executes nrniv.exe.  Nrniv.exe can be run by itself if
// it does not need a console or system("command")

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "d2upath.c"

char* nrnhome;
char* nh;

static void setneuronhome() {
	int i, j;
	char buf[256];
	GetModuleFileName(NULL, buf, 256);
	for (i=strlen(buf); i >= 0 && buf[i] != '\\'; --i) {;}
	buf[i] = '\0'; // /neuron.exe gone
	for (i=strlen(buf); i >= 0 && buf[i] != '\\'; --i) {;}
	buf[i] = '\0'; // /bin gone
	nrnhome = new char[strlen(buf)+1];
	strcpy(nrnhome, buf);
}

static char* argstr(int argc, char** argv) {
	// put args into single string, escaping spaces in args.
	int i, j, cnt;
	char* s;
	char* a;
	char* u;
	cnt = 100;
	for (i=1; i < argc; ++i) {
		cnt += strlen(argv[i]) + 20;
	}
	s = new char[cnt];
	j = 0;
	for (i=1; i < argc; ++i) {
		// convert dos to unix path and space to @@
		// the latter will be converted back to space in src/oc/hoc.c
		// cygwin 7 need to convert x: and x:/ and x:\ to
		// /cygdrive/x/
		u = hoc_dos2unixpath(argv[i]);
		for (a = u; *a; ++a) {
			if (*a == ' ') {
				s[j++] = '@';
				s[j++] = '@';
			}else{
				s[j++] = *a;
			}
		}
		if (i < argc-1) {
			s[j++] = ' ';
		}
		free(u);
	}
	s[j] = '\0';
	return s;
}
	
int main(int argc, char** argv) {
	int err;
	char* buf;
	char* args;
	char* msg;

#if !defined(MINGW)
	ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
	setneuronhome();
	nh = hoc_dos2unixpath(nrnhome);
	args = argstr(argc, argv);
	buf = new char[strlen(args) + 6*strlen(nh) + 200];
#if defined(MINGW)
	//sprintf(buf, "%s\\mingw\\bin\\bash.exe -rcfile %s/lib/nrnstart.bsh -i %s/lib/neuron2.sh nrngui %s", nrnhome, nh, nh, args);
	if (nh[1] == ':') {
		nh[1] = nh[0];
		nh[0] = '/';
	}
	sprintf(buf, "%s\\mingw\\bin\\bash.exe -i %s/lib/neuron3.sh %s nrngui %s", nrnhome, nh, nh, args);
//MessageBox(0, buf, "NEURON", MB_OK);
#else
	sprintf(buf, "%s\\bin\\mintty -c %s/lib/minttyrc %s/bin/bash --rcfile %s/lib/nrnstart.bsh %s/lib/neuron.sh %s %s", nrnhome, nh, nh, nh, nh, nh, args);
#endif
	msg = new char[strlen(buf) + 100];
	err = WinExec(buf, SW_SHOW);
	if (err < 32) {
		sprintf(msg, "Cannot WinExec %s\n", buf);
		MessageBox(0, msg, "NEURON", MB_OK);
	}
	return 0;
}
