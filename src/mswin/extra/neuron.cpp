// this is the wrapper that allows one to create shortcuts and
// associate hoc files with neuron.exe without having to consider
// the issues regarding shells and the rxvt console. The latter
// can be given extra arguments in the lib/neuron.sh file which
// finally executes nrniv.exe.  Nrniv.exe can be run by itself if
// it does not need a console or system("command")
#include <windows.h>

#include "d2upath.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

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
    // put args into single string, escaping spaces in args.
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
        // convert dos to unix path and space to @@
        // the latter will be converted back to space in src/oc/hoc.c
        // cygwin 7 need to convert x: and x:/ and x:\ to
        // /cygdrive/x/
        u = hoc_dos2unixpath(argv[i]);
        for (a = u; *a; ++a) {
            if (*a == ' ') {
                s[j++] = '@';
                s[j++] = '@';
            } else {
                s[j++] = *a;
            }
        }
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
    char* args;

#ifndef MINGW
    ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
    setneuronhome();
    nh = hoc_dos2unixpath(nrnhome);
    args = argstr(argc, argv);
    auto const buf_size = strlen(args) + 6 * strlen(nh) + 200;
    char* const buf = new char[buf_size];
#ifdef MINGW
    std::snprintf(buf,
                  buf_size,
                  "%s\\mingw\\usr\\bin\\bash.exe -i %s/lib/neuron3.sh %s nrngui %s",
                  nrnhome,
                  nh,
                  nh,
                  args);
#else
    std::snprintf(buf,
                  buf_size,
                  "%s\\bin\\mintty -c %s/lib/minttyrc %s/bin/bash --rcfile %s/lib/nrnstart.bsh "
                  "%s/lib/neuron.sh %s %s",
                  nrnhome,
                  nh,
                  nh,
                  nh,
                  nh,
                  nh,
                  args);
#endif
    auto const msg_size = strlen(buf) + 100;
    char* const msg = new char[msg_size];

    // Windows 11 exits immediately
    // https://stackoverflow.com/questions/2255608/force-application-to-wait-until-winexec-has-completed
    // WinExec is no longer recommended. You can use CreateProcess and WaitForSingleObject as
    // shown in this example on Creating Processes.
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    // Start the child process.
    err = CreateProcess(NULL,   // No module name (use command line)
                        buf,    // Command line
                        NULL,   // Process handle not inheritable
                        NULL,   // Thread handle not inheritable
                        FALSE,  // Set handle inheritance to FALSE
                        0,      // No creation flags
                        NULL,   // Use parent's environment block
                        NULL,   // Use parent's starting directory
                        &si,    // Pointer to STARTUPINFO structure
                        &pi);   // Pointer to PROCESS_INFORMATION structure

    if (!err) {
        std::snprintf(msg, msg_size, "CreateProcess failed (%d).  %s\n", GetLastError(), buf);
        MessageBox(0, msg, "NEURON", MB_OK);
        return 1;
    }

    // Wait until child process exits.
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Close process and thread handles.
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 0;
}
