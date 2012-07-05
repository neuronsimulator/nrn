/* Generic launcher for starting from the macnrn.term file */
/*
This replaces the osacompile -o nrngui.app prototype_applescript.txt
since I cannot get it to work with 10.2
*/

#include <../../nrnconf.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

extern char* mac_args();

FILE* f;
#define FNAME ".nrn_as2sh"

static char fname[1024];
static char* home;
static char* instdir;
static char* myname;
static void mkfile(char* farg);

int main(int argc, char** argv) {
	char* termcmd;
	char* cp;
	char cwd[1024];
	char* farg;

	home = getenv("HOME");

	instdir = (char*)malloc(strlen(argv[0]));
	strcpy(instdir, argv[0]);
	cp = strstr(instdir, ".app/Contents/MacOS/a.out");
	*cp='\0';
	for (myname=instdir+strlen(instdir)-1; *myname != '/'; --myname) {
		;
	}
	*myname = '\0';
	++myname;

	getcwd(cwd, 1024);

	farg = mac_args();

	mkfile(farg);

	termcmd = (char*)malloc(strlen(instdir) + 100);
	sprintf(termcmd, "/usr/bin/open \"%s/nrn/%s/bin/macnrn.term\"", instdir, ncpu);
	system(termcmd);

	return 0;
}

void mkfile(char* farg) {
	sprintf(fname, "%s/%s", home, FNAME);
	f = fopen(fname, "w");
	fprintf(f, "#!/bin/sh\n");
	fprintf(f, "NRNHOME=\"%s/nrn\"\n", instdir);
	fprintf(f, "NEURONHOME=\"${NRNHOME}/share/nrn\"\n");
	fprintf(f, "CPU=%s\n", ncpu);
	fprintf(f, "NRNBIN=\"${NRNHOME}/%s/bin/\"\n", ncpu);
	fprintf(f, "PATH=\"${NRNHOME}/%s/bin:${PATH}\"\n", ncpu);
	fprintf(f, "export NRNHOME\n");
	fprintf(f, "export NEURONHOME\n");
	fprintf(f, "export NRNBIN\n");
	fprintf(f, "export PATH\n");
	fprintf(f, "export CPU\n");
#if carbon
	fprintf(f, "nrncarbon=yes\n");
	fprintf(f, "export nrncarbon\n");
#endif
	fprintf(f, "cd \"${NRNHOME}/%s/bin\"\n", ncpu);
	fprintf(f, "./%s.sh%s\n", myname, farg);
	fclose(f);
}

