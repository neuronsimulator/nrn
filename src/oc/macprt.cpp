
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "hoc.h"

#include "gui-redirect.h"

extern void debugfile(const char*, ...);
extern int oc_print_from_dll(char*);
extern void single_event_run();

extern char* neuron_home;

int oc_print_from_dll(char* buf) { /* interchange \n and \r !*/
    char* cp;
    for (cp = buf; *cp != '\0'; ++cp) { /* safe because buf is already a temp buffer */
        if (*cp == '\n') {
            *cp = '\r';
        } else if (*cp == '\r') {
            *cp = '\n';
        }
    }
    return printf("%s", buf);
}

/* jijun 4/22/97, 4/23/97 */
extern void setneuronhome(const char* p) {
    CInfoPBRec myPB;
    short vRefNum;
    long dirID;
    Str255 dirName;
    char prePath[256];
    static char fullPath[256];

    OSErr err = HGetVol((0), &vRefNum, &dirID);
    if (err == noErr) {
        myPB.dirInfo.ioNamePtr = dirName;
        myPB.dirInfo.ioVRefNum = vRefNum;
        myPB.dirInfo.ioDrParID = dirID;
        myPB.dirInfo.ioFDirIndex = -1;
        do {
            myPB.dirInfo.ioDrDirID = myPB.dirInfo.ioDrParID;
            err = PBGetCatInfoSync(&myPB);
            if (err == noErr) {
                dirName[dirName[0] + 1] = '\0';
                strcpy(prePath, &dirName[1]);
                strcat(prePath, ":");
                strcat(prePath, fullPath);
                strcpy(fullPath, prePath);
            }
        } while (myPB.dirInfo.ioDrDirID > 2);
    }

    neuron_home = fullPath;
    // get rid of last ':'
    neuron_home[strlen(neuron_home) - 1] = '\0';
    //    debugfile("neuron_home = %s\n", neuron_home);
}

char* getenv(const char* s) {
    static char buf[200];
    if (strcmp(s, "NEURONHOME") == 0) {
        return neuron_home;
    }
    if (strcmp(s, "NRNDEMO") == 0) {
        strcpy(buf, neuron_home);
        strcat(buf, ":demo:");
        return buf;
    }
    printf("getenv: don't know |%s|\n", s);
    return 0;
}

int hoc_copyfile(const char* src, const char* dest) {
    return 0;
}

void hoc_check_intupt(int intupt) {
#if 1
    extern void set_intset();
    EventRecord e;
    if (EventAvail(keyDownMask, &e)) {
        // debugfile("%d\n", e.what);
        if (e.what == keyDown) {
            char c = e.message & charCodeMask;
            if (c == 0x03) {
                set_intset();
            }
        }
        // single_event_run();
    }
#endif
}

FILE* popen(char* s1, char* s2) {
    printf("no popen\n");
    return 0;
}

pclose(FILE* p) {
    printf("no pclose\n");
}

hoc_win_normal_cursor() {}

hoc_win_wait_cursor() {}

void plprint(const char* s) {
    printf("%s", s);
}
int hoc_plttext;
/*
int getpid() {
    return 1;
}
*/
hoc_close_plot() {}
hoc_Graphmode() {
    TRY_GUI_REDIRECT_DOUBLE("graphmode", NULL);
    ret();
    pushx(0.);
}
hoc_Graph() {
    TRY_GUI_REDIRECT_DOUBLE("graph", NULL);
    ret();
    pushx(0.);
}
hoc_regraph() {
    TRY_GUI_REDIRECT_DOUBLE("regraph", NULL);
    ret();
    pushx(0.);
}
hoc_plotx() {
    TRY_GUI_REDIRECT_DOUBLE("plotx", NULL);
    ret();
    pushx(0.);
}
hoc_ploty() {
    TRY_GUI_REDIRECT_DOUBLE("ploty", NULL);
    ret();
    pushx(0.);
}
hoc_Plt() {
    TRY_GUI_REDIRECT_DOUBLE("plt", NULL);
    ret();
    pushx(0.);
}
hoc_Setcolor() {
    TRY_GUI_REDIRECT_DOUBLE("setcolor", NULL);
    ret();
    pushx(0.);
}
hoc_Lw() {
    ret();
    pushx(0.);
}
hoc_settext() {
    TRY_GUI_REDIRECT_DOUBLE("settext", NULL);
    ret();
    pushx(0.);
}
hoc_Plot() {
    TRY_GUI_REDIRECT_DOUBLE("plot", NULL);
    ret();
    pushx(0.);
}
hoc_axis() {
    TRY_GUI_REDIRECT_DOUBLE("axis", NULL);
    ret();
    pushx(0.);
}


// int gethostname() {printf("no gethostname\n");}


plt(int mode, double x, double y) {}


initplot() {}
