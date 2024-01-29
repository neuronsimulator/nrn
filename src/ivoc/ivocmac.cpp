#include <../../nrnconf.h>

#include <CodeFragments.h>


#include <stdio.h>
#include "apwindow.h"
#include "ivoc.h"
#include "rubband.h"
#include "symdir.h"
#include "oc2iv.h"
#include "graph.h"
#include <InterViews/window.h>
#include <IV-Mac/window.h>
#include <InterViews/display.h>
#include <InterViews/cursor.h>
#include <Dispatch/dispatcher.h>

typedef void (*NrnBBSCallback)(const char*);


char* mktemp(char*) {
    return NULL;
}

bool nrnbbs_connect() {
    return false;
}
void nrnbbs_disconnect() {}
bool nrnbbs_connected() {
    return false;
}

void nrnbbs_post(const char*) {}
void nrnbbs_post_int(const char*, int) {}
void nrnbbs_post_string(const char*, const char*) {}

bool nrnbbs_take(const char*) {
    return false;
}
bool nrnbbs_take_int(const char*, int*) {
    return false;
}
bool nrnbbs_take_string(const char*, char*) {
    return false;
}

bool nrnbbs_look(const char*) {
    return false;
}

void nrnbbs_exec(const char*) {}

void nrnbbs_notify(const char*, NrnBBSCallback) {}

void nrnbbs_wait(bool* pflag = (bool*) 0) {}

bool is_mac_dll(FSSpec*);
bool mac_open_dll(const char*, FSSpec*);

bool is_mac_dll(FSSpec* fs) {
    FInfo finfo;
    FSpGetFInfo(fs, &finfo);
    return finfo.fdType == 'shlb';
}

extern Symlist *hoc_symlist, *hoc_built_in_symlist;
extern OSErr __path2fss(const char* name, FSSpec*);
extern void hoc_nrn_load_dll();

static long fsspec2id(FSSpec* fs) {
    CInfoPBRec ci;
    Str255 name;
    int i;
    for (i = 0; i < 64; ++i) {
        name[i] = fs->name[i];
    }
    ci.hFileInfo.ioCompletion = 0;
    ci.hFileInfo.ioNamePtr = name;
    ci.hFileInfo.ioVRefNum = fs->vRefNum;
    ci.hFileInfo.ioDirID = fs->parID;
    ci.hFileInfo.ioFDirIndex = 0;
    OSErr err = PBGetCatInfo(&ci, false);
    return ci.hFileInfo.ioDirID;
}
static int ndll;
static long dllid[10];

bool mac_open_dll(const char* name, FSSpec* fs) {
    CFragConnectionID id;
    Ptr mainaddr;
    Str255 sname;
    int j;
    long fid = fsspec2id(fs);
    for (j = 0; j < ndll; ++j) {
        if (dllid[j] == fid) {
            printf("%s DLL already loaded\n", name);
            return false;
        }
    }
    dllid[ndll] = fid;
    ndll = (ndll < 10) ? ndll + 1 : 10;
    printf("Loading DLL %s\n", name);
#if 1
    OSErr myErr = GetDiskFragment(fs, 0, kCFragGoesToEOF, 0, kLoadCFrag, &id, &mainaddr, sname);
    if (myErr) {
        sname[sname[0] + 1] = '\0';
        printf("dll load error %d\n%s\n", myErr, sname + 1);
        return false;
    }
    //	printf("successfully loaded %s\n", name);
    //	printf("mainaddr=%p\n", mainaddr);
    mainaddr = NULL;
    long cnt;
    myErr = CountSymbols(id, &cnt);
    //	printf("symbols exported = %d\n", cnt);
    for (long i = 0; i < cnt; ++i) {
        Ptr symaddr;
        CFragSymbolClass symclass;
        myErr = GetIndSymbol(id, i, sname, &symaddr, &symclass);
        sname[sname[0] + 1] = '\0';
        if (strcmp((sname + 1), "main") == 0) {
            mainaddr = symaddr;
        }
        //		printf("symbol %d name %s\n", i, sname+1);
    }
//	printf("mainaddr=%p\n", mainaddr);
#if 1
    if (mainaddr) {
        Symlist* sav = hoc_symlist;
        hoc_symlist = hoc_built_in_symlist;
        hoc_built_in_symlist = NULL;
        (*(Pfri) mainaddr)();
        hoc_built_in_symlist = hoc_symlist;
        hoc_symlist = sav;
        return true;
    }
#endif
#endif
    return false;
}

bool mac_load_dll(const char* name) {
    FSSpec fss;
    if ((__path2fss(name, &fss) != fnfErr) && is_mac_dll(&fss)) {
        return mac_open_dll(name, &fss);
    }
    return false;
}

void hoc_nrn_load_dll() {
    int b = mac_load_dll(expand_env_var(gargstr(1)));
    ret((double) b);
}

void pwmimpl_redraw(Window* w) {
    w->rep()->MACpaint();
}

void ivoc_bring_to_top(Window* w) {
    BringToFront(w->rep()->macWindow());
}

//---------------------------------------------------------


void Oc::cleanup() {
    if (help_cursor_) {
        delete help_cursor_;
    }
}

void PrintableWindow::hide() {
    unmap();
}

void PrintableWindow::xmove(int x, int y) {
    WindowPtr theWin = Window::rep()->macWindow();
    MoveWindow(theWin, (x + 1), (y + 17), true);
}
int PrintableWindow::xleft() const {
    WindowRep& w = *Window::rep();
    if (w.bound()) {
        GrafPtr oldPort;
        GetPort(&oldPort);
        WindowPtr theWin = w.macWindow();
        w.setport();
        Point upperLeft;
        upperLeft.h = theWin->portRect.left;
        upperLeft.v = theWin->portRect.top;
        LocalToGlobal(&upperLeft);
        SetPort(oldPort);
        return upperLeft.h - 1;
    } else {
        return 0;
    }
}
int PrintableWindow::xtop() const {
    WindowRep& w = *Window::rep();
    if (w.bound()) {
        GrafPtr oldPort;
        GetPort(&oldPort);
        WindowPtr theWin = w.macWindow();
        w.setport();
        Point upperLeft;
        upperLeft.h = theWin->portRect.left;
        upperLeft.v = theWin->portRect.top;
        LocalToGlobal(&upperLeft);
        SetPort(oldPort);
        return upperLeft.v - 17;
    } else {
        return 0;
    }
}

void PrintableWindow::xplace(int x, int y) {
    WindowRep& wr = *Window::rep();
    // printf("xplace %d %d %d\n", x, y,wr.bound());
    if (wr.bound()) {
        xmove(x, y);
    } else {
        xplace_ = true;
        xleft_ = x + 1;
        xtop_ = y + 17;
    }
}
void PrintableWindow::default_geometry() {
    DismissableWindow::default_geometry();
    // the problem is that at this point the canvas size is not necessarly correct.
    if (xplace_) {
        pplace(xleft_, display()->pheight() - xtop_ - canvas()->pheight());
    }
}

// following analogy to MACwindow::MACpaint
static CGrafPtr cg_;
static GDHandle gd_;

void Rubberband::rubber_on(Canvas* c) {
    //	printf("Rubberband::rubber_on\n");
    //	c->front_buffer();
    GetGWorld(&cg_, &gd_);
    WindowPtr mw = c->window()->rep()->macWindow();
    SetGWorld((CGrafPort*) mw, GetMainDevice());
}
void Rubberband::rubber_off(Canvas* c) {
    //	c->back_buffer();
    SetGWorld(cg_, gd_);
    //	printf("Rubberband::rubber_off\n");
}

IOHandler::IOHandler() {}
IOHandler::~IOHandler() {}
int IOHandler::inputReady(int) {
    return 0;
}
int IOHandler::outputReady(int) {
    return 0;
}
int IOHandler::exceptionRaised(int) {
    return 0;
}
void IOHandler::timerExpired(long, long) {}
void IOHandler::childStatus(pid_t, int) {}
