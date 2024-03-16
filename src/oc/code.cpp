#include <../../nrnconf.h>
/* /local/src/master/nrn/src/oc/code.cpp,v 1.37 1999/07/03 14:20:21 hines Exp */

#include "backtrace_utils.h"
#include <errno.h>
#include "hoc.h"
#include "code.h"
#include "hocstr.h"
#include "parse.hpp"
#include "ocfunc.h"
#include "ocmisc.h"
#include "oc_ansi.h"
#include "hocparse.h"
#include "equation.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <nrnmpi.h>
#include "nrnfilewrap.h"
#include "utils/enumerate.h"


#include "options.h"
#include "section.h"

#include <vector>
#include <variant>
#include <sstream>


int bbs_poll_;
extern void bbs_handle();
#define BBSPOLL             \
    if (--bbs_poll_ == 0) { \
        bbs_handle();       \
    }

int nrn_isecstack();

extern void debugzz(Inst*);
int hoc_return_type_code = 0; /* flag for allowing integers (1) and booleans (2) to be recognized as
                                 such */

// array indices on the stack have their own type to help with determining when
// a compiled fragment of HOC code is processing a variable whose number of
// dimensions was changed.
struct stack_ndim_datum {
    stack_ndim_datum(int ndim) {
        i = ndim;
    }
    int i;
};
std::ostream& operator<<(std::ostream& os, const stack_ndim_datum& d) {
    os << d.i;
    return os;
}

using StackDatum = std::variant<double,
                                Symbol*,
                                int,
                                stack_ndim_datum,
                                Object**,
                                Object*,
                                char**,
                                neuron::container::generic_data_handle,
                                std::nullptr_t>;

/** @brief The stack.
 *
 *  It would be nice to use std::stack, but it seems that for now its API is too
 *  restrictive.
 */
static std::vector<StackDatum> stack{};

#define NPROG 50000
Inst* prog;               /* the machine */
Inst* progp;              /* next free spot for code generation */
Inst* pc;                 /* program counter during execution */
Inst* progbase;           /* start of current subprogram */
Inst* prog_parse_recover; /* start after parse error */
int hoc_returning;        /* 1 if return stmt seen, 2 if break, 3 if continue */
/* 4 if stop */
namespace nrn::oc {
struct frame {             /* proc/func call stack frame */
    Symbol* sp;            /* symbol table entry */
    Inst* retpc;           /* where to resume after return */
    StackDatum* argn;      /* n-th argument on stack */
    int nargs;             /* number of arguments */
    Inst* iter_stmt_begin; /* Iterator statement starts here */
    Object* iter_stmt_ob;  /* context of Iterator statement */
    Object* ob;            /* for stack frame debug message */
};
}  // namespace nrn::oc
using Frame = nrn::oc::frame;
#define NFRAME 512                    /* default size */
static Frame *frame, *fp, *framelast; /* first, frame pointer, last */

/* temporary object references come from this pool. This allows the
stack to be aware if it is storing a temporary. We are trying to
solve problems of objrefs on the stack changing the object they point
to and also a failure of garbage collection since temporary objrefs have
not, in the past, been reffed or unreffed.
The first problem is easily solved without much efficiency loss
by having the stack store the object pointer instead of the objref pointer.

Garbage collection is implemented by reffing any object that is placed
on the stack via hoc_push_object (and thus borrows the use of the
type OBJECTTMP) It is then the responsibility of everything that
pops an object to determine whether the object should be unreffed.
This is also done on error recovery and when the stack frame is popped.
I hate the efficiency loss but it is not as bad as it could be
since most popping occurs when the stack frame is popped and in this
case it is faster to check for OBJECTTMP than if the returned Object**
is from the pool.
*/
#define DEBUG_GARBAGE  1
#define TOBJ_POOL_SIZE 50
static Object** hoc_temp_obj_pool_;
static int obj_pool_index_;
static int tobj_count; /* how many stack pushes of OBJECTTMP have been reffed*/

/*
Here is the old comment on the function when it was in hoc_oop.cpp.

At this time we are dealing uniformly with object variables and cannot
deal cleanly with objects.  Eventually it may be possible to put an
object pointer on the stack but not now.  Try to avoid using "functions
which return new objects" as arguments to other functions.  If this is
done then it may happen that when the stack pointer is finally used it
may point to a different object than when it was put on the stack.
Things are safe when a temp_objvar is finally removed from the stack.
Memory leakage will occur if a temp_objvar is passed as an arg but never
assigned to a full fledged object variable.  ie its reference count is 0
but unref will never be called on it.
The danger is illustrated with
    proc p(obj.func_returning_object()) { // $o1 is on the stack
        print $o1	// correct object
        for i=0,100 {
            o = obj.func_returning_different_object()
            print i, $o1  //when i=50 $o1 will be different
        }
    }
In this case one should first assign $o1 to another object variable
and then use that object variable exclusively instead of $o1.
This also prevent leakage of the object pointed to by $o1.

If this ever becomes a problem then it is not too difficult to
implement objects on the stack with garbage collection.
*/

Object** hoc_temp_objptr(Object* obj) {
    obj_pool_index_ = (obj_pool_index_ + 1) % TOBJ_POOL_SIZE;
    Object** const tobj{hoc_temp_obj_pool_ + obj_pool_index_};
    *tobj = obj;
    return tobj;
}

/* should be called after finished with pointer from a popobj */

void hoc_tobj_unref(Object** p) {
    if (p >= hoc_temp_obj_pool_ && p < hoc_temp_obj_pool_ + TOBJ_POOL_SIZE) {
        --tobj_count;
        hoc_obj_unref(*p);
    }
}

/*
vec.c.x[0] used freed memory because the temporary vector was unreffed
after the x pointer was put on the stack but before it was evaluated.
The hoc_pop_defer replaces the nopop in in hoc_object_component handling
of a cplus steer method (which pushes a double pointer). The corresponding
hoc_unref_defer takes place in hoc_object_eval after evaluating
the pointer. This should take care of the most common (itself very rare)
problem. However it still would not in general
take care of the purposeless passing
of &vec.c.x[0] as an argument to a function since intervening pop_defer/unref_defer
pairs could take place.
*/
static Object* unref_defer_;

void hoc_unref_defer() {
    if (unref_defer_) {
#if 0
        printf("hoc_unref_defer %s %d\n", hoc_object_name(unref_defer_), unref_defer_->refcount);
#endif
        hoc_obj_unref(unref_defer_);
        unref_defer_ = nullptr;
    }
}

namespace {
/** @brief Return the (variant) value at depth i in the stack.
 *
 *  i = 0 is the top/most-recently-pushed element of the stack.
 */
StackDatum& get_stack_entry_variant(std::size_t i) {
    assert(!stack.empty());
    return *std::next(stack.rbegin(), i);
}

template <class... Ts>
struct overloaded: Ts... {
    using Ts::operator()...;
};
template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

template <typename T>
[[noreturn]] void report_type_mismatch(StackDatum const& entry) {
    std::visit(
        [](auto const& val) {
            assert((!std::is_same_v<std::decay_t<decltype(val)>, T>) );
            std::ostringstream oss;
            oss << "bad stack access: expecting " << cxx_demangle(typeid(T).name()) << "; really "
                << cxx_demangle(typeid(decltype(val)).name());
            if constexpr (!std::is_same_v<std::decay_t<decltype(val)>, std::nullptr_t>) {
                oss << ' ' << val;
            }
            // extra info for some types
            overloaded{[&oss](Object* o) { oss << " -> " << hoc_object_name(o); },
                       [&oss](Object** o) {
                           if (o) {
                               oss << " -> " << hoc_object_name(*o);
                           }
                       },
                       [&oss](Symbol* sym) {
                           if (sym) {
                               oss << " -> " << sym->name;
                           }
                       },
                       [&oss](stack_ndim_datum& d) { oss << " -> ndim " << d.i; },
                       [&oss](std::nullptr_t) { oss << " already unreffed on stack"; },
                       [](auto const&) {}}(val);
            hoc_execerror(oss.str().c_str(), nullptr);
        },
        entry);
    throw std::logic_error("report_type_mismatch");
}

template <typename T>
void check_type(StackDatum const& entry) {
    if (!std::holds_alternative<T>(entry)) {
        report_type_mismatch<T>(entry);
    }
}

template <typename T>
T& cast(StackDatum& entry) {
    check_type<T>(entry);
    return std::get<T>(entry);
}
template <typename T>
T const& cast(StackDatum const& entry) {
    check_type<T>(entry);
    return std::get<T>(entry);
}
}  // namespace

// Defined and explicitly instantiated to keep the StackDatum type private to this file.
template <typename T>
T const& hoc_look_inside_stack(int i) {
    return cast<T>(get_stack_entry_variant(i));
}
template double const& hoc_look_inside_stack(int);
template Symbol* const& hoc_look_inside_stack(int);
template Object** const& hoc_look_inside_stack(int);
template Object* const& hoc_look_inside_stack(int);
namespace {
bool stack_entry_is_tmpobject(StackDatum const& entry) {
    return std::holds_alternative<Object*>(entry);
}
bool stack_entry_is_tmpobject(std::size_t i) {
    return stack_entry_is_tmpobject(get_stack_entry_variant(i));
}

void unref_if_tmpobject(StackDatum& entry) {
    if (stack_entry_is_tmpobject(entry)) {
        auto* o = std::get<Object*>(entry);
        --tobj_count;
        hoc_obj_unref(o);
        entry = nullptr;  // STKOBJ_UNREF;
    }
}

int get_legacy_int_type(StackDatum const& entry) {
    if (std::holds_alternative<char**>(entry)) {
        return STRING;
    } else if (std::holds_alternative<neuron::container::generic_data_handle>(entry)) {
        return VAR;
    } else if (std::holds_alternative<double>(entry)) {
        return NUMBER;
    } else if (std::holds_alternative<Object**>(entry)) {
        return OBJECTVAR;
    } else if (std::holds_alternative<Object*>(entry)) {
        return OBJECTTMP;
    } else if (std::holds_alternative<int>(entry)) {
        return USERINT;
    } else if (std::holds_alternative<Symbol*>(entry)) {
        return SYMBOL;
    } else if (std::holds_alternative<std::nullptr_t>(entry)) {
        return STKOBJ_UNREF;
    } else {
        throw std::runtime_error("get_legacy_int_type");
    }
}
}  // namespace

/** Get the type of the top entry.
 */
int hoc_stack_type() {
    return get_legacy_int_type(get_stack_entry_variant(0));
}

bool hoc_stack_type_is_ndim() {
    return std::holds_alternative<stack_ndim_datum>(get_stack_entry_variant(0));
}

void hoc_pop_defer() {
    if (unref_defer_) {
        hoc_unref_defer();
    }
    if (stack.empty()) {
        hoc_execerror("stack underflow", nullptr);
    }
    if (hoc_stack_type() == OBJECTTMP) {
        unref_defer_ = hoc_look_inside_stack<Object*>(0);
        if (unref_defer_) {
            ++unref_defer_->refcount;
        }
    }
    hoc_nopop();
}

/* should be called on each OBJECTTMP on the stack after adjusting the
stack pointer downward */

void hoc_stkobj_unref(Object* o, int stkindex) {
    // stkindex refers to the value, stkindex+1 refers to the legacy type
    // information for it
    auto& entry = stack[stkindex];
    if (stack_entry_is_tmpobject(entry)) {
        --tobj_count;
        hoc_obj_unref(o);
        entry = nullptr;  // STKOBJ_UNREF;
    }
}

/* check the args of the frame and unref any of type OBJECTTMP */
static void frameobj_clean(Frame* f) {
    if (f->nargs == 0) {
        return;
    }
    StackDatum* s = f->argn + 1;
    for (int i = f->nargs - 1; i >= 0; --i) {
        --s;
        unref_if_tmpobject(*s);
    }
}

/* unref items on the stack frame associated with localobj in case of error */
static void frame_objauto_recover_on_err(Frame* ff) { /* only on error */
    // for all frames deeper than the one we are restoring to
    for (Frame* f = fp; f > ff; --f) {
        Symbol* sp = f->sp;
        if (sp->type == MECHANISM) {
            // Possible to reach this code path via the frame push in
            // NrnProperty::NrnProperty(const char* name) with the
            // extracellular mechanism Symbol
            continue;
        }
        if (!sp->u.u_proc) {  // Skip if the procedure is not defined
            continue;
        }
        // argn is the nargs argument on the stack.
        // stkp is the last localobj slot pair on the stack.
        StackDatum* stkp = f->argn + sp->u.u_proc->nauto;
        assert(!stack.empty());
        assert(stkp >= &stack.front());
        assert(stkp <= &stack.back());
        for (int i = sp->u.u_proc->nobjauto - 1; i >= 0; --i) {
            auto* ob = cast<Object*>(stkp[-i]);
            hoc_obj_unref(ob);
            /* Note that these AUTOOBJECT stack locations have an itemtype that
               are left over from the previous stack usage of that location.
               Regardless of that itemtype (e.g. OBJECTTMP), these did NOT
               increment tobj_count so we need to guarantee that the subsequent
               stack_obtmp_recover_on_err does not inadvertently free it again
               by setting the itemtype to a non OBJECTTMP value.  I hope this is
               the only place where stack space was used in which no item type
               was specified.
               We are doing this here which happens rarely to avoid having to
               set them when the stack obj pointers are zeroed.
            */
            // stkp[-2 * i + 1].i = 0;
        }
    }
}

static void stack_obtmp_recover_on_err(int tcnt) {
    if (tobj_count > tcnt) {
        // unref tmpobjects from the top of the stack until we have the right number left
        for (const auto&& [index, stkp]: renumerate(stack)) {
            if (stack_entry_is_tmpobject(stkp)) {
                hoc_stkobj_unref(std::get<Object*>(stkp), index);
                if (tobj_count == tcnt) {
                    return;
                }
            } else if (std::holds_alternative<std::nullptr_t>(stkp)) {
                printf("OBJECTTMP at stack index %ld already unreffed\n", index);
            }
        }
    }
}

// create space for stack and code
void hoc_init_space() {
    if (hoc_nframe == 0) {
        hoc_nframe = NFRAME;
    }
    if (hoc_nstack == 0) {
        // Default stack size
        hoc_nstack = 1000;
    }
    stack.reserve(hoc_nstack);
    progp = progbase = prog = (Inst*) emalloc(sizeof(Inst) * NPROG);
    fp = frame = (Frame*) emalloc(sizeof(Frame) * hoc_nframe);
    framelast = frame + hoc_nframe;
    hoc_temp_obj_pool_ = (Object**) emalloc(sizeof(Object*) * TOBJ_POOL_SIZE);
}

#define MAXINITFCNS 10
static int maxinitfcns;
static Pfrv initfcns[MAXINITFCNS];

/** @brief Print up to the 10 most-recently-pushed elements on the stack.
 */
void hoc_prstack() {
    std::size_t i{};
    std::ostringstream oss;
    oss << "interpreter stack: " << stack.size() << '\n';
    for (auto&& stkp: reverse(stack)) {
        if (i > 10) {
            oss << " ...\n";
            break;
        }
        std::visit(
            [i, &oss](auto& value) {
                oss << ' ' << i << ' ';
                if constexpr (std::is_same_v<std::decay_t<decltype(value)>, std::nullptr_t>) {
                    oss << "nullptr";
                } else {
                    oss << value;
                }
                oss << ' ' << cxx_demangle(typeid(decltype(value)).name()) << '\n';
            },
            stkp);
        ++i;
    }
    Printf(oss.str().c_str());
}

void hoc_on_init_register(Pfrv pf) {
    /* modules that may have to be cleaned up after an execerror */
    if (maxinitfcns < MAXINITFCNS) {
        initfcns[maxinitfcns++] = pf;
    } else {
        fprintf(stderr, "increase definition for MAXINITFCNS\n");
        nrn_exit(1);
    }
}

// initialize for code generation
void hoc_initcode() {
    int i;
    errno = 0;
    if (hoc_errno_count > 5) {
        fprintf(stderr, "errno set %d times on last execution\n", hoc_errno_count);
    }
    hoc_errno_count = 0;
    prog_parse_recover = progbase = prog;
    progp = progbase;
    hoc_unref_defer();

    frame_objauto_recover_on_err(frame);
    if (tobj_count) {
        stack_obtmp_recover_on_err(0);
#if DEBUG_GARBAGE
        if (tobj_count) {
            printf("initcode failed with %d left\n", tobj_count);
        }
#endif
        tobj_count = 0;
    }
    fp = frame;
    free_list(&p_symlist);
    hoc_returning = 0;
    do_equation = 0;
    for (i = 0; i < maxinitfcns; ++i) {
        (*initfcns[i])();
    }
    nrn_initcode(); /* special requirements for NEURON */
}


static Frame* rframe;
static std::size_t rstack;
static const char* parsestr;

void oc_save_code(Inst** a1,
                  Inst** a2,
                  std::size_t& a3,
                  Frame** a4,
                  int* a5,
                  int* a6,
                  Inst** a7,
                  Frame** a8,
                  std::size_t& a9,
                  Symlist** a10,
                  Inst** a11,
                  int* a12) {
    *a1 = progbase;
    *a2 = progp;
    a3 = stack.size();
    *a4 = fp;
    *a5 = hoc_returning;
    *a6 = do_equation;
    *a7 = pc;
    *a8 = rframe;
    a9 = rstack;
    *a10 = p_symlist;
    *a11 = prog_parse_recover;
    *a12 = tobj_count;
}

void oc_restore_code(Inst** a1,
                     Inst** a2,
                     std::size_t& a3,
                     Frame** a4,
                     int* a5,
                     int* a6,
                     Inst** a7,
                     Frame** a8,
                     std::size_t& a9,
                     Symlist** a10,
                     Inst** a11,
                     int* a12) {
    progbase = *a1;
    progp = *a2;
    frame_objauto_recover_on_err(*a4);
    if (tobj_count > *a12) {
        stack_obtmp_recover_on_err(*a12);
#if DEBUG_GARBAGE
        if (tobj_count != *a12) {
            printf("oc_restore_code tobj_count=%d should be %d\n", tobj_count, *a12);
        }
#endif
    }
    if (a3 > stack.size()) {
        hoc_execerror("oc_restore_code cannot summon stack entries from nowhere", nullptr);
    }
    stack.resize(a3);
    fp = *a4;
    hoc_returning = *a5;
    do_equation = *a6;
    pc = *a7;
    rframe = *a8;
    rstack = a9;
    p_symlist = *a10;
    prog_parse_recover = *a11;
}

int hoc_strgets_need(void) {
    return strlen(parsestr);
}

char* hoc_strgets(char* cbuf, int nc) { /* getc for a string, used by parser */
    strncpy(cbuf, parsestr, nc);
    if (*parsestr == '\0') {
        return (char*) 0;
    } else {
        return cbuf;
    }
}

// initialize for recursive code generation
static void rinitcode() {
    errno = 0;
    hoc_errno_count = 0;
    hoc_prog_parse_recover = hoc_progbase;
    hoc_progp = hoc_progbase;
    if (rstack > stack.size()) {
        hoc_execerror("rinitcode cannot create stack entries from nowhere", nullptr);
    }
    stack.resize(rstack);
    fp = rframe;
    hoc_free_list(&hoc_p_symlist);
    if (hoc_returning != 4) { /* if stop not seen */
        hoc_returning = 0;
    }
    hoc_do_equation = 0;
}

int hoc_ParseExec(int yystart) {
    /* can recursively parse and execute what is in cbuf.
       may parse single tokens. called from hoc_oc(str).
       All these parse and execute routines should be combined into
       a single method robust method. The pipeflag method has become
       encrusted with too many irrelevant mechanisms. There is no longer
       anything sacred about the cbuf. The only requiremnent is to tell
       the get line function where to get its string.
     */
    int yret;

    Frame *sframe, *sfp;
    Inst *sprogbase, *sprogp, *spc, *sprog_parse_recover;
    Symlist* sp_symlist;
    std::size_t sstack{}, sstackp{};

    if (yystart) {
        sframe = rframe;
        sfp = fp;
        sprogbase = progbase;
        sprogp = progp;
        spc = pc, sprog_parse_recover = prog_parse_recover;
        sstackp = stack.size();
        sstack = rstack;
        sp_symlist = p_symlist;
        rframe = fp;
        rstack = stack.size();
        progbase = progp;
        p_symlist = (Symlist*) 0;
        rinitcode();
    }
    if (hoc_in_yyparse) {
        hoc_execerror("Cannot reenter parser.",
                      "Maybe you were in the middle of a direct command.");
    }
    yret = yyparse();
    switch (yret) {
    case 1:
        hoc_execute(hoc_progbase);
        rinitcode();
        break;
    case -3:
        hoc_execerror("incomplete statement parse not allowed\n", nullptr);
    default:
        break;
    }
    if (yystart) {
        rframe = sframe;  // restore the old value
        fp = sfp;
        progbase = sprogbase;
        progp = sprogp;
        pc = spc;
        prog_parse_recover = sprog_parse_recover;
        if (sstackp > stack.size()) {
            hoc_execerror("hoc_ParseExec cannot summon entries from nowhere", nullptr);
        }
        stack.resize(sstackp);
        rstack = sstack;
        p_symlist = sp_symlist;
    }

    return yret;
}

int hoc_xopen_run(Symbol* sp, const char* str) { /*recursively parse and execute for xopen*/
                                                 /* if sp != 0 then parse string and save code */
                                                 /* without executing. Note str must be a 'list'*/
    int n = 0;
    Frame *sframe = rframe, *sfp = fp;
    Inst *sprogbase = progbase, *sprogp = progp, *spc = pc,
         *sprog_parse_recover = prog_parse_recover;
    Symlist* sp_symlist = p_symlist;
    std::size_t sstack{rstack}, sstackp{stack.size()};
    rframe = fp;
    rstack = stack.size();
    progbase = progp;
    p_symlist = (Symlist*) 0;

    if (sp == (Symbol*) 0) {
        for (rinitcode(); hoc_yyparse(); rinitcode())
            execute(progbase);
    } else {
        int savpipeflag;
        rinitcode();
        savpipeflag = hoc_pipeflag;
        hoc_pipeflag = 2;
        parsestr = str;
        if (!hoc_yyparse()) {
            execerror("Nothing to parse", (char*) 0);
        }
        n = (int) (progp - progbase);
        hoc_pipeflag = savpipeflag;
        hoc_define(sp);
        rinitcode();
    }
    rframe = sframe;
    fp = sfp;
    progbase = sprogbase;
    progp = sprogp;
    pc = spc;
    prog_parse_recover = sprog_parse_recover;
    if (sstackp > stack.size()) {
        hoc_execerror("hoc_xopen_run cannot summon entries from nowhere", nullptr);
    }
    stack.resize(sstackp);
    rstack = sstack;
    p_symlist = sp_symlist;
    return n;
}

#define HOC_TEMP_CHARPTR_SIZE 128
static char* stmp[HOC_TEMP_CHARPTR_SIZE];
static int istmp = 0;

char** hoc_temp_charptr(void) {
    istmp = (istmp + 1) % HOC_TEMP_CHARPTR_SIZE;
    return stmp + istmp;
}

int hoc_is_temp_charptr(char** cpp) {
    if (cpp >= stmp && cpp < stmp + HOC_TEMP_CHARPTR_SIZE) {
        return 1;
    }
    return 0;
}

namespace {
/** @brief Pop the top of the stack, ignoring its type.
 */
void pop_value() noexcept {
    if (stack.empty()) {
        hoc_execerror("stack underflow", nullptr);
    }
    stack.pop_back();
}

/** @brief Pop and return the top of the stack, with type checking.
 */
template <typename T>
T pop_value() {
    if (stack.empty()) {
        hoc_execerror("stack underflow", nullptr);
    }
    if (std::holds_alternative<T>(stack.back())) {
        auto rval = std::move(stack.back());
        stack.pop_back();
        return std::get<T>(std::move(rval));
    } else {
        report_type_mismatch<T>(stack.back());
    }
}

/** @brief Push a value to the top of the stack.
 */
template <typename T>
void push_value(T value) {
    if (stack.size() == stack.capacity()) {
        hoc_execerror("Stack too deep.", "Increase with -NSTACK stacksize option");
    }
    stack.emplace_back(std::move(value));
}

// Get the type and value of the nth argument
StackDatum& get_argument(int narg) {
    if (narg > fp->nargs) {
        hoc_execerror(fp->sp->name, "not enough arguments");
    }
    return fp->argn[narg - fp->nargs];
}
}  // namespace

int hoc_stacktype() {
    return hoc_stack_type();
}

// push double onto stack
void hoc_pushx(double d) {
    push_value(d);
}

// push pointer to object pointer onto stack
void hoc_pushobj(Object** d) {
    if (d >= hoc_temp_obj_pool_ && d < (hoc_temp_obj_pool_ + TOBJ_POOL_SIZE)) {
        hoc_push_object(*d);
    } else {
        push_value(d);
    }
}

// push pointer to object onto stack
void hoc_push_object(Object* d) {
    push_value(d);
    hoc_obj_ref(d);
    ++tobj_count;
}

// push pointer to string pointer onto stack
void hoc_pushstr(char** d) {
    push_value(d);
}

// code for pushing a symbols string
void hoc_push_string() {
    Symbol* s{(hoc_pc++)->sym};
    if (!s) {
        hoc_pushstr(nullptr);
    } else if (s->type == CSTRING) {
        hoc_pushstr(&(s->u.cstr));
    } else {
        Object* obsav{};
        Objectdata* odsav{};
        Symlist* slsav{};
        if (s->cpublic == 2) {
            s = s->u.sym;
            odsav = hoc_objectdata_save();
            obsav = hoc_thisobject;
            slsav = hoc_symlist;
            hoc_objectdata = hoc_top_level_data;
            hoc_thisobject = 0;
            hoc_symlist = hoc_top_level_symlist;
        }
        hoc_pushstr(OPSTR(s));
        if (obsav) {
            hoc_objectdata = hoc_objectdata_restore(odsav);
            hoc_thisobject = obsav;
            hoc_symlist = slsav;
        }
    }
}

// push double pointer onto stack
void hoc_pushpx(double* d) {
    // Trying to promote a raw pointer to a data handle is expensive, so don't
    // do that every time we push a double* onto the stack.
    hoc_push(neuron::container::data_handle<double>{neuron::container::do_not_search, d});
}

// push symbol pointer onto stack
void hoc_pushs(Symbol* d) {
    push_value(d);
}

/* push integer onto stack */
void hoc_pushi(int d) {
    push_value(d);
}

void hoc_push(neuron::container::generic_data_handle handle) {
    push_value(std::move(handle));
}

/* push index onto stack */
void hoc_push_ndim(int d) {
    push_value(stack_ndim_datum(d));
}

// type of nth arg
int hoc_argtype(int narg) {
    return get_legacy_int_type(get_argument(narg));
}

int hoc_is_double_arg(int narg) {
    return (hoc_argtype(narg) == NUMBER);
}

int hoc_is_pdouble_arg(int narg) {
    return (hoc_argtype(narg) == VAR);
}

int hoc_is_str_arg(int narg) {
    return (hoc_argtype(narg) == STRING);
}

int hoc_is_object_arg(int narg) {
    auto const type = hoc_argtype(narg);
    return (type == OBJECTVAR || type == OBJECTTMP);
}

int hoc_is_tempobj_arg(int narg) {
    return (hoc_argtype(narg) == OBJECTTMP);
}

Object* hoc_obj_look_inside_stack(int i) { /* stack pointer at depth i; i=0 is top */
    auto const& entry = get_stack_entry_variant(i);
    return std::visit(
        overloaded{[](Object* o) { return o; },
                   [](Object** o) { return *o; },
                   [](auto const& val) -> Object* {
                       throw std::runtime_error(
                           "hoc_obj_look_inside_stack expects Object* or Object** but found " +
                           cxx_demangle(typeid(val).name()));
                   }},
        entry);
}

int hoc_inside_stacktype(int i) { /* 0 is top */
    return get_legacy_int_type(get_stack_entry_variant(i));
}

// pop double and return top elem from stack
double hoc_xpop() {
    return pop_value<double>();
}

namespace neuron {
/** @brief hoc_get_arg<generic_data_handle>()
 */
container::generic_data_handle oc::detail::hoc_get_arg_helper<container::generic_data_handle>::impl(
    std::size_t narg) {
    return cast<container::generic_data_handle>(get_argument(narg));
}
/** @brief hoc_pop<generic_data_handle>()
 */
container::generic_data_handle oc::detail::hoc_pop_helper<container::generic_data_handle>::impl() {
    return pop_value<neuron::container::generic_data_handle>();
}
}  // namespace neuron

// pop double pointer and return top elem from stack
double* hoc_pxpop() {
    // All pointers are actually stored on the stack as data handles
    return static_cast<double*>(hoc_pop<neuron::container::data_handle<double>>());
}

// pop symbol pointer and return top elem from stack
Symbol* hoc_spop() {
    return pop_value<Symbol*>();
}

// pop array index and return top elem from stack
int hoc_pop_ndim() {
    return pop_value<stack_ndim_datum>().i;
}

/** @brief Pop pointer to object pointer and return top elem from stack.
 *
 *  When using objpop, after dealing with the pointer, one should call
 *  hoc_tobj_unref(pobj) in order to prevent memory leakage since the object may
 *  have been reffed when it was pushed on the stack.
 */
Object** hoc_objpop() {
    if (stack_entry_is_tmpobject(0)) {
        return hoc_temp_objptr(pop_value<Object*>());
    }
    return pop_value<Object**>();
}

void TmpObjectDeleter::operator()(Object* o) const {
    --tobj_count;
    hoc_obj_unref(o);
}

// pop object and return top elem from stack, the return value is wrapped up in
// a unique_ptr that calls TmpObjectDeleter::operator() from its destructor.
TmpObject hoc_pop_object() {
    return TmpObject{pop_value<Object*>()};
}

// pop pointer to string pointer and return top elem from stack
char** hoc_strpop() {
    return pop_value<char**>();
}

// pop symbol pointer and return top elem from stack
int hoc_ipop() {
    return pop_value<int>();
}

// just pop the stack without returning anything
void hoc_nopop() {
    auto const& top_entry = get_stack_entry_variant(0);
    if (stack_entry_is_tmpobject(top_entry)) {  // OBJECTTMP
        // TODO this should be handled by the destructor of the variant member
        // corresponding to OBJECTTMP
        // TODO double check this
        hoc_stkobj_unref(std::get<Object*>(top_entry), stack.size() - 1);
    }
    pop_value();
}

// push constant onto stack
void hoc_constpush() {
    hoc_pushx(*((pc++)->sym)->u.pnum);
}

// push zero onto stack
void hoc_pushzero() {
    hoc_pushx(0.);
}

// push variable onto stack
void hoc_varpush() {
    hoc_pushs((pc++)->sym);
}

#define relative(pc) (pc + (pc)->i)

void forcode(void) {
    double d;
    Inst* savepc = pc; /* loop body */
    int isec;

    isec = nrn_isecstack();
    execute(savepc + 3); /* condition */
    d = hoc_xpop();
    while (d) {
        execute(relative(savepc)); /* body */
        if (hoc_returning) {
            nrn_secstack(isec);
        }
        if (hoc_returning == 1 || hoc_returning == 4) /* return or stop */
            break;
        else if (hoc_returning == 2) /* break */
        {
            hoc_returning = 0;
            break;
        } else /* continue */
            hoc_returning = 0;
        if ((savepc + 2)->i)               /* diff between while and for */
            execute(relative(savepc + 2)); /* increment */
        execute(savepc + 3);
        d = hoc_xpop();
    }
    if (!hoc_returning)
        pc = relative(savepc + 1); /* next statement */
}

void hoc_shortfor(void) {
    Inst* savepc = pc;
    double begin, end, *pval = 0;
    Symbol* sym;
    int isec;

    end = hoc_xpop() + hoc_epsilon;
    begin = hoc_xpop();
    sym = hoc_spop();

    switch (sym->type) {
    case UNDEF:
        hoc_execerror(sym->name, "undefined variable");
    case VAR:
        if (!ISARRAY(sym)) {
            if (sym->subtype == USERINT) {
                execerror("integer iteration variable", sym->name);
            } else if (sym->subtype == USERDOUBLE) {
                pval = sym->u.pval;
            } else {
                pval = OPVAL(sym);
            }
            break;
        } else {
            if (sym->subtype == USERINT)
                execerror("integer iteration variable", sym->name);
            else if (sym->subtype == USERDOUBLE)
                pval = sym->u.pval + araypt(sym, SYMBOL);
            else
                pval = OPVAL(sym) + araypt(sym, OBJECTVAR);
        }
        break;
    case AUTO: {
        pval = &(cast<double>(fp->argn[sym->u.u_auto]));
    } break;
    default:
        execerror("for loop non-variable", sym->name);
    }
    isec = nrn_isecstack();
    for (*pval = begin; *pval <= end; *pval += 1.) {
        execute(relative(savepc));
        if (hoc_returning) {
            nrn_secstack(isec);
        }
        if (hoc_returning == 1 || hoc_returning == 4) {
            break;
        } else if (hoc_returning == 2) {
            hoc_returning = 0;
            break;
        } else {
            hoc_returning = 0;
        }
    }
    if (!hoc_returning)
        pc = relative(savepc + 1);
}

void hoc_iterator(void) {
    /* pc is ITERATOR symbol, argcount, stmtbegin, stmtend */
    /* for testing execute stmt once */
    Symbol* sym;
    int argcount;
    Inst *stmtbegin, *stmtend;

    sym = (pc++)->sym;
    argcount = (pc++)->i;
    stmtbegin = relative(pc);
    stmtend = relative(pc + 1);
    ;
    hoc_iterator_object(sym, argcount, stmtbegin, stmtend, hoc_thisobject);
}

void hoc_iterator_object(Symbol* sym, int argcount, Inst* beginpc, Inst* endpc, Object* ob) {
    fp++;
    if (fp >= framelast) {
        fp--;
        execerror(sym->name, "call nested too deeply, increase with -NFRAME framesize option");
    }
    fp->sp = sym;
    fp->nargs = argcount;
    fp->retpc = endpc;
    fp->argn = &stack.back();
    for (auto i = 0; i < sym->u.u_proc->nauto; ++i) {
        // last sym->u.u_proc->nobjauto of these are objects
        if (sym->u.u_proc->nauto - i <= sym->u.u_proc->nobjauto) {
            push_value(static_cast<Object*>(nullptr));
        } else {
            push_value(0.0);
        }
    }
    fp->iter_stmt_begin = beginpc;
    fp->iter_stmt_ob = ob;
    fp->ob = ob;
    hoc_execute(sym->u.u_proc->defn.in);
    hoc_nopop(); /* 0.0 from the procret() */
    if (hoc_returning != 4) {
        hoc_returning = 0;
    }
}

void hoc_iterator_stmt() {
    Inst* pcsav;
    Object* ob;
    Object* obsav;
    Objectdata* obdsav;
    Symlist* slsav;
    int isec;
    Frame* iter_f = fp; /* iterator frame */
    Frame* ef = fp - 1; /* iterator statement frame */
    fp++;               /* execution frame */

    fp->sp = iter_f->sp;
    fp->ob = iter_f->ob;
    if (ef != frame) {
        /*SUPPRESS 26*/
        fp->argn = ef->argn;
        fp->nargs = ef->nargs;
    } else { /* top. only for stack trace */
        fp->argn = 0;
        fp->nargs = 0;
    }

    ob = iter_f->iter_stmt_ob;
    obsav = hoc_thisobject;
    obdsav = hoc_objectdata_save();
    slsav = hoc_symlist;
    hoc_thisobject = ob;
    if (ob) {
        hoc_objectdata = ob->u.dataspace;
        hoc_symlist = ob->ctemplate->symtable;
    } else {
        hoc_objectdata = hoc_top_level_data;
        hoc_symlist = hoc_top_level_symlist;
    }

    pcsav = pc;
    isec = nrn_isecstack();
    execute(iter_f->iter_stmt_begin);
    pc = pcsav;
    hoc_objectdata = hoc_objectdata_restore(obdsav);
    hoc_thisobject = obsav;
    hoc_symlist = slsav;
    --fp;
    if (hoc_returning) {
        nrn_secstack(isec);
    }
    switch (hoc_returning) {
    case 1: /* return means not only return from iter but return from
        the procedure containing the iter statement */
        hoc_execerror("return from within an iterator statement not allowed.",
                      "Set a flag and use break.");
    case 2: /* break means return from iter */
        procret();
        break;
    case 3: /* continue means go on from iter as though nothing happened*/
        hoc_returning = 0;
        break;
    }
}

static void for_segment2(Symbol* sym, int mode) {
    /* symbol on stack; statement pointed to by pc
    continuation pointed to by pc+1. template used is shortfor in code.cpp
    of hoc system.
    */
    int i, imax;
    Inst* savepc = pc;
    double *pval = 0, dx;
    int isec;

    switch (sym->type) {
    case UNDEF:
        hoc_execerror(sym->name, "undefined variable");
    case VAR:
        if (!ISARRAY(sym)) {
            if (sym->subtype == USERINT) {
                execerror("integer iteration variable", sym->name);
            } else if (sym->subtype == USERDOUBLE) {
                pval = sym->u.pval;
            } else {
                pval = OPVAL(sym);
            }
            break;
        } else {
            if (sym->subtype == USERINT)
                execerror("integer iteration variable", sym->name);
            else if (sym->subtype == USERDOUBLE)
                pval = sym->u.pval + araypt(sym, SYMBOL);
            else
                pval = OPVAL(sym) + araypt(sym, OBJECTVAR);
        }
        break;
    case AUTO: {
        auto& entry = fp->argn[sym->u.u_auto];
        entry = 0.0;  // update the active entry
        pval = &(cast<double>(entry));
    } break;
    default:
        execerror("for loop non-variable", sym->name);
    }
    imax = segment_limits(&dx);
    {
        if (mode == 0) {
            i = 1;
            *pval = dx / 2.;
        } else {
            i = 0;
            *pval = 0.;
        }
        isec = nrn_isecstack();
        for (; i <= imax; i++) {
            if (i == imax) {
                if (mode == 0) {
                    continue;
                }
                *pval = 1.;
            }
            execute(relative(savepc));
            if (hoc_returning) {
                nrn_secstack(isec);
            }
            if (hoc_returning == 1 || hoc_returning == 4) {
                break;
            } else if (hoc_returning == 2) {
                hoc_returning = 0;
                break;
            } else {
                hoc_returning = 0;
            }
            if (i == 0) {
                *pval += dx / 2.;
            } else if (i < imax) {
                *pval += dx;
            }
        }
    }
    if (!hoc_returning)
        pc = relative(savepc + 1);
}

void for_segment(void) {
    for_segment2(hoc_spop(), 1);
}

void for_segment1(void) {
    Symbol* sym;
    double d;
    int mode;
    d = hoc_xpop();
    sym = hoc_spop();
    mode = (fabs(d) < hoc_epsilon) ? 0 : 1;
    for_segment2(sym, mode);
}

void ifcode(void) {
    double d;
    Inst* savepc = pc; /* then part */

    execute(savepc + 3); /* condition */
    d = hoc_xpop();
    if (d)
        execute(relative(savepc));
    else if ((savepc + 1)->i) /* else part? */
        execute(relative(savepc + 1));
    if (!hoc_returning)
        pc = relative(savepc + 2); /* next stmt */
}

void Break(void) /* break statement */
{
    hoc_returning = 2;
}

void Continue(void) /* continue statement */
{
    hoc_returning = 3;
}

void Stop(void) /* stop statement */
{
    hoc_returning = 4;
}

void hoc_define(Symbol* sp) { /* put func/proc in symbol table */
    Inst *inst, *newinst;

    if (sp->u.u_proc->defn.in != STOP)
        free((char*) sp->u.u_proc->defn.in);
    free_list(&(sp->u.u_proc->list));
    sp->u.u_proc->list = p_symlist;
    p_symlist = (Symlist*) 0;
    sp->u.u_proc->size = (unsigned) (progp - progbase);
    sp->u.u_proc->defn.in = (Inst*) emalloc((unsigned) (progp - progbase) * sizeof(Inst));
    newinst = sp->u.u_proc->defn.in;
    for (inst = progbase; inst != progp;)
        *newinst++ = *inst++;
    progp = progbase; /* next code starts here */
}

// print the call sequence on an execerror
void frame_debug() {
    Frame* f;
    int i, j;
    char id[10];

    if (nrnmpi_numprocs_world > 1) {
        Sprintf(id, "%d ", nrnmpi_myid_world);
    } else {
        id[0] = '\0';
    }
    for (i = 5, f = fp; f != frame && --i; f = f - 1) { /* print only to depth of 5 */
        for (j = i; j; j--) {
            Fprintf(stderr, "  ");
        }
        if (f->ob) {
            Fprintf(stderr, "%s%s.%s(", id, hoc_object_name(f->ob), f->sp->name);  // error
        } else {
            Fprintf(stderr, "%s%s(", id, f->sp->name);
        }
        for (j = 1; j <= f->nargs;) {
            auto const& entry = f->argn[j - f->nargs];
            std::visit(overloaded{[](double val) { Fprintf(stderr, "%g", val); },
                                  [](char** pstr) {
                                      const char* s = *pstr;
                                      if (strlen(s) > 15) {
                                          Fprintf(stderr, "\"%.10s...\"", s);
                                      } else {
                                          Fprintf(stderr, "\"%s\"", s);
                                      }
                                  },
                                  [](Object** pobj) {
                                      Fprintf(stderr, "%s", hoc_object_name(*pobj));
                                  },
                                  [](auto const&) { Fprintf(stderr, "..."); }},
                       entry);
            if (++j <= f->nargs) {
                Fprintf(stderr, ", ");
            }
        }
        Fprintf(stderr, ")\n");
    }
    if (i <= 0) {
        Fprintf(stderr, "and others\n");
    }
}

void hoc_push_frame(Symbol* sp, int narg) { /* helpful for explicit function calls */
    if (++fp >= framelast) {
        --fp;
        execerror(sp->name, "call nested too deeply, increase with -NFRAME framesize option");
    }
    fp->sp = sp;
    fp->nargs = narg;
    fp->argn = &stack.back();  // last argument
    fp->ob = hoc_thisobject;
}

void pop_frame(void) {
    int i;
    frameobj_clean(fp);
    for (i = 0; i < fp->nargs; i++) {
        // pop arguments
        hoc_nopop();
    }
    --fp;
}

// call a function
void hoc_call() {
    int i, isec;
    Symbol* sp = pc[0].sym; /* symbol table entry */
    /* for function */
    if (++fp >= framelast) {
        --fp;
        execerror(sp->name, "call nested too deeply, increase with -NFRAME framesize option");
    }
    fp->sp = sp;
    fp->nargs = pc[1].i;
    fp->retpc = pc + 2;
    fp->ob = hoc_thisobject;
    fp->argn = &stack.back();  // last argument
    BBSPOLL
    isec = nrn_isecstack();
    if (sp->type == FUN_BLTIN || sp->type == OBJECTFUNC || sp->type == STRINGFUNC) {
        // Push slots for auto
        for (auto i = 0; i < sp->u.u_proc->nauto; ++i) {
            push_value(0.0);
        }
        (sp->u.u_proc->defn.pf)();
        if (hoc_errno_check()) {
            hoc_warning("errno set during call of", sp->name);
        }
    } else if ((sp->type == FUNCTION || sp->type == PROCEDURE || sp->type == HOCOBJFUNCTION) &&
               sp->u.u_proc->defn.in != STOP) {
        // Push slots for auto
        for (auto i = 0; i < sp->u.u_proc->nauto; ++i) {
            // last sym->u.u_proc->nobjauto of these are objects
            if (sp->u.u_proc->nauto - i <= sp->u.u_proc->nobjauto) {
                push_value(static_cast<Object*>(nullptr));
            } else {
                push_value(0.0);
            }
        }
        if (sp->cpublic == 2) {
            Objectdata* odsav = hoc_objectdata_save();
            Object* obsav = hoc_thisobject;
            Symlist* slsav = hoc_symlist;

            hoc_objectdata = hoc_top_level_data;
            hoc_thisobject = 0;
            hoc_symlist = hoc_top_level_symlist;

            execute(sp->u.u_proc->defn.in);

            hoc_objectdata = hoc_objectdata_restore(odsav);
            hoc_thisobject = obsav;
            hoc_symlist = slsav;
        } else {
            execute(sp->u.u_proc->defn.in);
        }
        /* the autoobject pointers were unreffed at the ret() */

    } else {
        execerror(sp->name, "undefined function");
    }
    if (hoc_returning) {
        nrn_secstack(isec);
    }
    if (hoc_returning != 4) { /*if not stopping */
        hoc_returning = 0;
    }
}

void hoc_fake_call(Symbol* s) {
    /*fake it so c code can call functions that ret() */
    /* but these functions better not ask for any arguments */
    /* don't forget a double is left on the stack and returning = 1 */
    /* use the symbol for the function as the argument, only requirement
       which is always true is that it has no local variables pushed on
       the stack so nauto=0 and nobjauto=0 */
    ++fp;
    fp->sp = s;
    fp->nargs = 0;
    fp->retpc = pc;
    fp->ob = nullptr;
}

double hoc_call_func(Symbol* s, int narg) {
    /* call the symbol as a function, The args better be pushed on the stack
    first arg first. */
    if (s->type == BLTIN) {
        return (*(s->u.ptr))(xpop());
    } else {
        Inst* pcsav;
        Inst fc[4];
        fc[0].pf = hoc_call;
        fc[1].sym = s;
        fc[2].i = narg;
        fc[3].in = STOP;

        pcsav = hoc_pc;
        hoc_execute(fc);
        hoc_pc = pcsav;
        return hoc_xpop();
    }
}

/* common return from func, proc, or iterator */
void hoc_ret() {
    // unref all the auto object pointers
    for (int i = fp->sp->u.u_proc->nobjauto - 1; i >= 0; --i) {
        // this is going from the deepest automatic object in the stack to the shallowest
        // should this decremenet tobj_count?
        hoc_obj_unref(hoc_look_inside_stack<Object*>(i));  //
    }
    // Pop off the autos
    for (auto i = 0; i < fp->sp->u.u_proc->nauto; ++i) {
        pop_value();
    }
    frameobj_clean(fp);
    for (int i = 0; i < fp->nargs; i++) {
        // pop arguments
        hoc_nopop();
    }
    hoc_pc = fp->retpc;
    --fp;
    hoc_returning = 1;
}

void funcret(void) /* return from a function */
{
    double d;
    if (fp->sp->type != FUNCTION)
        execerror(fp->sp->name, "(proc or iterator) returns value");
    d = hoc_xpop(); /* preserve function return value */
    ret();
    hoc_pushx(d);
}

void procret(void) /* return from a procedure */
{
    if (fp->sp->type == FUNCTION)
        execerror(fp->sp->name, "(func) returns no value");
    if (fp->sp->type == HOCOBJFUNCTION)
        execerror(fp->sp->name, "(obfunc) returns no value");
    ret();
    hoc_pushx(0.); /*will be popped immediately; necessary because caller
                 may have compiled it as a function*/
}

void hocobjret(void) /* return from a hoc level obfunc */
{
    Object** d;
    if (fp->sp->type != HOCOBJFUNCTION)
        execerror(fp->sp->name, "objfunc returns objref");
    d = hoc_objpop(); /* preserve function return value */
    Object* o = *d;   // safe to deref here even if d points to localobj on stack.
    if (o) {
        o->refcount++;
    }
    // d may point to an Object* on the stack, ie. localobj name.
    // if so, d after ret() would point outside the stack,
    // so it cannot be dereferenced without a sanitizer error on the mac.
    ret();
    /*make a temp and ref it in case autoobj returned since ret would
    have unreffed it*/
    hoc_push_object(o);

    if (o) {
        o->refcount--;
    }
    hoc_tobj_unref(d);  // only dereferences d if points into the hoc_temp_obj_pool_
}

void hoc_Numarg(void) {
    int narg;
    Frame* f = fp - 1;
    if (f == frame) {
        narg = 0;
    } else {
        narg = f->nargs;
    }
    ret();
    hoc_pushx((double) narg);
}

void hoc_Argtype() {
    int narg, iarg, type, itype = 0;
    Frame* f = fp - 1;
    if (f == frame) {
        execerror("argtype can only be called in a func or proc", 0);
    }
    iarg = (int) chkarg(1, -1000., 100000.);
    if (iarg > f->nargs || iarg < 1) {
        itype = -1;
    } else {
        auto const& entry = f->argn[iarg - f->nargs];
        itype =
            std::visit(overloaded{[](double) { return 0; },
                                  [](Object*) { return 1; },
                                  [](Object**) { return 1; },
                                  [](char**) { return 2; },
                                  [](neuron::container::generic_data_handle const&) { return 3; },
                                  [](auto const& x) -> int {
                                      throw std::runtime_error(
                                          "hoc_Argtype didn't expect argument of type " +
                                          cxx_demangle(typeid(decltype(x)).name()));
                                  }},
                       entry);
    }
    hoc_retpushx(itype);
}

int ifarg(int narg) { /* true if there is an nth argument */
    if (narg > fp->nargs)
        return 0;
    return 1;
}

// return pointer to nth argument
Object** hoc_objgetarg(int narg) {
    auto const& arg_entry = get_argument(narg);
    if (stack_entry_is_tmpobject(arg_entry)) {
        return hoc_temp_objptr(std::get<Object*>(arg_entry));
    }
    return cast<Object**>(arg_entry);
}

// return pointer to nth argument
char** hoc_pgargstr(int narg) {
    return std::visit(overloaded{[](char** pstr) { return pstr; },
                                 [](Symbol* sym) {
                                     if (sym->type == CSTRING) {
                                         return &sym->u.cstr;
                                     } else if (sym->type == STRING) {
                                         return OPSTR(sym);
                                     } else {
                                         hoc_execerror("Expecting string argument", nullptr);
                                     }
                                 },
                                 [](auto const&) -> char** {
                                     hoc_execerror("Expecting string argument", nullptr);
                                 }},
                      get_argument(narg));
}

// return pointer to nth argument
double* hoc_getarg(int narg) {
    auto& arg_entry = get_argument(narg);
    auto& value = cast<double>(arg_entry);
    return &value;
}

int hoc_argindex(void) {
    int j = (int) hoc_xpop();
    if (j < 1) {
        hoc_execerror("arg index i < 1", 0);
    }
    return j;
}

// push argument onto stack
void hoc_arg() {
    int i = (pc++)->i;
    if (i == 0) {
        i = hoc_argindex();
    }
    hoc_pushx(*hoc_getarg(i));
}

void hoc_stringarg(void) /* push string arg onto stack */
{
    int i;
    i = (pc++)->i;
    if (i == 0) {
        i = hoc_argindex();
    }
    hoc_pushstr(hoc_pgargstr(i));
}

double hoc_opasgn(int op, double dest, double src) {
    switch (op) {
    case '+':
        return dest + src;
    case '*':
        return dest * src;
    case '-':
        return dest - src;
    case '/':
        if (src == 0.) {
            hoc_execerror("Divide by 0", (char*) 0);
        }
        return dest / src;
    default:
        return src;
    }
}

void argassign(void) /* store top of stack in argument */
{
    double d;
    int i, op;
    i = (pc++)->i;
    if (i == 0) {
        i = hoc_argindex();
    }
    op = (pc++)->i;
    d = hoc_xpop();
    if (op) {
        d = hoc_opasgn(op, *getarg(i), d);
    }
    hoc_pushx(d); /* leave value on stack */
    *getarg(i) = d;
}

void hoc_argrefasgn(void) {
    double d, *pd;
    int i, j, op;
    i = (pc++)->i;
    j = (pc++)->i;
    if (i == 0) {
        i = hoc_argindex();
    }
    op = (pc++)->i;
    d = hoc_xpop();
    if (j) {
        j = (int) (hoc_xpop() + hoc_epsilon);
    }
    pd = hoc_pgetarg(i);
    if (op) {
        d = hoc_opasgn(op, pd[j], d);
    }
    hoc_pushx(d); /* leave value on stack */
    pd[j] = d;
}

void hoc_argref(void) {
    int i, j;
    double* pd;
    i = (pc++)->i;
    j = (pc++)->i;
    if (i == 0) {
        i = hoc_argindex();
    }
    pd = hoc_pgetarg(i);
    if (!pd) {
        hoc_execerr_ext("hoc argument %d is an invalid datahandle\n", i);
    }
    if (j) {
        j = (int) (hoc_xpop() + hoc_epsilon);
    }
    hoc_pushx(pd[j]);
}


void hoc_argrefarg(void) {
    double* pd;
    int i;
    i = (pc++)->i;
    if (i == 0) {
        i = hoc_argindex();
    }
    pd = hoc_pgetarg(i);
    hoc_pushpx(pd);
}

void bltin(void) /* evaluate built-in on top of stack */
{
    double d;
    d = hoc_xpop();
    d = (*((pc++)->sym->u.ptr))(d);
    hoc_pushx(d);
}

Symbol* hoc_get_symbol(const char* var) {
    Symlist* sl = (Symlist*) 0;
    Symbol *prc, *sym;
    Inst* last;
    prc = hoc_parse_stmt(var, &sl);
    hoc_run_stmt(prc);

    last = (Inst*) prc->u.u_proc->defn.in + prc->u.u_proc->size - 1;
    if (last[-2].pf == eval) {
        sym = last[-3].sym;
    } else if (last[-3].pf == rangepoint || last[-3].pf == rangevareval) {
        sym = last[-2].sym;
    } else if (last[-4].pf == hoc_object_eval) {
        sym = last[-10].sym;
    } else {
        sym = (Symbol*) 0;
    }
    free_list(&sl);
    return sym;
}

Symbol* hoc_get_last_pointer_symbol(void) { /* hard to imagine a kludgier function*/
    Symbol* sym = (Symbol*) 0;
    Inst* pcv;
    int istop = 0;
    for (pcv = pc; pcv; --pcv) {
        if (pcv->pf == hoc_ob_pointer) {
            if (pcv[-2].sym) {
                sym = pcv[-2].sym; /* e.g. &ExpSyn[0].A */
            } else {
                sym = pcv[-6].sym; /* e.g. & Cell[0].soma.v(.5) */
            }
            break;
        } else if (pcv->pf == hoc_evalpointer) {
            sym = pcv[-1].sym;
            break;
        } else if (pcv->pf == rangevarevalpointer) {
            sym = pcv[1].sym;
            break;
        } else if (pcv->in == STOP) {
            /* hopefully only got here from python. Give up on second STOP*/
            if (istop++ == 1) {
                break;
            }
        }
    }
    return sym;
}

void hoc_autoobject(void) { /* AUTOOBJ symbol at pc+1. */
    /* pointer to object pointer left on stack */
    int i;
    Symbol* obs;
    Object** obp;
#if PDEBUG
    printf("code for hoc_autoobject()\n");
#endif
    obs = (pc++)->sym;
    hoc_pushobj(&(cast<Object*>(fp->argn[obs->u.u_auto])));
}

void eval(void) /* evaluate variable on stack */
{
    Objectdata* odsav;
    Object* obsav = 0;
    Symlist* slsav;
    double d = 0.0;
    extern double cable_prop_eval(Symbol*);
    Symbol* sym;
    sym = hoc_spop();
    if (sym->cpublic == 2) {
        sym = sym->u.sym;
        odsav = hoc_objectdata_save();
        obsav = hoc_thisobject;
        slsav = hoc_symlist;
        hoc_objectdata = hoc_top_level_data;
        hoc_thisobject = 0;
        hoc_symlist = hoc_top_level_symlist;
    }
    switch (sym->type) {
    case UNDEF:
        execerror("undefined variable", sym->name);
    case VAR:
        if (!ISARRAY(sym)) {
            if (do_equation && sym->s_varn > 0 && hoc_access[sym->s_varn] == 0) {
                hoc_access[sym->s_varn] = var_access;
                var_access = sym->s_varn;
            }
            switch (sym->subtype) {
            case USERDOUBLE:
                d = *(sym->u.pval);
                break;
            case USERINT:
                d = (double) (*(sym->u.pvalint));
                break;
            case USERPROPERTY:
                d = cable_prop_eval(sym);
                break;
            case USERFLOAT:
                d = (double) (*(sym->u.pvalfloat));
                break;
            default:
                d = *(OPVAL(sym));
                break;
            }
        } else {
            switch (sym->subtype) {
            case USERDOUBLE:
                d = (sym->u.pval)[araypt(sym, SYMBOL)];
                break;
            case USERINT:
                d = (sym->u.pvalint)[araypt(sym, SYMBOL)];
                break;
            case USERFLOAT:
                d = (sym->u.pvalfloat)[araypt(sym, SYMBOL)];
                break;
#if NEMO
            case NEMONODE:
                hoc_eval_nemonode(sym, hoc_xpop(), &d);
                break;
            case NEMOAREA:
                hoc_eval_nemoarea(sym, hoc_xpop(), &d);
                break;
#endif /*NEMO*/
            default:
                d = (OPVAL(sym))[araypt(sym, OBJECTVAR)];
                break;
            }
        }
        break;
    case AUTO:
        d = cast<double>(fp->argn[sym->u.u_auto]);
        break;
    default:
        execerror("attempt to evaluate a non-variable", sym->name);
    }

    if (obsav) {
        hoc_objectdata = hoc_objectdata_restore(odsav);
        hoc_thisobject = obsav;
        hoc_symlist = slsav;
    }
    hoc_pushx(d);
}

// leave pointer to variable on stack
void hoc_evalpointer() {
    Objectdata* odsav;
    Object* obsav = 0;
    Symlist* slsav;
    double* d = 0;
    //*cable_prop_eval_pointer();
    Symbol* sym;
    sym = hoc_spop();
    if (sym->cpublic == 2) {
        sym = sym->u.sym;
        odsav = hoc_objectdata_save();
        obsav = hoc_thisobject;
        slsav = hoc_symlist;
        hoc_objectdata = hoc_top_level_data;
        hoc_thisobject = 0;
        hoc_symlist = hoc_top_level_symlist;
    }
    switch (sym->type) {
    case UNDEF:
        execerror("undefined variable", sym->name);
    case VAR:
        if (!ISARRAY(sym)) {
            switch (sym->subtype) {
            case USERDOUBLE:
                d = sym->u.pval;
                break;
            case USERINT:
            case USERFLOAT:
                execerror("can use pointer only to doubles", sym->name);
                break;
            case USERPROPERTY:
                d = cable_prop_eval_pointer(sym);
                break;
            default:
                d = OPVAL(sym);
                break;
            }
        } else {
            switch (sym->subtype) {
            case USERDOUBLE:
                d = sym->u.pval + araypt(sym, SYMBOL);
                break;
            case USERINT:
            case USERFLOAT:
#if NEMO
            case NEMONODE:
            case NEMOAREA:
#endif /*NEMO*/
                execerror("can use pointer only to doubles", sym->name);
                break;
            default:
                d = OPVAL(sym) + araypt(sym, OBJECTVAR);
                break;
            }
        }
        break;
    case AUTO: {
        d = &(cast<double>(fp->argn[sym->u.u_auto]));
    } break;
    default:
        execerror("attempt to evaluate pointer to a non-variable", sym->name);
    }
    if (obsav) {
        hoc_objectdata = hoc_objectdata_restore(odsav);
        hoc_thisobject = obsav;
        hoc_symlist = slsav;
    }
    hoc_pushpx(d);
}

void add(void) /* add top two elems on stack */
{
    double d1, d2;
    d2 = hoc_xpop();
    d1 = hoc_xpop();
    d1 += d2;
    hoc_pushx(d1);
}

void hoc_sub(void) /* subtract top two elems on stack */
{
    double d1, d2;
    d2 = hoc_xpop();
    d1 = hoc_xpop();
    d1 -= d2;
    hoc_pushx(d1);
}

void mul(void) /* multiply top two elems on stack */
{
    double d1, d2;
    d2 = hoc_xpop();
    d1 = hoc_xpop();
    d1 *= d2;
    hoc_pushx(d1);
}

void hoc_div(void) /* divide top two elems on stack */
{
    double d1, d2;
    d2 = hoc_xpop();
    if (d2 == 0.0)
        execerror("division by zero", (char*) 0);
    d1 = hoc_xpop();
    d1 /= d2;
    hoc_pushx(d1);
}

void hoc_cyclic(void) /* the modulus function */
{
    double d1, d2;
    double r, q;
    d2 = hoc_xpop();
    if (d2 <= 0.)
        execerror("a%b, b<=0", (char*) 0);
    d1 = hoc_xpop();
    r = d1;
    if (r >= d2) {
        q = floor(d1 / d2);
        r = d1 - q * d2;
    } else if (r <= -d2) {
        q = floor(-d1 / d2);
        r = d1 + q * d2;
    }
    if (r > d2) {
        r = r - d2;
    }
    if (r < 0.) {
        r = r + d2;
    }

    hoc_pushx(r);
}

// negate top element on stack
void hoc_negate() {
    double const d = hoc_xpop();
    hoc_pushx(-d);
}

void gt(void) {
    double d1, d2;
    d2 = hoc_xpop();
    d1 = hoc_xpop();
    d1 = (double) (d1 > d2 + hoc_epsilon);
    hoc_pushx(d1);
}

void hoc_lt() {
    double d1, d2;
    d2 = hoc_xpop();
    d1 = hoc_xpop();
    d1 = (double) (d1 < d2 - hoc_epsilon);
    hoc_pushx(d1);
}

void ge(void) {
    double d1, d2;
    d2 = hoc_xpop();
    d1 = hoc_xpop();
    d1 = (double) (d1 >= d2 - hoc_epsilon);
    hoc_pushx(d1);
}

void le(void) {
    double d1, d2;
    d2 = hoc_xpop();
    d1 = hoc_xpop();
    d1 = (double) (d1 <= d2 + hoc_epsilon);
    hoc_pushx(d1);
}

void hoc_eq() {
    auto const& entry1 = get_stack_entry_variant(0);
    auto const& entry2 = get_stack_entry_variant(1);
    auto const type2 = get_legacy_int_type(entry2);
    double result{};
    switch (type2) {
    case NUMBER: {
        double d2{hoc_xpop()};
        double d1{hoc_xpop()};
        result = d1 <= d2 + hoc_epsilon && d1 >= d2 - hoc_epsilon;
    } break;
    case STRING:
        result = (strcmp(*hoc_strpop(), *hoc_strpop()) == 0);
        break;
    case OBJECTTMP:
    case OBJECTVAR: {
        Object** o1{hoc_objpop()};
        Object** o2{hoc_objpop()};
        result = (*o1 == *o2);
        hoc_tobj_unref(o1);
        hoc_tobj_unref(o2);
    } break;
    default:
        hoc_execerror("don't know how to compare these types", nullptr);
    }
    hoc_pushx(result);
}

void hoc_ne() {
    auto const& entry1 = get_stack_entry_variant(0);
    auto const& entry2 = get_stack_entry_variant(1);
    auto const type1 = get_legacy_int_type(entry1);
    double result{};
    switch (type1) {
    case NUMBER: {
        double d2{hoc_xpop()};
        double d1{hoc_xpop()};
        result = (d1 < d2 - hoc_epsilon || d1 > d2 + hoc_epsilon);
    } break;
    case STRING:
        result = (strcmp(*hoc_strpop(), *hoc_strpop()) != 0);
        break;
    case OBJECTTMP:
    case OBJECTVAR: {
        Object** o1{hoc_objpop()};
        Object** o2{hoc_objpop()};
        result = (*o1 != *o2);
        hoc_tobj_unref(o1);
        hoc_tobj_unref(o2);
    } break;
    default:
        hoc_execerror("don't know how to compare these types", nullptr);
    }
    hoc_pushx(result);
}

void hoc_and() {
    double const d2{hoc_xpop()};
    double const d1{hoc_xpop()};
    double const result = d1 != 0.0 && d2 != 0.0;
    hoc_pushx(result);
}

void hoc_or(void) {
    double d1, d2;
    d2 = hoc_xpop();
    d1 = hoc_xpop();
    d1 = (double) (d1 != 0.0 || d2 != 0.0);
    hoc_pushx(d1);
}

void hoc_not(void) {
    double d;
    d = hoc_xpop();
    d = (double) (d == 0.0);
    hoc_pushx(d);
}

// arg1 raised to arg2
void hoc_power() {
    double d1, d2;
    d2 = hoc_xpop();
    d1 = hoc_xpop();
    d1 = Pow(d1, d2);
    hoc_pushx(d1);
}

// assign result of execute to top symbol
void hoc_assign() {
    Objectdata* odsav;
    Object* obsav = 0;
    Symlist* slsav;
    int op;
    Symbol* sym;
    double d2;
    op = (pc++)->i;
    sym = hoc_spop();
    if (sym->cpublic == 2) {
        sym = sym->u.sym;
        odsav = hoc_objectdata_save();
        obsav = hoc_thisobject;
        slsav = hoc_symlist;
        hoc_objectdata = hoc_top_level_data;
        hoc_thisobject = 0;
        hoc_symlist = hoc_top_level_symlist;
    }
    d2 = hoc_xpop();
    switch (sym->type) {
    case UNDEF:
        hoc_execerror(sym->name, "undefined variable");
    case VAR:
        if (!ISARRAY(sym)) {
            switch (sym->subtype) {
            case USERDOUBLE:
                if (op) {
                    d2 = hoc_opasgn(op, *(sym->u.pval), d2);
                }
                *(sym->u.pval) = d2;
                break;
            case USERINT:
                if (op) {
                    d2 = hoc_opasgn(op, (double) (*(sym->u.pvalint)), d2);
                }
                *(sym->u.pvalint) = (int) (d2 + hoc_epsilon);
                break;
            case USERPROPERTY:
                cable_prop_assign(sym, &d2, op);
                break;
            case USERFLOAT:
                if (op) {
                    d2 = hoc_opasgn(op, (double) (*(sym->u.pvalfloat)), d2);
                }
                *(sym->u.pvalfloat) = (float) (d2);
                break;
            default:
                if (op) {
                    d2 = hoc_opasgn(op, *(OPVAL(sym)), d2);
                }
                *(OPVAL(sym)) = d2;
                break;
            }
        } else {
            int ind;
            switch (sym->subtype) {
            case USERDOUBLE:
                ind = araypt(sym, SYMBOL);
                if (op) {
                    d2 = hoc_opasgn(op, (sym->u.pval)[ind], d2);
                }
                (sym->u.pval)[ind] = d2;
                break;
            case USERINT:
                ind = araypt(sym, SYMBOL);
                if (op) {
                    d2 = hoc_opasgn(op, (double) ((sym->u.pvalint)[ind]), d2);
                }
                (sym->u.pvalint)[ind] = (int) (d2 + hoc_epsilon);
                break;
            case USERFLOAT:
                ind = araypt(sym, SYMBOL);
                if (op) {
                    d2 = hoc_opasgn(op, (double) ((sym->u.pvalfloat)[ind]), d2);
                }
                (sym->u.pvalfloat)[ind] = (float) d2;
                break;
#if NEMO
            case NEMONODE:
                hoc_asgn_nemonode(sym, hoc_xpop(), &d2, op);
                break;
            case NEMOAREA:
                hoc_asgn_nemoarea(sym, hoc_xpop(), &d2, op);
                break;
#endif /*NEMO*/
            default:
                ind = araypt(sym, OBJECTVAR);
                if (op) {
                    d2 = hoc_opasgn(op, (OPVAL(sym))[ind], d2);
                }
                (OPVAL(sym))[ind] = d2;
                break;
            }
        }
        break;
    case AUTO:
        if (op) {
            d2 = hoc_opasgn(op, cast<double>(fp->argn[sym->u.u_auto]), d2);
        }
        fp->argn[sym->u.u_auto] = d2;
        break;
    default:
        execerror("assignment to non-variable", sym->name);
    }
    if (obsav) {
        hoc_objectdata = hoc_objectdata_restore(odsav);
        hoc_thisobject = obsav;
        hoc_symlist = slsav;
    }
    hoc_pushx(d2);
}

void hoc_assign_str(char** cpp, const char* buf) {
    char* s = *cpp;
    *cpp = (char*) emalloc((unsigned) (strlen(buf) + 1));
    Strcpy(*cpp, buf);
    if (s) {
        hoc_free_string(s);
    }
}

void assstr(void) { /* assign string on top to stack - 1 */
    char **ps1, **ps2;

    ps1 = hoc_strpop();
    ps2 = hoc_strpop();
    hoc_assign_str(ps2, *ps1);
}

// returns array string for multiple dimensions
char* hoc_araystr(Symbol* sym, int index, Objectdata* obd) {
    static char name[100];
    char* cp = name + 100;
    char buf[20];
    int i, n, j, n1;

    *--cp = '\0';
    if (ISARRAY(sym)) {
        Arrayinfo* a;
        if (sym->subtype == 0) {
            a = obd[sym->u.oboff + 1].arayinfo;
        } else {
            a = sym->arayinfo;
        }
        for (i = a->nsub - 1; i >= 0; --i) {
            n = a->sub[i];
            j = index % n;
            index /= n;
            Sprintf(buf, "%d", j);
            n1 = strlen(buf);
            assert(n1 + 2 < cp - name);
            *--cp = ']';
            for (j = n1 - 1; j >= 0; --j) {
                *--cp = buf[j];
            }
            *--cp = '[';
        }
    }
    return cp;
}

// Raise error if compile time number of dimensions differs from
// execution time number of dimensions. Program next item is Symbol*.
static void ndim_chk_helper(int ndim) {
    Symbol* sp = (pc++)->sym;
    int ndim_now = sp->arayinfo ? sp->arayinfo->nsub : 0;
    if (ndim_now != ndim) {
        hoc_execerr_ext("array dimension of %s now %d (at compile time it was %d)",
                        sp->name,
                        ndim_now,
                        ndim);
    }
    // if this is missing when hoc_araypt is called, it means the symbol
    // was compiled as a scalar.
    hoc_push_ndim(ndim);
}

void hoc_chk_sym_has_ndim1() {
    ndim_chk_helper(1);
}
void hoc_chk_sym_has_ndim2() {
    ndim_chk_helper(2);
}
void hoc_chk_sym_has_ndim() {
    int ndim = (pc++)->i;
    ndim_chk_helper(ndim);
}

// return subscript - subs in reverse order on stack
int hoc_araypt(Symbol* sp, int type) {
    Arrayinfo* const aray{type == OBJECTVAR ? OPARINFO(sp) : sp->arayinfo};
    int total{};
    int ndim{0};
    if (hoc_stack_type_is_ndim()) {  // if sp compiled as scalar
        ndim = hoc_pop_ndim();       // do not raise error here but below.
    }
    if (ndim != aray->nsub) {
        hoc_execerr_ext("array dimension of %s now %d (at compile time it was %d)",
                        sp->name,
                        aray->nsub,
                        ndim);
    }
    for (int i = 0; i < aray->nsub; ++i) {
        int const d = hoc_look_inside_stack<double>(aray->nsub - 1 - i) + hoc_epsilon;
        if (d < 0 || d >= aray->sub[i]) {
            hoc_execerr_ext(
                "subscript %d index %d of %s out of range %d", i, d, sp->name, aray->sub[i]);
        }
        total = total * (aray->sub[i]) + d;
    }
    for (int i = 0; i < aray->nsub; ++i) {
        hoc_nopop();
    }
    int varn;
    if (hoc_do_equation && sp->s_varn != 0 && (varn = (aray->a_varn)[total]) != 0 &&
        hoc_access[varn] == 0) {
        hoc_access[varn] = hoc_var_access;
        hoc_var_access = varn;
    }
    return total;
}

// pop top value from stack, print it
void print() {
    nrnpy_pr("\t");
    prexpr();
    nrnpy_pr("\n");
}

// print numeric value
void prexpr() {
    static HocStr* s;
    char* ss;
    Object** pob;

    if (!s)
        s = hocstr_create(256);
    switch (hoc_stacktype()) {
    case NUMBER:
        std::snprintf(s->buf, s->size + 1, "%.8g ", hoc_xpop());
        break;
    case STRING:
        ss = *(hoc_strpop());
        hocstr_resize(s, strlen(ss) + 1);
        std::snprintf(s->buf, s->size + 1, "%s ", ss);
        break;
    case OBJECTTMP:
    case OBJECTVAR:
        pob = hoc_objpop();
        std::snprintf(s->buf, s->size + 1, "%s ", hoc_object_name(*pob));
        hoc_tobj_unref(pob);
        break;
    default:
        hoc_execerror("Don't know how to print this type\n", nullptr);
    }
    plprint(s->buf);
}

void prstr(void) /* print string value */
{
    static HocStr* s;
    char** cpp;
    if (!s)
        s = hocstr_create(256);
    cpp = hoc_strpop();
    hocstr_resize(s, strlen(*cpp) + 10);
    std::snprintf(s->buf, s->size + 1, "%s", *cpp);
    plprint(s->buf);
}

/*-----------------------------------------------------------------*/
void hoc_delete_symbol(void)
/* Added 15-JUN-90 by JCW.  This routine deletes a
"defined-on-the-fly" variable from the symbol
list. */
/* modified greatly by Hines. Very unsafe in general. */
{
#if 1
    /*---- local variables -----*/
    Symbol *doomed, *sp;
    /*---- start function ------*/

    /* copy address of the symbol that will be deleted */
    doomed = (pc++)->sym;

#endif
/*	hoc_execerror("delete_symbol doesn't work right now.", (char *)0);*/
#if 1
    if (doomed->type == UNDEF)
        fprintf(stderr, "%s: no such variable\n", doomed->name);
    else if (doomed->defined_on_the_fly == 0)
        fprintf(stderr, "%s: can't be deleted\n", doomed->name);
    else {
        extern void hoc_free_symspace(Symbol*);
        hoc_free_symspace(doomed);
    }
#endif
}

/*----------------------------------------------------------*/

void hoc_newline(void) /* print newline */
{
    plprint("\n");
}

void varread(void) /* read into variable */
{
    double d = 0.0;
    extern NrnFILEWrap* fin;
    Symbol* var = (pc++)->sym;

    assert(var->cpublic != 2);
    if (!((var->type == VAR || var->type == UNDEF) && !ISARRAY(var) && var->subtype == NOTUSER)) {
        execerror(var->name, "is not a scalar variable");
    }
Again:
    switch (nrn_fw_fscanf(fin, "%lf", OPVAL(var))) {
    case EOF:
        if (hoc_moreinput())
            goto Again;
        d = *(OPVAL(var)) = 0.0;
        break;
    case 0:
        execerror("non-number read into", var->name);
        break;
    default:
        d = 1.0;
        break;
    }
    var->type = VAR;
    hoc_pushx(d);
}


static Inst* codechk(void) {
    if (progp >= prog + NPROG - 1)
        execerror("procedure too big", (char*) 0);
    if (zzdebug)
        debugzz(progp);
    return progp++;
}

Inst* Code(Pfrv f) { /* install one instruction or operand */
    progp->pf = f;
    return codechk();
}

Inst* codei(int f) {
    progp->pf = NULL; /* zero high order bits to avoid debugzz problem */
    progp->i = f;
    return codechk();
}

Inst* hoc_codeptr(void* vp) {
    progp->ptr = vp;
    return codechk();
}

void codesym(Symbol* f) {
    progp->sym = f;
    IGNORE(codechk());
}

void codein(Inst* f) {
    progp->in = f;
    IGNORE(codechk());
}

void insertcode(Inst* begin, Inst* end, Pfrv f) {
    Inst* i;
    for (i = end - 1; i != begin; i--) {
        *i = *(i - 1);
    }
    begin->pf = f;

    if (zzdebug) {
        Inst* p;
        printf("insert code: what follows is the entire code so far\n");
        for (p = prog; p < progp; ++p) {
            debugzz(p);
        }
        printf("end of insert code debugging\n");
    }
}

#if defined(DOS) || defined(WIN32)
static int ntimes;
#endif

void execute(Inst* p) /* run the machine */
{
    Inst* pcsav;

    BBSPOLL
    for (pc = p; pc->in != STOP && !hoc_returning;) {
#if defined(DOS)
        if (++ntimes > 10) {
            ntimes = 0;
#if 0
            kbhit(); /* DOS can't capture interrupt when number crunching*/
#endif
        }
#endif
        if (hoc_intset)
            execerror("interrupted", (char*) 0);
        /* (*((pc++)->pf))(); DEC 5000 increments pc after the return!*/
        pcsav = pc++;
        (*((pcsav)->pf))();
    }
}
