#ifndef oc2iv_h
#define oc2iv_h

#include <string.h>
#include <stdio.h>
// common things in oc that can be used by ivoc
#include "hocdec.h"

// xmenu
#define CChar const char
extern void hoc_ivpanel(CChar*, bool h = false);
extern void hoc_ivpanelmap(int scroll = -1);
extern void hoc_ivbutton(CChar* name, CChar* action, Object* pyact = 0);
extern void hoc_ivradiobutton(CChar* name, CChar* action, bool activate = false, Object* pyact = 0);
extern void hoc_ivmenu(CChar*, bool add2menubar = false);
extern void hoc_ivvarmenu(CChar*, CChar*, bool add2menubar = false, Object* pyvar = NULL);
extern void hoc_ivvalue(CChar* name, CChar* variable, bool deflt = false, Object* pyvar = 0);
extern void hoc_ivfixedvalue(CChar* name,
                             CChar* variable,
                             bool deflt = false,
                             bool usepointer = false);
extern void hoc_ivvalue_keep_updated(CChar* name, CChar* variable, Object* pyvar = 0);
void hoc_ivpvalue(CChar* name,
                  neuron::container::data_handle<double>,
                  bool deflt = false,
                  HocSymExtension* extra = NULL);
extern void hoc_ivvaluerun(CChar* name,
                           CChar* variable,
                           CChar* action,
                           bool deflt = false,
                           bool canrun = false,
                           bool usepointer = false,
                           Object* pyvar = 0,
                           Object* pyact = 0);
void hoc_ivvaluerun_ex(CChar* name,
                       CChar* var,
                       neuron::container::data_handle<double> pvar,
                       Object* pyvar,
                       CChar* action,
                       Object* pyact,
                       bool deflt = false,
                       bool canrun = false,
                       bool usepointer = false,
                       HocSymExtension* extra = NULL);
void hoc_ivpvaluerun(CChar* name,
                     neuron::container::data_handle<double>,
                     CChar* action,
                     bool deflt = false,
                     bool canrun = false,
                     HocSymExtension* extra = NULL);

extern void hoc_ivlabel(CChar*);
extern void hoc_ivvarlabel(char**, Object* pyvar = 0);
extern void hoc_ivstatebutton(neuron::container::data_handle<double>,
                              CChar* name,
                              CChar* action,
                              int style,
                              Object* pyvar = 0,
                              Object* pyact = 0);
extern void hoc_ivslider(neuron::container::data_handle<double>,
                         float low = 0,
                         float high = 100,
                         float resolution = 1,
                         int steps = 10,
                         const char* send_cmd = NULL,
                         bool vert = false,
                         bool slow = false,
                         Object* pyvar = 0,
                         Object* pyact = 0);

inline double* object_pval(Symbol* sym, Objectdata* od) {
    return od[sym->u.oboff].pval;
}
inline char* object_str(Symbol* sym, Objectdata* od) {
    return *od[sym->u.oboff].ppstr;
}
inline char** object_pstr(Symbol* sym, Objectdata* od) {
    return od[sym->u.oboff].ppstr;
}
inline Object** object_pobj(Symbol* sym, Objectdata* od) {
    return od[sym->u.oboff].pobj;
}
inline hoc_Item** object_psecitm(Symbol* sym, Objectdata* od) {
    return od[sym->u.oboff].psecitm;
}

class Oc2IV {
  public:
    static char** object_pstr(const char* symname, Object* = NULL);
    static char* object_str(const char* symname, Object* = NULL);
};

struct Symlist;

#ifndef OCMATRIX

class ParseTopLevel {
  public:
    ParseTopLevel();
    virtual ~ParseTopLevel();
    void save();
    void restore();

  private:
    Objectdata* obdsav_;
    Object* obsav_;
    Symlist* symsav_;
    bool restored_;
};
#endif

#endif
