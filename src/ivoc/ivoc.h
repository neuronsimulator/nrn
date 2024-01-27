#ifndef oc_h
#define oc_h

#include <Dispatch/iohandler.h>
#include <Dispatch/dispatcher.h>
#include <InterViews/session.h>
#include <OS/string.h>
#include <stdio.h>
#include "gui-redirect.h"
extern int nrn_err_dialog_active_;

#include <ostream>

#ifdef MINGW
extern bool nrn_is_gui_thread();
extern void nrn_gui_exec(void (*)(void*), void*);
#endif

class Observer;
class Observable;
class Cursor;
struct Object;

class HandleStdin: public IOHandler {
  public:
    HandleStdin();
    virtual int inputReady(int fd);
    virtual int exceptionRaised(int fd);
    bool stdinSeen_;
    bool acceptInput_;
};

struct Symbol;
struct Symlist;

class Oc {
  public:
    Oc();
    Oc(Session*, const char* pname = NULL, const char** env = NULL);
    virtual ~Oc();

    int run(int argc, const char** argv);
    int run(const char*, bool show_err_mes = true);

    Symbol* parseExpr(const char*, Symlist** = NULL);
    double runExpr(Symbol*);
    static bool valid_expr(Symbol*);
    static bool valid_stmt(const char*, Object* ob = NULL);
    const char* name(Symbol*);

    void notifyHocValue();  // loops over HocValueBS buttonstates.

    void notify();                  // called on doNotify from oc
    void notify_attach(Observer*);  // add to notify list
    void notify_detach(Observer*);

    void notify_freed(void (*pf)(void*, int));  // register a callback func
    void notify_when_freed(void* p, Observer*);
    void notify_pointer_disconnect(Observer*);

    static Session* getSession();
    static int getStdinSeen() {
        return handleStdin_->stdinSeen_;
    }
    static void setStdinSeen(bool i) {
        handleStdin_->stdinSeen_ = i;
    }
    static bool setAcceptInput(bool);
    static bool helpmode() {
        return helpmode_;
    }
    static void helpmode(bool);
    static void helpmode(Window*);
    static void help(const char*);

    static std::ostream* save_stream;
    static void cleanup();

  private:
    static int refcnt_;
    static Session* session_;
    static HandleStdin* handleStdin_;
    static bool helpmode_;
    static Cursor* help_cursor();
    static Cursor* help_cursor_;
    static Observable* notify_change_;
};

#endif
