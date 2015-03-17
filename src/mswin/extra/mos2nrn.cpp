// this is a copy of neuron.exe but no space handling and calls
// mos2nrn.sh

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
	// put args into single string, each with enclosing ""
	int i, j, cnt;
	char* s;
	char* a;
	char* u;
	cnt = 100;
	for (i=1; i < argc; ++i) {
		cnt += strlen(argv[i])+20;
	}
	s = new char[cnt];
	j = 0;
	for (i=1; i < argc; ++i) {
		s[j++] = '"';
		u = hoc_dos2unixpath(argv[i]);
		for (a = u; *a; ++a) {
			s[j++] = *a;
		}
		s[j++] = '"';
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
	char* temp;

	setneuronhome();
	nh = hoc_dos2unixpath(nrnhome);
	args = argstr(argc, argv);
	temp = getenv("TEMP");
	if (!temp) { temp = "c:/tmp"; }
	temp = hoc_dos2unixpath(temp);
	buf = new char[strlen(args) + 3*strlen(nh) + 200 + strlen(temp)];
	
#if defined(MINGW)
	if (nh[1] == ':') {
		nh[1] = nh[0];
		nh[0] = '/';
	}
	
	sprintf(buf, "%s\\mingw\\bin\\bash.exe %s/lib/mos2nrn3.sh %s %s %s", nrnhome, nh, temp, nh, args);
#else
	sprintf(buf, "%s\\bin\\sh %s/lib/mos2nrn.sh %s %s", nrnhome, nh, nh, args);
#endif
	msg = new char[strlen(buf) + 100];
	err = WinExec(buf, SW_SHOW);
	if (err < 32) {
		sprintf(msg, "Cannot WinExec %s\n", buf);
		MessageBox(0, msg, "NEURON", MB_OK);
	}
	return 0;
}
