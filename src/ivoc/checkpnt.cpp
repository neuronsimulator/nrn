#include <../../nrnconf.h>
#if HAVE_IV  // to end of file

/*

strategy:

symbols
    hoc_built_in_symlist
    hoc_top_level_symlist
    symlists for each template
    object data
linked-function pointers

procedures
    symbol pointers and linked function pointers used here

nrnoc
    mostly data but also pointers in the dparam.
    because of setpointer (eg, pre-synaptic potential, these can
    be pointers to any object data or nrnoc data.


bottom line:
    extremely important to have generic method for pointers to any
    location of an allocated block.

two pass:
    allocate all blocks
    make pointer map
    fill pointers

two kinds of pointers
    ownership	-- responsible for allocating the space
    observer
map
    key address size type

This was my initial thought. The incremental implementation has
evolved considerably with many ad hoc aspects and false starts.

assume usage of compatible executeable. This means the same hoc_built_in_symlist
for writing and reading the checkpoint. For now we save this symlist in the
checkpoint and on reading check to make sure it is the same as the one
in memory. If so I think we have prima facie evidence that the instruction
list is consistent. ie add() -inst_table-> int ---> int -hoc_inst[]-> add().
the hoc_inst[] contains the signature of an instruction that tells what
(if anything)is expected on the execution list.
All other built-in functions are accessed through their symbols.
In fact all symbols are transmitted via
    sym -stable_-> int -----> int -symplist[]-> sym

Here is the format of the checkpoint file (no leading blanks in file):

total number of symbols
hoc_built-in-symlist (in recursive symbol and table format)
hoc_top_level_symlist (in recursive symbol and table format)
instructions for all FUNC/PROC symbols
-1
Objectdata (VAR with NOTUSER subtype, OBJECTVAR, STRING) and local data
-1
ivoc,nrnoc, nrniv stuff (to be implemented)


The recursive symbol and table format is
symtable size
symbolinteger type subtype
symbolname
    if the symbol is of a type that itself has a symbol table then we recurse

instructions for all ... is a -1 terminated list of
symbolinteger
size
instructionintegers mingled with
    signature info consisting of symbolinteger, integer, etc

Objectdata etc is -1 terminated list of
symbolinteger
data depending on type. eg for VAR && NOTUSER it is
    arrayinfo
    values for all elements in the array
  and for OBJECTVAR it is
    arrayinfo
    Objectdata recursion for all elements in the array
    objectref to the objects for all elements in the array

*/

#define HAVE_XDR 0

#include <OS/list.h>
#include <OS/table.h>
#include "oc2iv.h"
#include "ocfunc.h"
#if HAVE_XDR
#include <rpc/xdr.h>
#endif
#include "checkpnt.h"

#include "redef.h"
#include "hoclist.h"
#include "parse.hpp"
#include "code.h"
#include "equation.h"
int hoc_readcheckpoint(char*);
extern int hoc_resize_toplevel(int);

static struct HocInst {
    Pfrv pi;
    const char* signature;
} hoc_inst_[] = {{0, 0},  // 0
                 {nopop, 0},
                 {eval, 0},
                 {add, 0},
                 {hoc_sub, 0},
                 {mul, 0},
                 {hoc_div, 0},
                 {hoc_negate, nullptr},
                 {power, 0},
                 {hoc_assign, nullptr},
                 {bltin, "s"},    // requires change
                 {varpush, "s"},  // 10
                 {constpush, "s"},
                 {pushzero, 0},
                 {print, 0},
                 {varread, "s"},
                 {prexpr, 0},
                 {prstr, 0},
                 {gt, 0},
                 {hoc_lt, nullptr},
                 {hoc_eq, nullptr},  // 20
                 {ge, 0},
                 {le, 0},
                 {ne, 0},
                 {hoc_and, 0},
                 {hoc_or, 0},
                 {hoc_not, 0},
                 {ifcode, "iii"},
                 {forcode, "iii"},
                 {shortfor, "ii"},
                 {call, "si"},  // 30
                 {hoc_arg, "i"},
                 {argassign, "i"},
                 {funcret, 0},
                 {procret, 0},
                 {hoc_stringarg, "i"},
                 {hoc_push_string, "s"},
                 {Break, 0},
                 {Continue, 0},
                 {Stop, 0},
                 {assstr, 0},  // 40
                 {hoc_evalpointer, 0},
                 {hoc_newline, 0},
                 {hoc_delete_symbol, "s"},
                 {hoc_cyclic, 0},
                 {dep_make, 0},
                 {eqn_name, 0},
                 {eqn_init, 0},
                 {eqn_lhs, 0},  // 50
                 {eqn_rhs, 0},
                 {hoc_objectvar, "s"},
                 {hoc_object_component, "siis"},
                 {hoc_object_eval, 0},
                 {hoc_object_asgn, 0},
                 {hoc_objvardecl, "si"},
                 {hoc_cmp_otype, "i"},
                 {hoc_newobj, "si"},
                 {hoc_asgn_obj_to_str, 0},
                 {hoc_known_type, "i"},  // 60
                 {hoc_push_string, "s"},
                 {hoc_objectarg, "i"},
                 {hoc_ob_pointer, 0},
                 {connectsection, 0},
                 {simpleconnectsection, 0},
                 {connectpointer, "s"},
                 {add_section, "si"},
                 {range_const, "s"},
                 {range_interpolate, "s"},
                 {range_interpolate_single, "s"},  // 70
                 {rangevareval, "s"},
                 {rangepoint, "s"},
                 {sec_access, "s"},
                 {ob_sec_access, 0},
                 {mech_access, "s"},
                 {for_segment, "ii"},
                 {sec_access_push, "s"},
                 {sec_access_pop, 0},
                 {forall_section, "ii"},
                 {hoc_ifsec, "ii"},  // 80
                 {hoc_ifseclist, "ii"},
                 {forall_sectionlist, "ii"},
                 {connect_point_process_pointer, 0},
                 {nrn_cppp, 0},
                 {rangevarevalpointer, "s"},
                 {sec_access_object, 0},
                 {mech_uninsert, "s"},
                 {hoc_arayinstal, "i"},
                 {0, 0}};

#define VPfri void*
declareTable(InstTable, VPfri, short)
implementTable(InstTable, VPfri, short)
static InstTable* inst_table_;

declareTable(Symbols, Symbol*, int)
implementTable(Symbols, Symbol*, int)

declareTable(Objects, Object*, int)
implementTable(Objects, Object*, int)

class PortablePointer {
  public:
    PortablePointer();
    PortablePointer(void* address, int type, unsigned long size = 1);
    virtual ~PortablePointer();
    void set(void* address, int type, unsigned long size = 1);
    void size(unsigned long s) {
        size_ = s;
    }
    unsigned long size() {
        return size_;
    }
    void* address() {
        return address_;
    }
    int type() {
        return type_;
    }

  private:
    void* address_;
    int type_;
    unsigned long size_;
};

PortablePointer::PortablePointer() {
    address_ = NULL;
    type_ = 0;
    size_ = 0;
}
PortablePointer::PortablePointer(void* address, int type, unsigned long s) {
    set(address, type, s);
}
void PortablePointer::set(void* address, int type, unsigned long s) {
    address_ = address;
    type_ = type;
    size_ = s;
}
PortablePointer::~PortablePointer() {}

class OcCheckpoint {
  public:
    OcCheckpoint();
    virtual ~OcCheckpoint();

    bool write(const char*);

  private:
    friend class Checkpoint;
    bool pass1();
    bool pass2();
    bool make_sym_table();
    bool build_map();
    PortablePointer* find(void*);

    bool func(Symbol*);

    bool sym_count(Symbol*);
    bool sym_table_install(Symbol*);
    bool sym_out(Symbol*);
    bool sym_instructions(Symbol*);
    bool sym_values(Symbol*);
    bool objects(Symbol*);

    bool symlist(Symlist*);
    bool symbol(Symbol*);
    bool proc(Proc*);
    bool instlist(unsigned long, Inst*);
    bool datum(Datum*);
    bool ctemplate(Symbol*);
    bool object();
    bool objectdata(Objectdata*);
    long arrayinfo(Symbol* s, Objectdata*);
    bool xdr(int&);
    bool xdr(long&);
    bool xdr(char*&, int);
    bool xdr(short&);
    bool xdr(unsigned int&);
    bool xdr(unsigned long&);
    bool xdr(double&);
    bool xdr(Object*&);

  private:
    int cnt_;
    int nobj_;
    Objects* otable_;
    bool (OcCheckpoint::*func_)(Symbol*);
    Symbols* stable_;
#if HAVE_XDR
    XDR xdrs_;
#endif
    Objectdata* objectdata_;
};

class OcReadChkPnt {
  public:
    OcReadChkPnt();
    virtual ~OcReadChkPnt();

    bool read();

  private:
    friend class Checkpoint;
    bool symbols();
    bool symtable();
    bool symbol();
    bool instructions();
    bool objects();
    bool objectdata();

    long arrayinfo(Symbol* s, Objectdata*);

    bool get(int&);
    bool get(long&);
    bool get(char*&);
    bool get(double&);
    bool get(Object*&);

  private:
    bool lookup_;
    int builtin_size_;
    int lineno_;
    int id_;
    int nsym_;
    Symbol** psym_;
    long nobj_;
    Object** pobj_;
    Symlist* symtable_;
};

static bool out_;
static FILE* f_;
static OcCheckpoint* cp_;
static OcReadChkPnt* rdckpt_;
static Checkpoint* ckpt_;

Checkpoint* Checkpoint::instance() {
    if (!ckpt_) {
        ckpt_ = new Checkpoint();
    }
    return ckpt_;
}

Checkpoint::Checkpoint() {}
Checkpoint::~Checkpoint() {}
bool Checkpoint::out() {
    return out_;
}
bool Checkpoint::xdr(long& i) {
    if (out()) {
        return cp_->xdr(i);
    } else {
        return rdckpt_->get(i);
    }
}
bool Checkpoint::xdr(Object*& o) {
    if (out()) {
        return cp_->xdr(o);
    } else {
        return rdckpt_->get(o);
    }
}

void hoc_checkpoint() {
    if (!cp_) {
        cp_ = new OcCheckpoint();
    }
    bool b;
    b = cp_->write(gargstr(1));
    hoc_ret();
    hoc_pushx(double(b));
}

int hoc_readcheckpoint(char* fname) {
    f_ = fopen(fname, "r");
    if (!f_) {
        return 0;
    }
    char buf[256];
    if (fgets(buf, 256, f_) == 0) {
        printf("checkpoint read from file %s failed.\n", fname);
        return 2;
    }
    if (strcmp(buf, "NEURON Checkpoint\n") != 0) {
        fclose(f_);
        return 0;
    }
    rdckpt_ = new OcReadChkPnt();
    bool b;
    b = rdckpt_->read();
    int rval = 1;
    if (!b) {
        printf("checkpoint read from file %s failed.\n", fname);
        rval = 2;
    }
    delete rdckpt_;
    rdckpt_ = NULL;
    return rval;
}

OcCheckpoint::OcCheckpoint() {
    func_ = NULL;
    stable_ = NULL;
    otable_ = NULL;
    if (!inst_table_) {
        short i;
        for (i = 1; hoc_inst_[i].pi; ++i) {
            ;
        }
        inst_table_ = new InstTable(2 * i);
        for (i = 1; hoc_inst_[i].pi; ++i) {
            inst_table_->insert((VPfri) hoc_inst_[i].pi, i);
        }
    }
}

OcCheckpoint::~OcCheckpoint() {
    if (stable_) {
        delete stable_;
    }
    if (otable_) {
        delete otable_;
    }
}

#if HAVE_XDR
#define USEXDR 1
#else
#define USEXDR 0
#endif

#undef DEBUG

#if USEXDR
#define DEBUG \
    if (0)    \
    fprintf
#else
#define DEBUG fprintf
#endif

bool OcCheckpoint::write(const char* fname) {
    bool b;
    int i;
    out_ = 1;
    f_ = fopen(fname, "w");
    if (!f_) {
        return false;
    }
    fprintf(f_, "NEURON Checkpoint\n");
#if USEXDR
    xdrstdio_create(&xdrs_, f_, XDR_ENCODE);
#endif
    b = make_sym_table();
    func_ = &OcCheckpoint::sym_out;
    b = (b && pass1());
    func_ = &OcCheckpoint::sym_instructions;
    b = (b && pass1());
    i = -1;
    b = (b && xdr(i));

    // printf("nobj_ = %d\n", nobj_);
    b = b && object();

    func_ = &OcCheckpoint::sym_values;
    objectdata_ = hoc_top_level_data;
    int size = hoc_resize_toplevel(0);
    b = b && xdr(size);
    i = 0;
    b = b && xdr(i);
    b = b && pass1();
    i = -1;
    b = (b && xdr(i));
#if USEXDR
    xdr_destroy(&xdrs_);
#endif
    fclose(f_);
    return b;
}

bool OcCheckpoint::make_sym_table() {
    bool b;
    cnt_ = 1;
    nobj_ = 0;
    func_ = &OcCheckpoint::sym_count;
    b = pass1();
    if (!b) {
        printf("make_sym_table failed on first pass1\n");
    }
    DEBUG(f_, "#symbols=%d\n", cnt_);
    b = (b && xdr(cnt_));
    if (stable_) {
        delete stable_;
    }
    stable_ = new Symbols(2 * cnt_);
    cnt_ = 1;
    func_ = &OcCheckpoint::sym_table_install;
    if (!b) {
        printf("make_sym_table failed before second pass1\n");
    }
    b = (b && pass1());
    if (!b) {
        printf("make_sym_table failed on second pass1\n");
    }
    func_ = NULL;
    return b;
}

bool OcCheckpoint::sym_count(Symbol* s) {
    ++cnt_;
    if (s->type == TEMPLATE) {
        nobj_ += s->u.ctemplate->count;
    }
    return true;
}

bool OcCheckpoint::sym_table_install(Symbol* s) {
    stable_->insert(s, cnt_);
    ++cnt_;
    return true;
}
bool OcCheckpoint::sym_out(Symbol* s) {
    int val;
    stable_->find(val, s);
    DEBUG(f_, "%d %s %d %d\n", val, s->name, s->type, s->subtype);
    int l1 = strlen(s->name);
    bool b = xdr(val) && xdr(s->name, l1) && xdr(s->type) && xdr(s->subtype) && xdr(s->cpublic) &&
             xdr(s->s_varn) && xdr(s->defined_on_the_fly);
    switch (s->type) {
    case VAR:
        if (s->subtype == NOTUSER) {
            b = b && xdr(s->u.oboff);
        }
        arrayinfo(s, NULL);
        break;
    case STRING:
    case OBJECTVAR:
        b = b && xdr(s->u.oboff);
        arrayinfo(s, NULL);
        break;
    case TEMPLATE: {
        cTemplate* t = s->u.ctemplate;
        if (!t->constructor) {
            b = b && xdr(t->dataspace_size) && xdr(t->id);
        }
        break;
    }
    case NUMBER:
        b = b && xdr(*s->u.pnum);
        break;
    case CSTRING:
        b = b && xdr(s->u.cstr, strlen(s->u.cstr));
        break;
    }
    if (!b) {
        printf("failed in sym_table_install\n");
    }
    return b;
}

bool OcCheckpoint::pass1() {
    return symlist(hoc_built_in_symlist) && symlist(hoc_top_level_symlist);
}
bool OcCheckpoint::pass2() {
    return false;
}
bool OcCheckpoint::build_map() {
    return false;
}
PortablePointer* find(void*) {
    return NULL;
}
bool OcCheckpoint::func(Symbol* s) {
    if (func_) {
        return (this->*func_)(s);
    }
    return true;
}
bool OcCheckpoint::symlist(Symlist* sl) {
    if (func_ == &OcCheckpoint::sym_out) {
        int i = 0;
        if (sl)
            for (Symbol* s = sl->first; s; s = s->next) {
                ++i;
            }
        DEBUG(f_, "symboltable size %d\n", i);
        if (!xdr(i)) {
            return false;
        }
    }
    if (sl)
        for (Symbol* s = sl->first; s; s = s->next) {
            if (!symbol(s)) {
                printf("symlist failed\n");
                return false;
            }
        }
    return true;
}
bool OcCheckpoint::symbol(Symbol* s) {
    if (!func(s)) {
        return false;
    }
    bool b = true;
    switch (s->type) {
    case TEMPLATE:
        b = ctemplate(s);
        break;
    case FUNCTION:
    case PROCEDURE:
        b = symlist(s->u.u_proc->list);
        break;
    }
    if (!b)
        printf("symbol failed\n");
    return b;
}
bool OcCheckpoint::sym_instructions(Symbol* s) {
    Proc* p = s->u.u_proc;
    if (s->type == FUNCTION || s->type == PROCEDURE) {
        int val;
        if (!stable_->find(val, s)) {
            printf("couldn't find %s in table\n", s->name);
            return false;
        }
        if (p->size) {
            DEBUG(f_, "instructions for %d |%s|\n", val, s->name);
            DEBUG(f_, "size %lu\n", p->size);
            bool b = xdr(val) && xdr(p->size);
            if (!b) {
                printf("failed in sym_intructions\n");
                return false;
            }
            b = instlist(p->size, p->defn.in);
            if (!b) {
                printf("instlist failed for %s\n", s->name);
            }
            return b;
        }
    }
    return true;
}
bool OcCheckpoint::instlist(unsigned long size, Inst* in) {
    for (unsigned long i = 0; i < size; ++i) {
        short val;
        int sval;
        if (in[i].in == STOP) {
            DEBUG(f_, "  STOP\n");
            val = 0;
            if (!xdr(val)) {
                printf("instlist failed 1\n");
                return false;
            }
            continue;
        }
        if (inst_table_->find(val, (VPfri) in[i].pf)) {
            DEBUG(f_, "  %d\n", val);
            if (!xdr(val)) {
                printf("instlist failed 2\n");
                return false;
            }
            const char* s = hoc_inst_[val].signature;
            for (int j = 0; s && s[j]; ++j) {
                ++i;
                switch (s[j]) {
                case 's':
                    if (in[i].sym) {
                        if (!stable_->find(sval, in[i].sym)) {
                            printf("couldn't find |%s| in table at instruction index %ld\n",
                                   in[i].sym->name,
                                   i);
                            return false;
                        }
                        // DEBUG(f_, "    %d |%s|\n", sval, in[i].sym->name);
                        if (!xdr(sval)) {
                            printf("instlist failed 3\n");
                            return false;
                        }
                    } else {
                        DEBUG(f_, "    0 SYMBOL0\n");
                        sval = 0;
                        if (!xdr(sval)) {
                            printf("instlist failed 4\n");
                            return false;
                        }
                    }
                    break;
                case 'i':
                    DEBUG(f_, "    %i\n", in[i].i);
                    if (!xdr(in[i].i)) {
                        printf("instlist failed 5\n");
                        return false;
                    }
                    break;
                }
            }
        } else {
            printf("OcCheckpoint::instlist failed at i = %lu\n", i);
            return false;
        }
    }
    return true;
}
bool OcCheckpoint::datum(Datum*) {
    return false;
}
bool OcCheckpoint::ctemplate(Symbol* s) {
    cTemplate* t = s->u.ctemplate;
    if (func_ == &OcCheckpoint::sym_values) {
        Objectdata* saveod = objectdata_;
        int ti;
        bool b;
        b = stable_->find(ti, s);
        DEBUG(f_, "%d %d %s\n", ti, t->count, s->name);
        b = b && xdr(ti);
        //		b = b && xdr(t->count);
        hoc_Item* q;
        ITERATE(q, t->olist) {
            Object* ob = OBJ(q);
            int oid;
            b = b && otable_->find(oid, ob);
            b = b && xdr(oid);
            if (t->constructor) {
                if (t->checkpoint) {
                    b = b && (t->checkpoint)(&ob->u.this_pointer);
                } else {
                    printf("No checkpoint available for %s\n", hoc_object_name(ob));
                    b = false;
                }
            } else {
                objectdata_ = ob->u.dataspace;
                b = b && symlist(t->symtable);
            }
            if (!b) {
                break;
            }
        }
        objectdata_ = saveod;
        return b;
    } else {
        return symlist(t->symtable);
    }
}
bool OcCheckpoint::object() {
    bool b;
    int i;
    if (otable_) {
        delete otable_;
    }
    b = xdr(nobj_);
    otable_ = new Objects(2 * nobj_ + 1);
    nobj_ = 0;
    func_ = &OcCheckpoint::objects;
    b = pass1();
    i = -1;
    b = b && xdr(i);
    return b;
}
bool OcCheckpoint::objects(Symbol* s) {
    bool b = true;
    if (s->type == TEMPLATE) {
        int sid;
        b = b && stable_->find(sid, s);
        b = b && xdr(sid);
        cTemplate* t = s->u.ctemplate;
#undef init
        if (t->init) {
            b = b && stable_->find(sid, t->init);
        } else {
            sid = 0;
        }
        b = b && xdr(sid);
        b = b && xdr(t->index) && xdr(t->count) && xdr(t->id);
        hoc_Item* q;
        ITERATE(q, t->olist) {
            Object* ob = OBJ(q);
            ++nobj_;
            otable_->insert(ob, nobj_);  // 0 is null object
            b = b && xdr(nobj_) && xdr(ob->refcount) && xdr(ob->index);
        }
    }
    return b;
}

bool OcCheckpoint::objectdata(Objectdata*) {
    return false;
}
#undef sub
long OcCheckpoint::arrayinfo(Symbol* s, Objectdata* od) {
    Arrayinfo* ao;
    Arrayinfo* as;

    as = s->arayinfo;
    if (od) {
        ao = od[s->u.oboff + 1].arayinfo;
    } else {
        ao = as;
    }
    long n = long(hoc_total_array_data(s, od));
    if (!as) {  // not an array
        DEBUG(f_, "0\n");
        int i = 0;
        xdr(i);
    } else if (od && as == ao) {  // same as original
        DEBUG(f_, "-1\n");
        int i = -1;
        xdr(i);
    } else {
        int v = ao->a_varn ? 1 : 0;
        DEBUG(f_, "%d %d %d", ao->nsub, ao->refcount, v);
        if (v) {
            printf("checkpoint of equation array vars not implemented: %s\n", s->name);
            return -1;
        }
        xdr(ao->nsub);
        //		xdr(ao->refcount);
        //		xdr(v);
        for (int i = 0; i < ao->nsub; ++i) {
            DEBUG(f_, " %d", ao->sub[i]);
            xdr(ao->sub[i]);
        }
        DEBUG(f_, "\n");
    }
    return n;
}

bool OcCheckpoint::proc(Proc*) {
    return false;
}

bool OcCheckpoint::sym_values(Symbol* s) {
    int sp;
    bool b;
    stable_->find(sp, s);
    if ((s->type == VAR && s->subtype == NOTUSER) || s->type == OBJECTVAR || s->type == STRING ||
        s->type == SECTION) {
        DEBUG(f_, "%d %s\n", sp, s->name);
        b = xdr(sp);
        long n = arrayinfo(s, objectdata_);
        if (n == -1) {
            return false;
        }
        //		DEBUG(f_, " %ld\n", n);
        //		b = b && xdr(n);
        for (long i = 0; i < n; ++i) {
            Objectdata od = objectdata_[s->u.oboff];
            if (s->type == VAR) {
                double d = od.pval[i];
                DEBUG(f_, "  %g\n", d);
                b = b && xdr(d);
            } else if (s->type == OBJECTVAR) {
                Object* ob = od.pobj[i];
                if (ob == NULL) {
                    DEBUG(f_, "  0\n");
                    int i = 0;
                    b = b && xdr(i);
                } else {
#if 0
					int t;
					stable_->find(t, ob->ctemplate->sym);
					DEBUG(f_, "  %d %d %s\n", t, ob->index,
					  ob->ctemplate->sym->name);
					b = b && xdr(t);
					b = b && xdr(ob->index);
#else
                    int oid;
                    b = b && otable_->find(oid, ob);
                    b = b && xdr(oid);
#endif
                }
            } else if (s->type == STRING) {
                char* cp = od.ppstr[i];
                DEBUG(f_, " |%s|\n", cp);
                b = b && xdr(cp, strlen(cp));
                //			}else if (s->type == SECTION) {
            }
        }
    }
    return true;
}

#if USEXDR && 0
bool OcCheckpoint::xdr(int& i) {
    return xdr_int(&xdrs_, &i);
}
bool OcCheckpoint::xdr(char*& i, int size) {
    return xdr_string(&xdrs_, &i, size);
}
bool OcCheckpoint::xdr(short& i) {
    return xdr_short(&xdrs_, &i);
}
bool OcCheckpoint::xdr(unsigned int& i) {
    return xdr_u_int(&xdrs_, &i);
}
bool OcCheckpoint::xdr(unsigned long& i) {
    return xdr_u_long(&xdrs_, &i);
}
#else
bool OcCheckpoint::xdr(int& i) {
    fprintf(f_, "%d\n", i);
    return true;
}
bool OcCheckpoint::xdr(long& i) {
    fprintf(f_, "%ld\n", i);
    return true;
}
bool OcCheckpoint::xdr(char*& s, int) {
    fprintf(f_, "%s\n", s);
    return true;
}
bool OcCheckpoint::xdr(short& i) {
    int j = i;
    fprintf(f_, "%d\n", j);
    return true;
}
bool OcCheckpoint::xdr(unsigned int& i) {
    int j = i;
    fprintf(f_, "%d\n", j);
    return true;
}
bool OcCheckpoint::xdr(unsigned long& i) {
    long j = i;
    fprintf(f_, "%ld\n", j);
    return true;
}
bool OcCheckpoint::xdr(double& i) {
    fprintf(f_, "%g\n", i);
    return true;
}
#endif
bool OcCheckpoint::xdr(Object*& o) {
    int i;
    bool b;
    b = otable_->find(i, o);
    b = b && xdr(i);
    return b;
}

#undef Chk
#define Chk(arg1, arg2)                        \
    if (!(arg1)) {                             \
        printf("%s line %d\n", arg2, lineno_); \
        return false;                          \
    }
#undef Get
#define Get(arg)      \
    if (!get(arg)) {  \
        return false; \
    }

OcReadChkPnt::OcReadChkPnt() {
    builtin_size_ = hoc_resize_toplevel(0);
}

OcReadChkPnt::~OcReadChkPnt() {
    delete[] psym_;
    delete[] pobj_;
}

bool OcReadChkPnt::read() {
    int size;
    out_ = 0;
    lineno_ = 1;
    id_ = 1;
    Chk(symbols(), "OcReadChkPnt::symbols() read failure");
    printf("finished with symbols at lineno = %d\n", lineno_);
    Chk(instructions(), "OcReadChkPnt::instructions() read failure");
    printf("finished with instructions at lineno = %d\n", lineno_);
    Chk(objects(), "OcReadChkPnt::objects() read failure");
    printf("finished with objects at lineno = %d\n", lineno_);
    Get(size);
    if (size != hoc_resize_toplevel(size - builtin_size_)) {
        printf("top_level_data not right size\n");
        return false;
    }
    Chk(objectdata(), "OcReadChkPnt::objectdata() read failure");
    printf("finished with objectdata at lineno = %d\n", lineno_);
    return true;
}

bool OcReadChkPnt::symbols() {
    Get(nsym_);
    psym_ = new Symbol*[nsym_];
    for (int i = 0; i < nsym_; ++i) {
        psym_[i] = 0;
    }
    lookup_ = true;
    symtable_ = hoc_built_in_symlist;
    Chk(symtable(), "built_in_symlist failure");
    lookup_ = false;
    symtable_ = hoc_top_level_symlist;
    if (symtable_->first != NULL) {
        printf("Some user symbols are already defined at the top level\n");
        return false;
    }
    Chk(symtable(), "top_level_symlist failure");
    return true;
}
bool OcReadChkPnt::symtable() {
    int size;
    Get(size);
    for (int i = 0; i < size; ++i) {
        Chk(symbol(), "symbol read failure");
    }
    return true;
}
bool OcReadChkPnt::symbol() {
    int id, type, subtype, i;
    char buf[2048], *name = buf;

    Get(id);
    if (id != id_) {
        printf("expected symbol id = %d but file id was %d\n", id_, id);
        return false;
    }
    ++id_;
    Get(name);
    Get(type);
    Get(subtype);

    Symbol* sym;
    if (lookup_) {
        sym = hoc_table_lookup(name, symtable_);
        if (!sym || sym->type != type || sym->subtype != subtype) {
            printf("%s not a built-in\n", name);
            return false;
        }
    } else {
        sym = hoc_install(name, (type == VAR) ? UNDEF : type, 0.0, &symtable_);
        sym->type = type;
        sym->subtype = subtype;
    }

    psym_[id] = sym;
    Get(i);
    sym->cpublic = i;
    Get(i);
    sym->s_varn = i;
    Get(i);
    sym->defined_on_the_fly = i;
    switch (type) {
    case VAR: {
        if (subtype == NOTUSER) {
            Get(i);
            if (lookup_ && i != sym->u.oboff) {
                printf("bad u.oboff field for built-in VAR\n");
                return false;
            } else {
                sym->u.oboff = i;
            }
        }
        arrayinfo(sym, NULL);
    } break;
    case OBJECTVAR:
    case STRING:
        Get(i);
        sym->u.oboff = i;
        arrayinfo(sym, NULL);
        break;
    case CSTRING:
        sym->u.cstr = NULL;
        Get(sym->u.cstr);
        break;
    case NUMBER:
        sym->u.pnum = new double;
        Get(*sym->u.pnum);
        break;
    case TEMPLATE: {
        cTemplate* t;
        int dsize, id;
        Symlist* slsave = symtable_;
        if (!lookup_) {
            Get(dsize);
            Get(id);
            t = new cTemplate;
            sym->u.ctemplate = t;
            t->sym = sym;
            t->dataspace_size = dsize;
            t->constructor = 0;
            t->destructor = 0;
            t->steer = 0;
            t->id = id;
            symtable_ = NULL;
            Chk(symtable(), "");
            t->symtable = symtable_;
        } else {
            // these don't have a dataspace
            t = sym->u.ctemplate;
            symtable_ = t->symtable;
            Chk(symtable(), "");
        }
        symtable_ = slsave;
    } break;
    case FUNCTION:
    case PROCEDURE: {
        Symlist* slsave = symtable_;
        symtable_ = sym->u.u_proc->list;
        Chk(symtable(), "");
        sym->u.u_proc->list = symtable_;
        symtable_ = slsave;
    } break;
    }
    return true;
}
bool OcReadChkPnt::instructions() {
    int sid, size, i, iid;
    Symbol* sym;
    const char* signature;
    for (;;) {
        Get(sid);
        if (sid == -1) {
            break;
        }
        sym = psym_[sid];
        if (!sym || (sym->type != FUNCTION && sym->type != PROCEDURE)) {
            printf("not a PROC or FUNC\n");
            return false;
        }
        Get(size);
        sym->u.u_proc->size = size;
        Inst* lin;
        sym->u.u_proc->defn.in = lin = new Inst[size];
        for (i = 0; i < size;) {
            Get(iid);
            lin[i++].pf = hoc_inst_[iid].pi;
            signature = hoc_inst_[iid].signature;
            if (signature)
                for (const char* cp = signature; *cp; ++cp) {
                    Get(iid);
                    switch (*cp) {
                    case 's':
                        lin[i++].sym = psym_[iid];
                        break;
                    case 'i':
                        lin[i++].i = iid;
                        break;
                    }
                }
        }
    }

    return true;
}
bool OcReadChkPnt::objects() {
    int sid;
    long i, n, iob = 0;
    Symbol* sym;
    Get(nobj_);
    pobj_ = new Object*[nobj_ + 1];
    pobj_[0] = NULL;
    for (;;) {
        Get(sid);
        if (sid == -1) {
            break;
        }
        sym = psym_[sid];
        if (sym->type != TEMPLATE) {
            printf("not a template\n");
            return false;
        }
        cTemplate* t = sym->u.ctemplate;
        Get(sid);
#undef init
        t->init = psym_[sid];
        Get(t->index);
        Get(n);
        t->count = int(n);
        if (t->constructor && !t->checkpoint && t->count > 0) {
            printf("Objects for a built-in template without checkpoint: %s\n", sym->name);
            return false;
        }
        t->olist = hoc_l_newlist();
        Get(t->id);
        for (i = 0; i < n; ++i) {
            int fiob;  // not really needed
            Get(fiob);
            Object* pob = new Object;
            pobj_[++iob] = pob;
            if (fiob != iob) {
                printf("object indexes out of sync\n");
            }
            pob->itm_me = hoc_l_lappendobj(t->olist, pob);
            pob->ctemplate = t;
            Get(pob->refcount);
            Get(pob->index);
            if (t->constructor) {
                // have to set this up later
                pob->u.this_pointer = NULL;
            } else {
                pob->u.dataspace = new Objectdata[t->dataspace_size];
            }
        }
    }
    if (iob != nobj_) {
        printf("objects read != objects expected\n");
        return false;
    }
    return true;
}
bool OcReadChkPnt::objectdata() {
    int sid;
    long n, i;
    Symbol* sym;
    double d;
    int oid;
    Objectdata *od, *odp;
    Get(oid);
    if (oid == 0) {
        od = hoc_top_level_data;
    } else if (oid == -1) {
        return true;
    } else {
        od = pobj_[oid]->u.dataspace;
    }
    for (;;) {
        Get(sid);
        if (sid == -1) {
            break;
        }
        sym = psym_[sid];
        switch (sym->type) {
        case VAR:
            n = arrayinfo(sym, od);
#if 0
			Get(i);	//not necessary but if present don't forget others
			if (i != n) {
				printf("inconsistent array size %d %d\n", i, n);
				returnf false;
			}
#endif
            odp = od + sym->u.oboff;
            if (od != hoc_top_level_data || builtin_size_ <= sym->u.oboff) {
                odp->pval = new double[n];
            }
            for (i = 0; i < n; ++i) {
                Get(d);
                odp->pval[i] = d;
            }
            break;
        case OBJECTVAR:
            n = arrayinfo(sym, od);
            odp = od + sym->u.oboff;
            odp->pobj = new Object*[n];
            for (i = 0; i < n; ++i) {
                int iob;
                Get(iob);
                odp->pobj[i] = pobj_[iob];
            }
            break;
        case STRING:
            n = arrayinfo(sym, od);
            od[sym->u.oboff].ppstr = new char*[n];
            for (i = 0; i < n; ++i) {
                od[sym->u.oboff].ppstr[i] = NULL;
                Get(od[sym->u.oboff].ppstr[i]);
            }
            break;
        case TEMPLATE: {
            cTemplate* t = sym->u.ctemplate;
            if (t->constructor) {
                for (long j = 0; j < t->count; ++j) {
                    int iob;
                    Object* ob;
                    Get(iob);
                    ob = pobj_[iob];
                    if (!(*t->checkpoint)(&ob->u.this_pointer)) {
                        printf("failed reading data for %s\n", hoc_object_name(ob));
                        return false;
                    }
                }
            } else {
                for (long j = 0; j < t->count; ++j) {
                    objectdata();
                }
            }
        } break;
        default:
            return false;
        }
    }
    return true;
}
long OcReadChkPnt::arrayinfo(Symbol* s, Objectdata* od) {
    int nsub;
    Get(nsub);
    if (lookup_) {
        int i;
        for (i = 0; i < nsub; ++i) {
            Get(i);
        }
    }
    Arrayinfo** ap;
    if (od == NULL) {
        ap = &s->arayinfo;
    } else {
        ap = &od[s->u.oboff + 1].arayinfo;
    }
    if (nsub == 0) {
        *ap = NULL;
        return 1;
    }
    if (nsub == -1) {
        *ap = s->arayinfo;
        if (*ap) {
            (*ap)->refcount++;
        }
        return long(hoc_total_array_data(s, NULL));
    }
    Arrayinfo* a = (Arrayinfo*) hoc_Emalloc(sizeof(Arrayinfo) + nsub * sizeof(int));
    if (!a) {
        return -1;
    }
    *ap = a;
    a->refcount = 1;
    a->a_varn = NULL;
    a->nsub = nsub;

    long n = 1;
    for (int i = 0; i < nsub; ++i) {
        int sub;
        Get(sub);
        a->sub[i] = sub;
        n *= sub;
    }
    return n;
}
bool OcReadChkPnt::get(int& i) {
    ++lineno_;
    char buf[200];
    if (!fgets(buf, 200, f_) || (sscanf(buf, "%d", &i) != 1)) {
        printf("error reading integer at line %d\n", lineno_);
        return false;
    }

    return true;
}
bool OcReadChkPnt::get(double& i) {
    ++lineno_;
    char buf[200];
    if (!fgets(buf, 200, f_) || (sscanf(buf, "%lf", &i) != 1)) {
        printf("error reading double at line %d\n", lineno_);
        return false;
    }

    return true;
}
bool OcReadChkPnt::get(long& i) {
    int j;
    Get(j);
    i = j;
    return true;
}

bool OcReadChkPnt::get(char*& s) {
    ++lineno_;
    if (s) {
        if (!fgets(s, 2048, f_)) {
            printf("error reading string at line %d\n", lineno_);
            return false;
        }
        s[strlen(s) - 1] = '\0';
    } else {
        char buf[256];
        if (!fgets(buf, 256, f_)) {
            printf("error reading string at line %d\n", lineno_);
            return false;
        }
        buf[strlen(buf) - 1] = '\0';
        s = new char[strlen(buf) + 1];
        strcpy(s, buf);
    }
    return true;
}


bool OcReadChkPnt::get(Object*& o) {
    long i;
    Get(i);
    o = pobj_[i];
    return true;
}
#endif
