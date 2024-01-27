#ifndef objcmd_h
#define objcmd_h

#include <memory>

#include <InterViews/observe.h>
#if HAVE_IV
#include <InterViews/action.h>
#include "rubband.h"
#endif

struct Object;

// command to be executed within scope of object.

class HocCommand: public Observer {
  public:
    HocCommand(const char*);
    HocCommand(const char*, Object*);
    HocCommand(Object* paction);  // Python method call or tuple with args
    virtual ~HocCommand();
    int execute(bool notify = true);
    int execute(const char*, bool notify = true);
    int exec_strret(char* buf, int size, bool notify = true);  // for python callback returning a
                                                               // string
    const char* name();
    virtual void update(Observable*);
    virtual void audit();
    virtual void help();
    double func_call(int narg, int* perr = NULL);  // perr used only by pyobject
    Object* object() {
        return obj_;
    }
    Object* pyobject() {
        return po_;
    }

  private:
    void init(const char*, Object*);

  private:
    Object* obj_;
    std::unique_ptr<std::string> s_{};
    Object* po_;
};

#if HAVE_IV
class HocCommandAction: public Action {
  public:
    HocCommandAction(HocCommand*);
    virtual ~HocCommandAction();
    virtual void execute();

  private:
    HocCommand* hc_;
};

class HocCommandTool: public Rubberband {
  public:
    HocCommandTool(HocCommand*);
    virtual ~HocCommandTool();
    virtual bool event(Event&);
    HocCommand* hc_;
};
#endif

#endif
