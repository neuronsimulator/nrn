#ifndef oc_h
#define oc_h

#include <Dispatch/iohandler.h>
#include <Dispatch/dispatcher.h>
#include <InterViews/session.h>
#include <OS/string.h>
#include <stdio.h>
#include <ivstream.h>
class Observer;
class Observable;
class Cursor;
struct Object;

class HandleStdin: public IOHandler {
public:
	HandleStdin();
        virtual int inputReady(int fd);
        virtual int  exceptionRaised(int fd);
	boolean stdinSeen_;
	boolean acceptInput_; 
};

struct Symbol;
struct Symlist;

class Oc {
public:
	Oc();
	Oc(Session*, char* pname = nil, char** env = nil);
	virtual ~Oc();

	int run(int argc, char** argv);
	int run(const char *, boolean show_err_mes = true);

	Symbol* parseExpr(const char *, Symlist** = nil);
	double runExpr(Symbol*);
	static boolean valid_expr(Symbol*);
	static boolean valid_stmt(const char*, Object* ob = nil);
	const char* name(Symbol*);
	
	void notifyHocValue(); // loops over HocValueBS buttonstates.

	void notify(); // called on doNotify from oc
	void notify_attach(Observer*); // add to notify list
	void notify_detach(Observer*);
		
	void notify_freed(void (*pf)(void*, int)); // register a callback func
	void notify_when_freed(void* p, Observer*);
	void notify_when_freed(double* p, Observer*);
	void notify_pointer_disconnect(Observer*);
	
	static Session* getSession();
	static int getStdinSeen() {return handleStdin_->stdinSeen_;}
	static void setStdinSeen(boolean i) {handleStdin_->stdinSeen_ = i;}
	static boolean setAcceptInput(boolean);
	static boolean helpmode() {return helpmode_;}
	static void helpmode(boolean);
	static void helpmode(Window*);
	static void help(const char*);

	static ostream* save_stream;
	static void cleanup();
private:
	static int refcnt_;
	static Session* session_;
	static HandleStdin* handleStdin_;
	static boolean helpmode_;
	static Cursor* help_cursor();
	static Cursor* help_cursor_;
	static Observable* notify_change_;
};

#endif
