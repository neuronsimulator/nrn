// this is a copy of neuron.exe but no space handling and calls
// mos2nrn.sh

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstdio>
#include "d2upath.cpp"

char* nrnhome;
char* nh;

static void setneuronhome() {
    int i, j;
    char buf[256];
    GetModuleFileName(NULL, buf, 256);
    for (i = strlen(buf); i >= 0 && buf[i] != '\\'; --i) {
        ;
    }
    buf[i] = '\0';  // /neuron.exe gone
    for (i = strlen(buf); i >= 0 && buf[i] != '\\'; --i) {
        ;
    }
    buf[i] = '\0';  // /bin gone
    nrnhome = new char[strlen(buf) + 1];
    strcpy(nrnhome, buf);
}

static char* argstr(int argc, char** argv) {
    // put args into single string, each with enclosing ""
    int i, j, cnt;
    char* s;
    char* a;
    char* u;
    cnt = 100;
    for (i = 1; i < argc; ++i) {
        cnt += strlen(argv[i]) + 20;
    }
    s = new char[cnt];
    j = 0;
    for (i = 1; i < argc; ++i) {
        s[j++] = '"';
        u = hoc_dos2unixpath(argv[i]);
        for (a = u; *a; ++a) {
            s[j++] = *a;
        }
        s[j++] = '"';
        if (i < argc - 1) {
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
    if (!temp) {
        temp = strdup("c:/tmp");
    }
    temp = hoc_dos2unixpath(temp);
    auto const bufsz = strlen(args) + 3 * strlen(nh) + 200 + strlen(temp);
    buf = new char[bufsz];

#ifdef MINGW
#if 0
	if (nh[1] == ':') {
		nh[1] = nh[0];
		nh[0] = '/';
	}
#endif

    std::snprintf(buf,
                  bufsz,
                  "%s\\mingw\\usr\\bin\\bash.exe %s/lib/mos2nrn3.sh %s %s %s",
                  nrnhome,
                  nh,
                  temp,
                  nh,
                  args);
#else
    std::snprintf(buf, bufsz, "%s\\bin\\sh %s/lib/mos2nrn.sh %s %s", nrnhome, nh, nh, args);
#endif
    auto const sz = strlen(buf) + 100;
    msg = new char[sz];
    err = WinExec(buf, SW_SHOW);
    if (err < 32) {
        std::snprintf(msg, sz, "Cannot WinExec %s\n", buf);
        MessageBox(0, msg, "NEURON", MB_OK);
    }
    delete[] msg;
    return 0;
}
