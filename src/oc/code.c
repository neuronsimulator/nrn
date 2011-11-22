#include <../../nrnconf.h>
/* /local/src/master/nrn/src/oc/code.c,v 1.37 1999/07/03 14:20:21 hines Exp */

#if defined(__GO32__) 
#include <go32.h>
#endif

#include <errno.h>
#include "hoc.h"
#include "hocstr.h"
#include "parse.h"
#include "equation.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <nrnmpi.h>
#if CABLE
#include "options.h"

int bbs_poll_;
#define BBSPOLL if (--bbs_poll_ == 0) { bbs_handle(); }

int nrn_isecstack();
#else
#define BBSPOLL /**/
#endif

extern Objectdata* hoc_objectdata, *hoc_top_level_data;
extern Object* hoc_thisobject;
extern Symlist* hoc_symlist, *hoc_top_level_symlist;
extern double chkarg();
extern char* hoc_object_name();

# define	STACKCHK	if (stackp >= stacklast) \
					execerror("Stack too deep.", "Increase with -NSTACK stacksize option");

int tstkchk_actual(i, j) int i, j; {
	int k, l;
	char *s[2];
	if (i != j) {
		for (k =0, l=i; k<2; k++, l=j) {
			switch (l) {
			case NUMBER:
				s[k] = "(double)";
				break;
			case STRING:
				s[k] = "(char *)";
				break;
			case OBJECTVAR:
				s[k] = "(Object **)";
				break;
			case USERINT:
				s[k] = "(int)";
				break;
			case SYMBOL:
				s[k] = "(Symbol)";
				break;
			case VAR:
				s[k] = "(double *)";
				break;
			case OBJECTTMP:	/* would use OBJECT if it existed */
				s[k] = "(Object *)";
				break;
			default:
				s[k] = "(Unknown)";
				break;
			}
		}
		fprintf(stderr, "bad stack access: expecting %s; really %s\n", s[1], s[0]);
		execerror("interpreter stack type error", (char *)0);
	}
	return 0;
}

#define USEMACROS 1

#if USEMACROS
#define pushxm(d)	((stackp++)->val = (d));((stackp++)->i = NUMBER)
#define pushsm(d)	((stackp++)->sym = (d));((stackp++)->i = SYMBOL)
#define xpopm()		(tstkchk((--stackp)->i, NUMBER), (--stackp)->val)
#define spopm()		(tstkchk((--stackp)->i, SYMBOL), (--stackp)->sym)
#define nopopm()	(stackp -= 2)
#define tstkchk(i,j)	(((i)!=(j))?tstkchk_actual(i,j):0)
#else
#define pushxm(d) pushx(d)
#define pushsm(d) pushs(d)
#define xpopm() xpop()
#define spopm()	spop()
#define nopopm() nopop()
#define tstkchk(i,j) tstkchk_actual(i,j)
#endif

#define EPS hoc_epsilon
extern double EPS;

#define	NSTACK	1000 /* default size */
#define nstack hoc_nstack
extern long nstack; /* actual size */
static Datum	*stack;	/* the stack */
static Datum	*stackp;	/* next free spot on stack */
static Datum	*stacklast; /* last stack element */

#define	NPROG	5000
Inst	*prog;	/* the machine */
Inst	*progp;		/* next free spot for code generation */
Inst	*pc;		/* program counter during execution */
Inst	*progbase; /* start of current subprogram */
Inst	*prog_parse_recover; /* start after parse error */
int	hoc_returning;	/* 1 if return stmt seen, 2 if break, 3 if continue */
			/* 4 if stop */
typedef struct Frame {	/* proc/func call stack frame */
	Symbol	*sp;	/* symbol table entry */
	Inst	*retpc;	/* where to resume after return */
	Datum	*argn;	/* n-th argument on stack */
	int	nargs;	/* number of arguments */
	Inst	*iter_stmt_begin; /* Iterator statement starts here */
	Object	*iter_stmt_ob;	/* context of Iterator statement */
	Object	*ob;	/* for stack frame debug message */
} Frame;
#define NFRAME	200 /* default size */
#define nframe hoc_nframe
extern long nframe; /* actual size */
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
#define DEBUG_GARBAGE 1
#define TOBJ_POOL_SIZE 50
static Object** hoc_temp_obj_pool_;
static int obj_pool_index_;
static int tobj_count; /* how many stack pushes of OBJECTTMP have been reffed*/

/*
Here is the old comment on the function when it was in hoc_oop.c.

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

Object** hoc_temp_objptr(obj) Object* obj; {
	Object** tobj;
	obj_pool_index_ = (obj_pool_index_ + 1)%TOBJ_POOL_SIZE;
	tobj = hoc_temp_obj_pool_ + obj_pool_index_;
	*tobj = obj;
	return tobj;
}

/* should be called after finished with pointer from a popobj */

hoc_tobj_unref(p) Object** p; {
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
hoc_unref_defer() {
	if (unref_defer_) {
#if 0
		printf("hoc_unref_defer %s %d\n", hoc_object_name(unref_defer_), unref_defer_->refcount);
#endif
		hoc_obj_unref(unref_defer_);
		unref_defer_ = (Object*)0;
	}
}
hoc_pop_defer() {
	Object* obj;
	if (unref_defer_) {
#if 0
		printf("hoc_pop_defer unrefs %s %d\n", hoc_object_name(unref_defer_), unref_defer_->refcount);
#endif
		hoc_unref_defer();
	}
	if (stackp <= stack)
		execerror("stack underflow", (char *) 0);
	if (stackp[-1].i == OBJECTTMP) {
		unref_defer_ = stackp[-2].obj;
		if (unref_defer_) {
			++unref_defer_->refcount;
		}
#if 0
printf("hoc_pop_defer %s %d\n", hoc_object_name(unref_defer_), unref_defer_->refcount);
#endif
	}
	hoc_nopop();
}

/* should be called on each OBJECTTMP on the stack after adjusting the
stack pointer downward */

hoc_stkobj_unref(o) Object* o; {
	--tobj_count;
	hoc_obj_unref(o);
}

/* check the args of the frame and unref any of type OBJECTTMP */

static frameobj_clean(f) Frame* f; {
	Datum* s;
	int i, narg;
	if (f->nargs == 0) {
		return;
	}
	s = f->argn + 2;
	for (i=f->nargs-1; i >= 0; --i) {
		if ((--s)->i == OBJECTTMP) {
			s->i = 0;
			hoc_stkobj_unref((--s)->obj);
		}else{
			--s;
		}
	}
}

extern Symlist *p_symlist;

hoc_init_space()	/* create space for stack and code */
{
	if (nframe == 0) {
		nframe = NFRAME;
	}
	if (nstack == 0) {
		nstack = NSTACK;
	}
	stackp = stack = (Datum *)emalloc(sizeof(Datum)*nstack);
	stacklast = stack + nstack;
	progp = progbase = prog = (Inst *)emalloc(sizeof(Inst)*NPROG);
	fp = frame = (Frame *)emalloc(sizeof(Frame)*nframe);
	framelast = frame + nframe;
	hoc_temp_obj_pool_ = (Object**)emalloc(sizeof(Object*)*TOBJ_POOL_SIZE);
}

#define MAXINITFCNS 10
static int maxinitfcns;
static Pfri initfcns[MAXINITFCNS];

hoc_prstack() {
	int i;
	Datum* s;
	printf("interpreter stack: %ld \n", (stackp - stack)/2);
	for (i=0, s = stackp - 1; s > stack; --s, ++i) {
		if (i > 10) {
			printf("...\n");
			break;
		}
		printf("%d stacktype=%d\n", i, s->i);
		--s;
	}
}

hoc_on_init_register(pf)
	Pfri pf;
{
	/* modules that may have to be cleaned up after an execerror */
	if (maxinitfcns < MAXINITFCNS) {
		initfcns[maxinitfcns++] = pf;
	}else{
		fprintf(stderr, "increase definition for MAXINITFCNS\n");
		nrn_exit(1);
	}
}

initcode()	/* initialize for code generation */
{
	int i;
	extern int hoc_errno_count;
	errno = 0;
	if (hoc_errno_count > 5) {
fprintf(stderr, "errno set %d times on last execution\n", hoc_errno_count);
	}
	hoc_errno_count = 0;
	prog_parse_recover = progbase = prog;
	progp = progbase;
	hoc_unref_defer();

if (tobj_count) {
	while(fp < frame) {
		frameobj_clean(fp);
		--fp;
	}
#if DEBUG_GARBAGE
	if (tobj_count) {
		printf("initcode failed with %d left\n", tobj_count);
	}
#endif
	tobj_count = 0;
}
	stackp = stack;
	fp = frame;
	free_list(&p_symlist);
	hoc_returning = 0;
	do_equation = 0;
	for (i=0; i < maxinitfcns; ++i) {
		(*initfcns[i])();
	}
#if CABLE
	nrn_initcode(); /* special requirements for NEURON */
#endif
}


static Frame *rframe;
static Datum *rstack;
static char *parsestr;

oc_save_code(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12)
	Inst*	*a1;
	Inst*	*a2;
	Datum*	*a3;
	Frame*	*a4;
	int	*a5;
	int	*a6;
	Inst*	*a7;
	Frame*	*a8;
	Datum*	*a9;
	Symlist* *a10;
	Inst*	*a11;
	int	*a12;
{
	*a1 = progbase;
	*a2 = progp;
	*a3 = stackp;
	*a4 = fp;
	*a5 = hoc_returning;
	*a6 = do_equation;
	*a7 = pc;
	*a8 = rframe;
	*a9 = rstack;
	*a10 = p_symlist;
	*a11 = prog_parse_recover;
	*a12 = tobj_count;
}

oc_restore_code(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12)
	Inst*	*a1;
	Inst*	*a2;
	Datum*	*a3;
	Frame*	*a4;
	int	*a5;
	int	*a6;
	Inst*	*a7;
	Frame*	*a8;
	Datum*	*a9;
	Symlist* *a10;
	Inst*	*a11;
	int	*a12;
{
	progbase = *a1;
	progp = *a2;
	if (tobj_count > *a12) {
		while(fp > *a4) {
			frameobj_clean(fp);
			--fp;
		}
#if DEBUG_GARBAGE
if (tobj_count != *a12) {
	printf("oc_restore_code tobj_count=%d should be %d\n", tobj_count, *a12);
}
#endif
	}
	stackp = *a3;
	fp = *a4;
	hoc_returning = *a5;
	do_equation = *a6;
	pc = *a7;
	rframe = *a8;
	rstack = *a9;
	p_symlist = *a10;
	prog_parse_recover = *a11;
}

int hoc_strgets_need() {
	return strlen(parsestr);
}

char *
hoc_strgets(cbuf, nc)	/* getc for a string, used by parser */
	char *cbuf;
	int nc;
{
	strncpy(cbuf, parsestr, nc);
	if (*parsestr == '\0') {
		return (char *)0;
	}else{
		return cbuf;
	}
}

static
rinitcode()	/* initialize for recursive code generation */
{
	extern int hoc_errno_count;
	errno = 0;
	hoc_errno_count = 0;
	prog_parse_recover = progbase;
	progp = progbase;
	stackp = rstack;
	fp = rframe;
	free_list(&p_symlist);
	if (hoc_returning != 4) {	/* if stop not seen */
		hoc_returning = 0;
	}
	do_equation = 0;
}

int hoc_ParseExec(yystart) int yystart; {
	/* can recursively parse and execute what is in cbuf.
	   may parse single tokens. called from hoc_oc(str).
	   All these parse and execute routines should be combined into
	   a single method robust method. The pipeflag method has become
	   encrusted with too many irrelevant mechanisms. There is no longer
	   anything sacred about the cbuf. The only requiremnent is to tell
	   the get line function where to get its string.
	 */
	int yret;
	
	extern int hoc_pipeflag, hoc_in_yyparse;
	Frame *sframe, *sfp;
	Inst *sprogbase, *sprogp, *spc, *sprog_parse_recover;
	Datum *sstackp, *sstack;
	Symlist *sp_symlist;

	if (yystart) {
		sframe=rframe;sfp=fp;
		sprogbase=progbase; sprogp=progp; spc=pc,
		sprog_parse_recover=prog_parse_recover;
		sstackp=stackp; sstack=rstack;
		sp_symlist=p_symlist;
		rframe = fp;
		rstack = stackp;
		progbase = progp;
		p_symlist = (Symlist *)0;
	}
	
	if (yystart) {
		rinitcode();
	}
	if (hoc_in_yyparse) {
		hoc_execerror("Cannot reenter parser.",
		"Maybe you were in the middle of a direct command.");
	}
	yret = yyparse();
	switch (yret) {
	case 1:
		execute(progbase);
		rinitcode();
		break;
	case 'e':
		hoc_edit();
		for (rinitcode(); hoc_yyparse(); rinitcode()) {
			execute(progbase);
		}
		break;
	case -3 :
		hoc_execerror("incomplete statement parse not allowed\n", (char*)0);
	default:
		break;
	}
	if (yystart) {
		rframe=sframe; fp=sfp;
		progbase=sprogbase; progp=sprogp; pc=spc;
		prog_parse_recover=sprog_parse_recover;
		stackp=sstackp; rstack=sstack; p_symlist=sp_symlist;
	}

	return yret;
}
	
int
hoc_xopen_run(sp, str) /*recursively parse and execute for xopen*/
	Symbol *sp;	/* if sp != 0 then parse string and save code */
	char *str;	/*  without executing. Note str must be a 'list'*/
{
	int n=0;
	extern int hoc_pipeflag;
	Frame *sframe=rframe, *sfp=fp;
	Inst *sprogbase=progbase, *sprogp=progp, *spc=pc,
		*sprog_parse_recover=prog_parse_recover;
	Datum *sstackp=stackp, *sstack=rstack;
	Symlist *sp_symlist=p_symlist;
	rframe = fp;
	rstack = stackp;
	progbase = progp;
	p_symlist = (Symlist *)0;
	
	if (sp == (Symbol *)0) {
		for (rinitcode(); hoc_yyparse(); rinitcode())
			execute(progbase);
	}else{ int savpipeflag;
		rinitcode();
		savpipeflag = hoc_pipeflag;
		hoc_pipeflag = 2;
		parsestr=str;
		if(!hoc_yyparse()) {
			execerror("Nothing to parse", (char *)0);
		}
		n = (int)(progp-progbase);
		hoc_pipeflag = savpipeflag;
		hoc_define(sp);
		rinitcode();
	}
	rframe=sframe; fp=sfp;
	progbase=sprogbase; progp=sprogp; pc=spc;
	prog_parse_recover=sprog_parse_recover;
	stackp=sstackp; rstack=sstack; p_symlist=sp_symlist;
	return n;
}

static char* stmp[20];
static int istmp = 0;
char** hoc_temp_charptr() {
	istmp = (istmp+1)%20;
	return stmp+istmp;
}

int
hoc_stack_type() {
	return stackp[-1].i;
}

pushx(d)		/* push double onto stack */
	double d;
{
	STACKCHK
	(stackp++)->val = d;
	(stackp++)->i = NUMBER;
}

hoc_pushobj(d)		/* push pointer to object pointer onto stack */
	Object **d;
{
	STACKCHK
	if (d >= hoc_temp_obj_pool_ && d < (hoc_temp_obj_pool_ + TOBJ_POOL_SIZE)) {
		hoc_push_object(*d);
		return;
	}
	(stackp++)->pobj = d;
	(stackp++)->i = OBJECTVAR;
}

hoc_push_object(d)		/* push pointer to object onto stack */
	Object *d;
{
	STACKCHK
	(stackp++)->obj = d;
	(stackp++)->i = OBJECTTMP;	/* would use OBJECT if it existed */
	hoc_obj_ref(d);
	++tobj_count;
}

hoc_pushstr(d)		/* push pointer to string pointer onto stack */
	char **d;
{
	STACKCHK
	(stackp++)->pstr = d;
	(stackp++)->i = STRING;
}

hoc_push_string() {	/* code for pushing a symbols string */
	Objectdata* odsav;
	Object* obsav = 0;
	Symlist* slsav;
	Symbol *s;
	s = (pc++)->sym;
	if (!s) {
		hoc_pushstr((char**) 0);
		return;
	}
	if (s->type == CSTRING) {
		hoc_pushstr(&(s->u.cstr));
	}else{
    if (s->public == 2) {
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

hoc_pushpx(d)		/* push double pointer onto stack */
	double *d;
{
	STACKCHK
	(stackp++)->pval = d;
	(stackp++)->i = VAR;
}

pushs(d)		/* push symbol pointer onto stack */
	Symbol * d;
{
	STACKCHK
	(stackp++)->sym = d;
	(stackp++)->i = SYMBOL;
}
pushi(d)		/* push integer onto stack */
	int d;
{
	STACKCHK
	(stackp++)->i = d;
	(stackp++)->i = USERINT;
}

int
hoc_stacktype() {
	if (stackp <= stack) {
		execerror("stack empty", (char*)0);
	}
	return (stackp - 1)->i;
}

int hoc_argtype(narg) /* type of nth arg */
	int narg;
{
	if (narg > fp->nargs)
		execerror(fp->sp->name, "not enough arguments");
	return(fp->argn[(narg - fp->nargs)*2 + 1].i);
}

int hoc_is_double_arg(narg) int narg; {
	return (hoc_argtype(narg) == NUMBER);
}

int hoc_is_pdouble_arg(narg) int narg; {
	return (hoc_argtype(narg) == VAR);
}

int hoc_is_str_arg(narg) int narg; {
	return (hoc_argtype(narg) == STRING);
}

int hoc_is_object_arg(narg) int narg; {
	int type = hoc_argtype(narg);
	return (type == OBJECTVAR || type == OBJECTTMP);
}

int hoc_is_tempobj_arg(narg) int narg; {
	return (hoc_argtype(narg) == OBJECTTMP);
}

Datum *
hoc_look_inside_stack(i, type) /* stack pointer at depth i; i=0 is top */
	int i;
	int type;
{
	tstkchk((stackp - 2*i - 1)->i, type);
	return stackp - 2*(i + 1);
}

Object*
hoc_obj_look_inside_stack(i) /* stack pointer at depth i; i=0 is top */
	int i;
{
	Datum* d = stackp - 2*i - 2;
	int type = d[1].i;
	if (type == OBJECTTMP) {
		return d[0].obj;
	}
	tstkchk(type,  OBJECTVAR);
	return *(d[0].pobj);
}

int hoc_inside_stacktype(int i) { /* 0 is top */
	return (stackp - 2*i - 1)->i;
}

double
xpop()		/* pop double and return top elem from stack */
{
	if (stackp <= stack)
		execerror("stack underflow", (char *) 0);
	--stackp;
	tstkchk(stackp->i, NUMBER);
	return (--stackp)->val;
}

#if 0
pstack() {
	char* hoc_object_name();
	Datum* d;
	int i;
	for (d=stackp; d > stack;) {
		i = (--d)->i;
		--d;
		switch(i) {
			case NUMBER:
				printf("(double)\n");
				break;
			case STRING:
				printf("(char *)\n");
				break;
			case OBJECTVAR:
				printf("(Object **) %s\n", hoc_object_name(*(d->pobj)));
				break;
			case USERINT:
				printf("(int)\n");
				break;
			case SYMBOL:
				printf("(Symbol) %s\n", d->sym);
				break;
			case VAR:
				printf("(double *)\n");
				break;
			case OBJECTTMP:	/* would use OBJECT if it existed */
				printf("(Object *) %s\n", hoc_object_name(d->obj));
				break;
			default:
				printf("(Unknown)\n");
				break;
		}
	}
}
#endif

double *
hoc_pxpop()		/* pop double pointer and return top elem from stack */
{
	if (stackp <= stack)
		execerror("stack underflow", (char *) 0);
	--stackp;
	tstkchk(stackp->i, VAR);
	return (--stackp)->pval;
}

Symbol *
spop()		/* pop symbol pointer and return top elem from stack */
{
	if (stackp <= stack)
		execerror("stack underflow", (char *) 0);
	--stackp;
	tstkchk(stackp->i, SYMBOL);
	return (--stackp)->sym;
}

/*
When using objpop, after dealing with the pointer, one should call
	hoc_tobj_unref(pobj) in order to prevent memory leakage since
	the object may have been reffed when it was pushed on the stack
*/

Object **
hoc_objpop()		/* pop pointer to object pointer and return top elem from stack */
{
	if (stackp <= stack)
		execerror("stack underflow", (char *) 0);
	--stackp;
	if (stackp->i == OBJECTTMP) {
		return hoc_temp_objptr((--stackp)->obj);
	}
	tstkchk(stackp->i, OBJECTVAR);
	return (--stackp)->pobj;
}

Object *
hoc_pop_object()		/* pop object and return top elem from stack */
{
	if (stackp <= stack)
		execerror("stack underflow", (char *) 0);
	--stackp;
	tstkchk(stackp->i, OBJECTTMP);
	return (--stackp)->obj;
}

char **
hoc_strpop()		/* pop pointer to string pointer and return top elem from stack */
{
	if (stackp <= stack)
		execerror("stack underflow", (char *) 0);
	--stackp;
	tstkchk(stackp->i, STRING);
	return (--stackp)->pstr;
}

int
ipop()		/* pop symbol pointer and return top elem from stack */
{
	if (stackp <= stack)
		execerror("stack underflow", (char *) 0);
	--stackp;
	tstkchk(stackp->i, USERINT);
	return (--stackp)->i;
}

nopop()	/* just pop the stack without returning anything */
{
	
	if (stackp <= stack)
		execerror("stack underflow", (char *) 0);
	--stackp;
	if (stackp->i == OBJECTTMP) {
		stackp->i = 0;
		hoc_stkobj_unref((--stackp)->obj);
	}else{
		--stackp;
	}
}

constpush()	/* push constant onto stack */
{
	pushxm(*((pc++)->sym)->u.pnum);
}

pushzero()	/* push zero onto stack */
{
	pushxm(0.);
}

varpush()	/* push variable onto stack */
{
	pushsm((pc++)->sym);
}

# define	relative(pc)	(pc + (pc)->i)

forcode()
{
	double d;
	Inst *savepc = pc;	/* loop body */
	int isec;

#if CABLE
	isec = nrn_isecstack();
#endif
	execute(savepc+3);	/* condition */
	d = xpopm();
	while (d)
	{
		execute(relative(savepc));	/* body */
#if CABLE
		if (hoc_returning) {nrn_secstack(isec);}
#endif
		if (hoc_returning == 1 || hoc_returning == 4)	/* return or stop */
			break;
		else if (hoc_returning == 2)	/* break */
		{
			hoc_returning = 0;
			break;
		}
		else				/* continue */
			hoc_returning = 0;
		if ((savepc+2)->i)   /* diff between while and for */
			execute(relative(savepc+2)); /* increment */
		execute(savepc+3);
		d = xpopm();
	}
	if (!hoc_returning)
		pc = relative(savepc+1);	/* next statement */
}

shortfor()
{
	Inst *savepc = pc;
	double begin, end, *pval;
	Symbol *sym;
	int isec;
	
	end = xpopm() + EPS;
	begin = xpopm();
	sym = spopm();

	switch (sym->type) {
	case UNDEF:
		hoc_execerror(sym->name, "undefined variable");
	case VAR:
if (!ISARRAY(sym)) {
		if (sym->subtype == USERINT) {
			execerror("integer iteration variable", sym->name);
		}else if (sym->subtype == USERDOUBLE) {
			pval = sym->u.pval;
		}else{
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
	case AUTO:
		pval = &(fp->argn[sym->u.u_auto*2].val);
		break;
	default:
		execerror("for loop non-variable", sym->name);
	}
#if CABLE
	isec = nrn_isecstack();
#endif
	for (*pval = begin; *pval<=end; *pval += 1.) {
		execute(relative(savepc));
#if CABLE
		if (hoc_returning) {nrn_secstack(isec);}
#endif
		if (hoc_returning == 1 || hoc_returning == 4) {
			break;
		}else if (hoc_returning == 2) {
			hoc_returning = 0;
			break;
		}else{
			hoc_returning = 0;
		}
	}
	if (!hoc_returning)
		pc =relative(savepc+1);
}

hoc_iterator() {
	/* pc is ITERATOR symbol, argcount, stmtbegin, stmtend */
	/* for testing execute stmt once */
	Symbol* sym;
	int argcount;
	Inst* stmtbegin, *stmtend;
	
	sym = (pc++)->sym;
	argcount = (pc++)->i;
	stmtbegin = relative(pc);
	stmtend = relative(pc+1);;
	hoc_iterator_object(sym, argcount, stmtbegin, stmtend, hoc_thisobject);
}

hoc_iterator_object(sym, argcount, beginpc, endpc, ob)
	Symbol* sym;
	int argcount;
	Inst* beginpc;
	Inst* endpc;
	Object* ob;
{
	int i;
	fp++;
	if (fp >= framelast) {
		fp--;
		execerror(sym->name, "call nested too deeply, increase with -NFRAME framesize option");
	}
	fp->sp = sym;
	fp->nargs = argcount;
	fp->retpc = endpc;
	fp->argn = stackp - 2;
	stackp += sym->u.u_proc->nauto * 2;
	/* clear the autoobject pointers */
	for (i = sym->u.u_proc->nobjauto; i > 0; --i) {
		stackp[-2*i].obj = (Object*)0;
	}
	fp->iter_stmt_begin = beginpc;
	fp->iter_stmt_ob = ob;
	fp->ob = ob;
	STACKCHK
	execute(sym->u.u_proc->defn.in);
	nopop(); /* 0.0 from the procret() */
	if (hoc_returning != 4) {
		hoc_returning = 0;
	}
}

hoc_iterator_stmt() {
	Inst* pcsav;
	Object* ob;
	Object* obsav;
	Objectdata* obdsav;
	Symlist* slsav;
	int isec;
	Frame* iter_f = fp;		/* iterator frame */
	Frame* ef = fp - 1;	/* iterator statement frame */
	fp++;			/* execution frame */

	fp->sp = iter_f->sp;
	fp->ob = iter_f->ob;
	if (ef != frame) {
		/*SUPPRESS 26*/
		fp->argn = ef->argn;
		fp->nargs = ef->nargs;
	}else{  /* top. only for stack trace */
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
		hoc_symlist = ob->template->symtable;
	}else{
		hoc_objectdata = hoc_top_level_data;
		hoc_symlist = hoc_top_level_symlist;
	}

	pcsav = pc;
#if CABLE
	isec = nrn_isecstack();
#endif
	execute(iter_f->iter_stmt_begin);
	pc = pcsav;
	hoc_objectdata = hoc_objectdata_restore(obdsav);
	hoc_thisobject = obsav;
	hoc_symlist = slsav;
	--fp;
#if CABLE
	if (hoc_returning) {nrn_secstack(isec);}
#endif
	switch (hoc_returning) {
	case 1: /* return means not only return from iter but return from
			the procedure containing the iter statement */
hoc_execerror("return from within an iterator statement not allowed.",
"Set a flag and use break.");
	case 2:	/* break means return from iter */
		procret();
		break;
	case 3:	/* continue means go on from iter as though nothing happened*/
		hoc_returning = 0;
		break;
	}

}

static for_segment2(sym, mode) Symbol* sym; int mode; {
	/* symbol on stack; statement pointed to by pc
	continuation pointed to by pc+1. template used is shortfor in code.c
	of hoc system.
	*/
	
#if CABLE
	int i, imax;
	Inst *savepc = pc;
	double *pval, dx;
	int isec;
#if METHOD3
	extern int _method3;
#endif
	
	switch (sym->type) {
	case UNDEF:
		hoc_execerror(sym->name, "undefined variable");
	case VAR:
if (!ISARRAY(sym)) {
		if (sym->subtype == USERINT)
			execerror("integer iteration variable", sym->name);
		else if (sym->subtype == USERDOUBLE)
			pval = sym->u.pval;
		else
			pval = OPVAL(sym);
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
	case AUTO:
		pval = &(fp->argn[sym->u.u_auto*2].val);
		break;
	default:
		execerror("for loop non-variable", sym->name);
	}
	imax = segment_limits(&dx);
#if METHOD3
   if (_method3) {
	for (i=0, *pval=0; i <= imax; i++) {
		if (mode == 0 && (i == imax || i == 0)) { continue; }
		if (i == imax) {
			*pval = 1.;
		} else {
			*pval = i * dx;
		}
		execute(relative(savepc));
		if (hoc_returning == 1 || hoc_returning == 4) {
			break;
		}else if (hoc_returning == 2) {
			hoc_returning = 0;
			break;
		}else{
			hoc_returning = 0;
		}
	}
   }else
#endif
   {
	if (mode == 0) {
		i = 1;
		*pval = dx/2.;
	}else{
		i = 0;
		*pval = 0.;
	}
#if CABLE
	isec = nrn_isecstack();
#endif
	for (; i <= imax; i++) {
		if (i == imax) {
			if (mode == 0) { continue; }
			*pval = 1.;
		}
		execute(relative(savepc));
#if CABLE
		if (hoc_returning) {nrn_secstack(isec);}
#endif
		if (hoc_returning == 1 || hoc_returning == 4) {
			break;
		}else if (hoc_returning == 2) {
			hoc_returning = 0;
			break;
		}else{
			hoc_returning = 0;
		}
		if (i == 0) {
			*pval += dx/2.;
		}
		else if (i < imax) {
			*pval += dx;
		}
	}
   }
	if (!hoc_returning)
		pc =relative(savepc+1);
#else
	execerror("for (var) {stmt} syntax only allowed in CABLE", (char *)0);
#endif
}
	
for_segment() {
	for_segment2(spopm(), 1);
}

for_segment1() {
	Symbol* sym;
	double d;
	int mode;
	d = xpopm();
	sym = spopm();
	mode = (fabs(d)< EPS) ? 0 : 1;
	for_segment2(sym, mode);	
}

ifcode()
{
	double d;
	Inst *savepc = pc;	/* then part */

	execute(savepc+3);	/* condition */
	d = xpopm();
	if (d)
		execute(relative(savepc));
	else if ((savepc+1)->i)	/* else part? */
		execute(relative(savepc+1));
	if (!hoc_returning)
		pc = relative(savepc+2);	/* next stmt */
}

Break()		/* break statement */
{
	hoc_returning = 2;
}

Continue()	/* continue statement */
{
	hoc_returning = 3;
}

Stop()		/* stop statement */
{
	hoc_returning = 4;
}

hoc_define(sp)	/* put func/proc in symbol table */
	Symbol *sp;
{
	Inst *inst, *newinst;

	if (sp->u.u_proc->defn.in != STOP)
		free((char *) sp->u.u_proc->defn.in);
	free_list(&(sp->u.u_proc->list));
	sp->u.u_proc->list = p_symlist;
	p_symlist = (Symlist *)0;
	sp->u.u_proc->size =(unsigned)(progp - progbase);
	sp->u.u_proc->defn.in = (Inst *) emalloc((unsigned)(progp-progbase)*sizeof(Inst));
	newinst = sp->u.u_proc->defn.in;
	for (inst = progbase; inst != progp; )
		*newinst++ = *inst++;
	progp = progbase;	/* next code starts here */
}

frame_debug()	/* print the call sequence on an execerror */
{
	Frame *f;
	int i, j;
	char id[10];

	if (nrnmpi_numprocs_world > 1) {
		sprintf(id, "%d ", nrnmpi_myid_world);
	}else{
		id[0]='\0';
	}
	for (i=5, f=fp; f != frame && --i; f=f-1) { /* print only to depth of 5 */
		for (j=i; j; j--) {
#ifdef WIN32
			printf("  ");
#else
			IGNORE(fputs("  ",stderr));
#endif
		}
		if (f->ob) {
			Fprintf(stderr, "%s%s.%s(", id, hoc_object_name(f->ob), f->sp->name);
		}else{
			Fprintf(stderr, "%s%s(", id, f->sp->name);
		}
		for (j=1; j<=f->nargs;) {
		   switch(f->argn[(j - f->nargs)*2 + 1].i) {
		   case NUMBER:
			Fprintf(stderr, "%g", f->argn[(j - f->nargs)*2].val);
			break;
		   case STRING:
		   	Fprintf(stderr, "\"%s\"", *f->argn[(j - f->nargs)*2].pstr);
			break;
		   case OBJECTVAR:
Fprintf(stderr, "%s", hoc_object_name(*f->argn[(j - f->nargs)*2].pobj));
			break;
		   default:
		   	Fprintf(stderr, "...");
		   	break;
		   }
			if (++j <= f->nargs) {
#ifdef WIN32
				printf(", ");
#else
				IGNORE(fputs(", ", stderr));
#endif
			}
		}
#ifdef WIN32
		printf(")\n");
#else
		IGNORE(fputs(")\n", stderr));
#endif
	}
	if (i <= 0) {
		Fprintf(stderr, "and others\n");
	}
}

push_frame(sp, narg) /* helpful for explicit function calls */
	Symbol* sp;
	int narg;
{
	if (++fp >= framelast) {
		--fp;
		execerror(sp->name, "call nested too deeply, increase with -NFRAME framesize option");
	}
	fp->sp = sp;
	fp->nargs = narg;
	fp->argn = stackp - 2;	/* last argument */
	fp->ob = hoc_thisobject;
}

pop_frame() {
	int i;
	frameobj_clean(fp);
	for (i = 0; i < fp->nargs; i++)
		nopopm();	/* pop arguments */
	--fp;
}

call()		/* call a function */
{
	int i, isec;
	Symbol *sp = pc[0].sym;	/* symbol table entry */
					/* for function */
	if (++fp >= framelast) {
		--fp;
		execerror(sp->name, "call nested too deeply, increase with -NFRAME framesize option");
	}
	fp->sp = sp;
	fp->nargs = pc[1].i;
	fp->retpc = pc + 2;
	fp->ob = hoc_thisobject;
	/*SUPPRESS 26*/
	fp->argn = stackp - 2;	/* last argument */
	BBSPOLL
#if CABLE
	isec = nrn_isecstack();
#endif
	if (sp->type == FUN_BLTIN || sp->type == OBJECTFUNC || sp->type == STRINGFUNC) {
		stackp += sp->u.u_proc->nauto * 2;	/* Offset stack for auto space */
		STACKCHK
		(*(sp->u.u_proc->defn.pf))();
		if (hoc_errno_check()) {
			hoc_warning("errno set during call of", sp->name);
		}
	} else if ((sp->type == FUNCTION || sp->type == PROCEDURE || sp->type == HOCOBJFUNCTION)
		&& sp->u.u_proc->defn.in != STOP) {
		stackp += sp->u.u_proc->nauto * 2;	/* Offset stack for auto space */
		STACKCHK
		/* clear the autoobject pointers. */
		for (i = sp->u.u_proc->nobjauto; i > 0; --i) {
			stackp[-2*i].obj = (Object*)0;
		}
		if (sp->public == 2) {
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
		}else{
			execute(sp->u.u_proc->defn.in);
		}
		/* the autoobject pointers were unreffed at the ret() */
		
	} else {
		execerror(sp->name, "undefined function");
	}
#if CABLE
	if (hoc_returning) {nrn_secstack(isec);}
#endif
	if (hoc_returning != 4) {	/*if not stopping */
		hoc_returning = 0;
	}
}

hoc_fake_call(s) Symbol* s; {
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
	fp->ob = 0;
}

double hoc_call_func(s, narg)
	Symbol* s;
	int narg;
{
	/* call the symbol as a function, The args better be pushed on the stack
	first arg first. */
	if (s->type == BLTIN) {
		return (*(s->u.ptr))(xpop());
	}else{
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

ret()		/* common return from func, proc, or iterator */
{
	int i;
	/* unref all the auto object pointers */
	for (i = fp->sp->u.u_proc->nobjauto; i > 0; --i) {
		hoc_obj_unref(stackp[-2*i].obj);
	}
	stackp -= fp->sp->u.u_proc->nauto * 2;	/* Pop off the autos */
	frameobj_clean(fp);
	for (i = 0; i < fp->nargs; i++)
		nopopm();	/* pop arguments */
	pc = (Inst *)fp->retpc;
	--fp;
	hoc_returning = 1;
}

funcret()	/* return from a function */
{
	double d;
	if (fp->sp->type != FUNCTION)
		execerror(fp->sp->name, "(proc or iterator) returns value");
	d = xpopm();	/* preserve function return value */
	ret();
	pushxm(d);
}

procret()	/* return from a procedure */
{
	if (fp->sp->type == FUNCTION)
		execerror(fp->sp->name,
			"(func) returns no value");
	if (fp->sp->type == HOCOBJFUNCTION)
		execerror(fp->sp->name,
			"(obfunc) returns no value");
	ret();
	pushxm(0.);	/*will be popped immediately; necessary because caller
					may have compiled it as a function*/
}

hocobjret()	/* return from a hoc level obfunc */
{
	Object** d;
	if (fp->sp->type != HOCOBJFUNCTION)
		execerror(fp->sp->name, "objfunc returns objref");
	d = hoc_objpop();	/* preserve function return value */
	if (*d) { (*d)->refcount++;}
	ret();
	/*make a temp and ref it in case autoobj returned since ret would
	have unreffed it*/
	hoc_push_object(*d);
	
	if (*d) { (*d)->refcount--;}
	hoc_tobj_unref(d);
}

hoc_Numarg()
{
	int narg;
	Frame* f = fp - 1;
	if (f == frame) {
		narg = 0;
	}else{
		narg = f->nargs;
	}
	ret();
	pushxm((double)narg);
}

hoc_Argtype()
{
	int narg, iarg, type, itype;
	Frame* f = fp - 1;
	if (f == frame) {
		execerror("argtype can only be called in a func or proc", 0);
	}
	iarg = (int)chkarg(1, -1000., 100000.);
	if (iarg > f->nargs || iarg < 1) {
		itype = -1;
	}else{
		type = (f->argn[(iarg - f->nargs)*2 + 1].i);
		switch (type) {
			case NUMBER: itype = 0; break;
			case OBJECTVAR:
			case OBJECTTMP: itype = 1; break;
			case STRING: itype = 2; break;
			case VAR: itype = 3; break;
		}
	}
	ret();
	pushxm((double)itype);
}

int
ifarg(narg)	/* true if there is an nth argument */
	int narg;
{
	if (narg > fp->nargs)
		return 0;
	return 1;
}

Object **
hoc_objgetarg(narg)	/* return pointer to nth argument */
	int narg;
{
	Datum* d;
	if (narg > fp->nargs)
		execerror(fp->sp->name, "not enough arguments");
	d = fp->argn + (narg - fp->nargs)*2;
	if (d[1].i == OBJECTTMP) {
		return hoc_temp_objptr(d[0].obj);
	}
	tstkchk(d[1].i, OBJECTVAR);
	return d[0].pobj;
}

char **
hoc_pgargstr(narg)	/* return pointer to nth argument */
	int narg;
{
	char ** cpp;
	Symbol *sym;
	int type;
	if (narg > fp->nargs)
		execerror(fp->sp->name, "not enough arguments");
	type = fp->argn[(narg - fp->nargs)*2 + 1].i;
   if (type == STRING) {
	cpp = fp->argn[(narg - fp->nargs)*2].pstr;
   }else if (type != SYMBOL) {
	execerror("Expecting string argument", (char *)0);
   }else{
	sym = fp->argn[(narg - fp->nargs)*2].sym;
	if (sym->type == CSTRING) {
		cpp = &sym->u.cstr;
	}else if (sym->type == STRING) {
		cpp = OPSTR(sym);
	}else{
		execerror("Expecting string argument", (char *)0);
	}
   }
	return cpp;
}

double* 
hoc_pgetarg(narg)	/* return pointer to nth argument */
	int narg;
{
	if (narg > fp->nargs)
		execerror(fp->sp->name, "not enough arguments");
	tstkchk(fp->argn[(narg - fp->nargs)*2 + 1].i, VAR);
	return fp->argn[(narg - fp->nargs)*2].pval;
}

double *
getarg(narg)	/* return pointer to nth argument */
	int narg;
{
	if (narg > fp->nargs)
		execerror(fp->sp->name, "not enough arguments");
#if 1
	tstkchk(fp->argn[(narg - fp->nargs)*2 + 1].i, NUMBER);
#endif
	return &fp->argn[(narg - fp->nargs)*2].val;
}

int hoc_argindex() {
	int j;
	j = (int)xpopm();
	if (j < 1) {
		hoc_execerror("arg index i < 1", 0);
	}
	return j;
}

arg()	/* push argument onto stack */
{
	int i;
	i = (pc++)->i;
	if (i == 0) {
		i = hoc_argindex();
	}
	pushxm( *getarg(i));
}

hoc_stringarg() /* push string arg onto stack */
{
	int i;
	i = (pc++)->i;
	if (i == 0) {
		i = hoc_argindex();
	}
	hoc_pushstr(hoc_pgargstr(i));
}

double hoc_opasgn(op, dest, src)
	int op;
	double dest, src;
{
	switch (op) {
	case '+':
		return dest + src;
	case '*':
		return dest * src;
	case '-':
		return dest - src;
	case '/':
		if (src == 0.) {
			hoc_execerror("Divide by 0", (char*)0);
		}
		return dest / src;
	default:
		return src;
	}
}
argassign()	/* store top of stack in argument */
{
	double d;
	int i, op;
	i = (pc++)->i;
	if (i == 0) {
		i = hoc_argindex();
	}
	op = (pc++)->i;
	d = xpopm();
	if (op) {
		d = hoc_opasgn(op, *getarg(i), d);
	}
	pushxm(d);	/* leave value on stack */
	*getarg(i) = d;
}

hoc_argrefasgn() {
	double d, *pd;
	int i, j, op;
	i = (pc++)->i;
	j = (pc++)->i;
	if (i == 0) {
		i = hoc_argindex();
	}
	op = (pc++)->i;
	d = xpopm();
	if (j) {
		j = (int)(xpopm() + EPS);
	}
	pd = hoc_pgetarg(i);
	if (op) {
		d = hoc_opasgn(op, pd[j], d);
	}
	pushxm(d);	/* leave value on stack */
	pd[j] = d;
}

hoc_argref() {
	int i, j;
	double* pd;
	i = (pc++)->i;
	j = (pc++)->i;
	if (i == 0) {
		i = hoc_argindex();
	}
 	pd = hoc_pgetarg(i);
	if (j) {
		j = (int)(xpopm() + EPS);
	}
 	pushxm(pd[j]);
}

hoc_argrefarg() {
	double* pd;
	int i;
	i = (pc++)->i;
	if (i == 0) {
		i = hoc_argindex();
	}
	pd = hoc_pgetarg(i);
	hoc_pushpx(pd);
}

bltin()		/* evaluate built-in on top of stack */
{
	double d;
	d = xpopm();
	d = (*((pc++)->sym->u.ptr))(d);
	pushxm(d);
}

Symbol* hoc_get_symbol(var)
	char* var;
{
	Symlist *sl = (Symlist *)0;
	Symbol *prc, *sym, *hoc_parse_stmt();
	int eval(), rangepoint(), rangevareval(), hoc_object_eval();
	Inst* last;
	prc = hoc_parse_stmt(var, &sl);
	hoc_run_stmt(prc);

	last = (Inst*)prc->u.u_proc->defn.in + prc->u.u_proc->size - 1;
	if (last[-2].pf == eval) {
		sym = last[-3].sym;
	}else if (last[-3].pf == rangepoint || last[-3].pf == rangevareval) {
		sym = last[-2].sym;
	}else if (last[-4].pf == hoc_object_eval) {
		sym = last[-10].sym;
	}else{
		sym = (Symbol*)0;
	}
	free_list(&sl);
	return sym;
}

Symbol* hoc_get_last_pointer_symbol() {/* hard to imagine a kludgier function*/
	Symbol* sym = (Symbol*)0;
	int hoc_ob_pointer(), rangevarevalpointer(), hoc_evalpointer(); 
	Inst* pcv;
	int istop=0;
	for (pcv = pc; pcv; --pcv) {
		if (pcv->pf == hoc_ob_pointer) {
			if (pcv[-2].sym) {
				sym = pcv[-2].sym; /* e.g. &ExpSyn[0].A */
			}else{
				sym = pcv[-6].sym; /* e.g. & Cell[0].soma.v(.5) */
			}
			break;
		}else if (pcv->pf == hoc_evalpointer) {
			sym = pcv[-1].sym;
			break;
		}else if (pcv->pf == rangevarevalpointer) {
			sym = pcv[1].sym;
			break;
		}else if (pcv->in == STOP) {
/* hopefully only got here from python. Give up on second STOP*/
			if (istop++ == 1) {
				break;
			}
		}
	}
	return sym;
}

hoc_autoobject() { /* AUTOOBJ symbol at pc+1. */
		  /* pointer to object pointer left on stack */
	int i;
	Symbol *obs;
	Object **obp;
#if PDEBUG
	printf("code for hoc_autoobject()\n");
#endif
	obs = (pc++)->sym;
	hoc_pushobj(&(fp->argn[obs->u.u_auto*2].obj));
}

eval()			/* evaluate variable on stack */
{
	Objectdata* odsav;
	Object* obsav = 0;
	Symlist* slsav;
	double d, cable_prop_eval();
	Symbol *sym;
	sym = spopm();
    if (sym->public == 2) {
	sym = sym->u.sym;
	odsav = hoc_objectdata_save();
	obsav = hoc_thisobject;
	slsav = hoc_symlist;
	hoc_objectdata = hoc_top_level_data;
	hoc_thisobject = 0;
	hoc_symlist = hoc_top_level_symlist;
    }
	switch (sym->type)
	{
	case UNDEF:
		execerror("undefined variable", sym->name);
	case VAR:
if (!ISARRAY(sym)) {
		if (do_equation && sym->s_varn > 0
		&& hoc_access[sym->s_varn] == 0)
		{
			hoc_access[sym->s_varn] = var_access;
			var_access = sym->s_varn;
		}
		switch (sym->subtype) {
		case USERDOUBLE:
			d = *(sym->u.pval);
			break;
		case USERINT:
			d = (double)(*(sym->u.pvalint));
			break;
#if	CABLE
		case USERPROPERTY:
			d = cable_prop_eval(sym);
			break;
#endif
		case USERFLOAT:
			d = (double)(*(sym->u.pvalfloat));
			break;
		default:
			d = *(OPVAL(sym));
			break;
		}
}else {
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
			hoc_eval_nemonode(sym, xpopm(), &d);
			break;
		case NEMOAREA:
			hoc_eval_nemoarea(sym, xpopm(), &d);
			break;
#endif /*NEMO*/
		default:
			d = (OPVAL(sym))[araypt(sym, OBJECTVAR)];
			break;
		}
}
		break;
	case AUTO:
		d = fp->argn[sym->u.u_auto*2].val;
		break;
	default:
		execerror("attempt to evaluate a non-variable", sym->name);
	}

    if (obsav) {
	hoc_objectdata = hoc_objectdata_restore(odsav);
	hoc_thisobject = obsav;
	hoc_symlist = slsav;
    }
	pushxm(d);
}

hoc_evalpointer()			/* leave pointer to variable on stack */
{
	Objectdata* odsav;
	Object* obsav = 0;
	Symlist* slsav;
	double *d, *cable_prop_eval_pointer();
	Symbol *sym;
	sym = spopm();
    if (sym->public == 2) {
	sym = sym->u.sym;
	odsav = hoc_objectdata_save();
	obsav = hoc_thisobject;
	slsav = hoc_symlist;
	hoc_objectdata = hoc_top_level_data;
	hoc_thisobject = 0;
	hoc_symlist = hoc_top_level_symlist;
    }
	switch (sym->type)
	{
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
#if	CABLE
		case USERPROPERTY:
			d = cable_prop_eval_pointer(sym);
			break;
#endif
		default:
			d = OPVAL(sym);
			break;
		}
}else {
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
	case AUTO:
#if 0
		execerror("can't use pointer to local variable", sym->name);
#else
		d = &(fp->argn[sym->u.u_auto*2].val);
#endif
		break;
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

add()		/* add top two elems on stack */
{
	double d1, d2;
	d2 = xpopm();
	d1 = xpopm();
	d1 += d2;
	pushxm(d1);
}

sub()		/* subtract top two elems on stack */
{
	double d1, d2;
	d2 = xpopm();
	d1 = xpopm();
	d1 -= d2;
	pushxm(d1);
}

mul()		/* multiply top two elems on stack */
{
	double d1, d2;
	d2 = xpopm();
	d1 = xpopm();
	d1 *= d2;
	pushxm(d1);
}

#if _CRAY
/*
  try to do integer division, so that if x is an exact multiple of y
  then we really get an integer as the result.
  Algorithm: find n such that tx = x * 10^n and ty = y * 10^n are both
  integral.  If tx/ty leaves no remainder, then tx/ty is the correct
  answer and is stored in iptr, intdiv returns true.  Otherwise a
  floating point division can be done, intdiv returns false.
*/   

static int intdiv(x, y, iptr)
     double x, y;
     int * iptr;
{
  long ix, iy, iz;
  int done = 0;
  while (!done)
    {
      if (fabs(x) > (1<<62) || fabs(y) > (1<<62))
      return 0;               /* out of range of integers */
      if (x == (long) x && y == (long) y)
      done = 1;
      else
      {
        x *= (long double) 10;
        y *= (long double) 10;
      }
    }
  ix = (long) x;
  iy = (long) y;
  iz = ix/iy;
  if (ix == iz*iy)
    {                         /* no remainder */
      *iptr = (int) iz;
      return 1;
    }
  return 0;
}
#endif

hoc_div()		/* divide top two elems on stack */
{
	double d1, d2;
	d2 = xpopm();
	if (d2 == 0.0)
		execerror("division by zero", (char *) 0);
	d1 = xpopm();
#if _CRAY
      {
        int i;
        if (intdiv(d1, d2, &i))
          d1 = (int) i;       /* result is an integer */
        else
          d1 = d1/d2;         /* result is not an integer */
      }
#else
      d1 /= d2;
#endif
	pushxm(d1);
}

hoc_cyclic()	/* the modulus function */
{
	double d1, d2;
	double r, q;
	d2 = xpopm();
	if (d2 <= 0.)
		execerror("a%b, b<=0", (char *) 0);
	d1 = xpopm();
	r = d1;
	if ( r >= d2) {
		q = floor(d1/d2);
		r = d1 - q*d2;
	}
	else if ( r <= -d2) {
		q = floor(-d1/d2);
		r = d1 + q*d2;
	}
	if (r > d2) {
		r = r - d2;
	}
	if (r < 0.) {
		r = r + d2;
	}

	pushxm(r);
}

negate()		/* negate top element on stack */
{
	double d;
	d = xpopm();
	pushxm(-d);
}

gt()
{
	double d1, d2;
	d2 = xpopm();
	d1 = xpopm();
	d1 = (double)(d1 > d2 + EPS);
	pushxm(d1);
}

lt()
{
	double d1, d2;
	d2 = xpopm();
	d1 = xpopm();
	d1 = (double)(d1 < d2 - EPS);
	pushxm(d1);
}

ge()
{
	double d1, d2;
	d2 = xpopm();
	d1 = xpopm();
	d1 = (double)(d1 >= d2 - EPS);
	pushxm(d1);
}

le()
{
	double d1, d2;
	d2 = xpopm();
	d1 = xpopm();
	d1 = (double)(d1 <= d2 + EPS);
	pushxm(d1);
}

eq()
{
	int t1, t2;
	double d1, d2;
	t1 = (stackp-1)->i;
	t2 = (stackp-3)->i;
	switch (t2) {
	case NUMBER:
		tstkchk(t1, t2);
		d2 = xpopm();
		d1 = xpopm();
		d1 = (double)(d1 <= d2 + EPS && d1 >= d2 - EPS);
		break;
	case STRING:
		d1 = (double)(strcmp(*hoc_strpop(), *hoc_strpop()) == 0);
		break;
	case OBJECTTMP:
	case OBJECTVAR:
		{ Object** o1, **o2;
		o1 = hoc_objpop();
		o2 = hoc_objpop();
		d1 = (double)(*o1 == *o2);
		hoc_tobj_unref(o1);
		hoc_tobj_unref(o2);
		}
		break;
	default:
		hoc_execerror("don't know how to compare these types", (char*)0);
	}
	pushxm(d1);
}

ne()
{
	int t1, t2;
	double d1, d2;
	t1 = (stackp-1)->i;
	t2 = (stackp-3)->i;
	switch (t1) {
	case NUMBER:
		tstkchk(t1, t2);
		d2 = xpopm();
		d1 = xpopm();
		d1 = (double)(d1 < d2 - EPS || d1 > d2 + EPS);
		break;
	case STRING:
		d1 = (double)(strcmp(*hoc_strpop(), *hoc_strpop()) != 0);
		break;
	case OBJECTTMP:
	case OBJECTVAR:
		{ Object** o1, **o2;
		o1 = hoc_objpop();
		o2 = hoc_objpop();
		d1 = (double)(*o1 != *o2);
		hoc_tobj_unref(o1);
		hoc_tobj_unref(o2);
		}
		break;
	default:
		hoc_execerror("don't know how to compare these types", (char*)0);
	}
	pushxm(d1);
}

hoc_and()
{
	double d1, d2;
	d2 = xpopm();
	d1 = xpopm();
	d1 = (double)(d1 != 0.0 && d2 != 0.0);
	pushxm(d1);
}

hoc_or()
{
	double d1, d2;
	d2 = xpopm();
	d1 = xpopm();
	d1 = (double)(d1 != 0.0 || d2 != 0.0);
	pushxm(d1);
}

hoc_not()
{
	double d;
	d = xpopm();
	d = (double)(d == 0.0);
	pushxm(d);
}

power()			/* arg1 raised to arg2 */
{
	double d1, d2;
	extern double Pow();
	d2 = xpopm();
	d1 = xpopm();
	d1 = Pow(d1, d2);
	pushxm(d1);
}

assign()	/* assign result of execute to top symbol */
{
	Objectdata* odsav;
	Object* obsav = 0;
	Symlist* slsav;
	int op;
	Symbol *sym;
	double d2;
	op = (pc++)->i;
	sym = spopm();
    if (sym->public == 2) {
	sym = sym->u.sym;
	odsav = hoc_objectdata_save();
	obsav = hoc_thisobject;
	slsav = hoc_symlist;
	hoc_objectdata = hoc_top_level_data;
	hoc_thisobject = 0;
	hoc_symlist = hoc_top_level_symlist;
    }
	d2 = xpopm();
	switch (sym->type)
	{
	case UNDEF:
		hoc_execerror(sym->name, "undefined variable");
	case VAR:
if(!ISARRAY(sym)) {
		switch (sym->subtype) {
		case USERDOUBLE:
			if (op) {
				d2 = hoc_opasgn(op, *(sym->u.pval), d2);
			}
			*(sym->u.pval) = d2;
			break;
		case USERINT:
			if (op) {
				d2 = hoc_opasgn(op, (double)(*(sym->u.pvalint)), d2);
			}
			*(sym->u.pvalint) = (int)(d2 + EPS);
			break;
#if	CABLE
		case USERPROPERTY:
			cable_prop_assign(sym, &d2, op);
			break;
#endif
		case USERFLOAT:
			if (op) {
				d2 = hoc_opasgn(op, (double)(*(sym->u.pvalfloat)), d2);
			}
			*(sym->u.pvalfloat) = (float)(d2);
			break;
		default:
			if (op) {
				d2 = hoc_opasgn(op, *(OPVAL(sym)), d2);
			}
			*(OPVAL(sym)) = d2;
			break;
		}
}else {
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
				d2 = hoc_opasgn(op, (double)((sym->u.pvalint)[ind]), d2);
			}
			(sym->u.pvalint)[ind] = (int)(d2 + EPS);
			break;
		case USERFLOAT:
			ind = araypt(sym, SYMBOL);
			if (op) {
				d2 = hoc_opasgn(op, (double)((sym->u.pvalfloat)[ind]), d2);
			}
			(sym->u.pvalfloat)[ind] = (float)d2;
			break;
#if NEMO
		case NEMONODE:
			hoc_asgn_nemonode(sym, xpopm(), &d2, op);
			break;
		case NEMOAREA:
			hoc_asgn_nemoarea(sym, xpopm(), &d2, op);
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
			d2 = hoc_opasgn(op, fp->argn[sym->u.u_auto*2].val, d2);
		}
		fp->argn[sym->u.u_auto*2].val = d2;
		break;
	default:
		execerror("assignment to non-variable", sym->name);
	}
    if (obsav) {
	hoc_objectdata = hoc_objectdata_restore(odsav);
	hoc_thisobject = obsav;
	hoc_symlist = slsav;
    }
	pushxm(d2);
}

hoc_assign_str(cpp, buf)
	char** cpp, *buf;
{
	char* s = *cpp;
	*cpp = (char *)emalloc((unsigned)(strlen(buf) + 1));
	Strcpy(*cpp, buf);
	if (s) {
		hoc_free_string(s);
	}
}

int
assstr()	/* assign string on top to stack - 1 */
{
	char **ps1, **ps2;

	ps1 = hoc_strpop();
	ps2 = hoc_strpop();
	hoc_assign_str(ps2, *ps1);
}

char *
hoc_araystr(sym, index, obd) /* returns array string for multiple dimensions */
	Symbol *sym;
	int index;
	Objectdata* obd;
{
	static char name[100];
	char *cp = name+100;
	char buf[10];
	int i, n, j;

	*--cp = '\0';
	if (ISARRAY(sym)) {
		Arrayinfo* a;
		if (sym->subtype == 0) {
			a = obd[sym->u.oboff + 1].arayinfo;
		}else{
			a = sym->arayinfo;
		}
		for (i = a->nsub - 1; i >= 0; --i) {
			n = a->sub[i];
			j = index%n;
			index /= n;
			*--cp = ']';
			Sprintf(buf, "%d", j);
			for (j = strlen(buf)-1; j >=0; --j) {
				*--cp = buf[j];
			}
			*--cp = '[';
		}
	}
	return cp;
}

int
hoc_array_index(sp, od)  /* subs must be in reverse order on stack */
	Symbol* sp;
	Objectdata* od;
{
	int i;
	if (ISARRAY(sp)) {
		if (sp->subtype == 0) {
			Objectdata* sav = hoc_objectdata;
			hoc_objectdata = od;
			i = araypt(sp, OBJECTVAR);
			hoc_objectdata = sav;
		}else{
			i = araypt(sp, 0);
		}
	}else{
		i= 0;
	}
	return i;
}

int
araypt(sp, type)	/* return subscript - subs in reverse order on stack */
	Symbol *sp;
	int type;
{
	int i, total, varn;
	int d;
	register Arrayinfo *aray;
	if (type == OBJECTVAR) {
		aray = OPARINFO(sp);
	}else{
		aray = sp->arayinfo;
	}

	total = 0;
	for (i = 0; i < aray->nsub; i++)
	{
		tstkchk((stackp - 2*(aray->nsub - i) + 1)->i, NUMBER);
		d = (int)((stackp - 2*(aray->nsub - i))->val + EPS);
		if (d < 0 || d >= aray->sub[i])
			execerror("subscript out of range", sp->name);
		total = total * (aray->sub[i]) + d;
	}
	for (i = 0; i< aray->nsub; i++)
		nopopm();
	if (do_equation && sp->s_varn != 0
	&& (varn = (aray->a_varn)[total]) != 0 && hoc_access[varn] == 0)
	{
		hoc_access[varn] = var_access;
		var_access = varn;
	}
	return total;
}

/* obsolete */
#if CABLE && 0
int
nrnpnt_araypt(sp, pi)
	Symbol *sp;
	int *pi;
{
	int i, total;
	int d;
	register Arrayinfo *aray = sp->arayinfo;
	/* the difference is that the first index is for a neuron point
	   process vector, and the remaining incices are normal vector indices.
	   the return value is the parameter index and the first
	   index is returned in pi. return is 0 if not a vector or if element 0.
	*/
	total = 0;
	for (i = 0; i < aray->nsub; i++)
	{
		tstkchk((stackp - 2*(aray->nsub - i) + 1)->i, NUMBER);
		d = (int)((stackp - 2*(aray->nsub - i))->val + EPS);
		if (d < 0 || d >= aray->sub[i])
			execerror("subscript out of range", sp->name);
		total = total * (aray->sub[i]) + d;
		if (i == 0) {
			*pi = total;
			total = 0;
		}
	}
	for (i = 0; i< aray->nsub; i++)
		nopopm();
	return total;
}
#endif

print()		/* pop top value from stack, print it */
{
#if defined(__GO32__)
	extern int egagrph;
	if (egagrph) {
		char buf[50];
		sprintf(buf, "\t");
		grx_output_some_chars(buf, strlen(buf));
		prexpr();
		grx_output_some_chars("\n", 1);
	}else
#endif
	{
	NOT_PARALLEL_SUB(Fprintf(stdout, "\t");)
	prexpr();
	NOT_PARALLEL_SUB(Fprintf(stdout, "\n");)
	}
}

prexpr()	/* print numeric value */
{
	static HocStr* s;
	char *hoc_object_name();
	char * ss;
#if CABLE
	char* secaccessname();
#endif
	Object** pob;
	
	if (!s) s = hocstr_create(256);
	switch (hoc_stacktype()) {
	case NUMBER:
		Sprintf(s->buf, "%.8g ", xpopm());
		break;
	case STRING:
		ss = *(hoc_strpop());
		hocstr_resize(s, strlen(ss) + 1);
		Sprintf(s->buf, "%s ", ss);
		break;
	case OBJECTTMP:
	case OBJECTVAR:		
		pob = hoc_objpop();
		Sprintf(s->buf, "%s ", hoc_object_name(*pob));
		hoc_tobj_unref(pob);
		break;
#if 0 && CABLE
	case SECTION:
		Sprintf(s->buf, "%s ", secaccessname());
		break;
#endif
	default:
		hoc_execerror("Don't know how to print this type\n", 0);
	}
	plprint(s->buf);

}

prstr()		/* print string value */
{
	static HocStr* s;
	char **cpp;
	if (!s) s = hocstr_create(256);
	cpp = hoc_strpop();
	hocstr_resize(s, strlen(*cpp)+10);
	Sprintf(s->buf, "%s", *cpp);
	plprint(s->buf);
}

/*-----------------------------------------------------------------*/
hoc_delete_symbol()
                 /* Added 15-JUN-90 by JCW.  This routine deletes a
                 "defined-on-the-fly" variable from the symbol
                 list. */
		/* modified greatly by Hines. Very unsafe in general. */
{
#if 1
                                       /*---- extern variables ----*/
    extern Symlist * symlist;
                                       /*---- local variables -----*/
    Symbol * doomed,
           * sp;
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
	hoc_free_symspace(doomed);
    }
#endif
}
/*----------------------------------------------------------*/

hoc_newline()		/* print newline */
{
	plprint("\n");
}

varread()	/* read into variable */
{
	double d;
	extern FILE *fin;
	Symbol *var = (pc++)->sym;

	assert(var->public != 2);
	if (! ((var->type == VAR || var->type == UNDEF) && !ISARRAY(var)
	       && var->subtype == NOTUSER
	      )
	   ) {
		execerror(var->name, "is not a scalar variable");
	}
  Again:
	switch (fscanf(fin, "%lf", OPVAL(var)))
	{
	case EOF:
		if (moreinput())
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
	pushxm(d);
}

extern int zzdebug;

static Inst *
codechk() {
	if (progp >= prog+NPROG-1)
		execerror("procedure too big", (char *) 0);
	if (zzdebug)
		debugzz(progp);
	return progp++;
}

Inst *
Code(f)		/* install one instruction or operand */
	Pfri f;
{
	progp->pf = f;
	return codechk();
}

Inst* codei(f)
	int f;
{
	progp->i = f;
	return codechk();
}

codesym(f)
	Symbol *f;
{
	progp->sym = f;
	IGNORE(codechk());
}

codein(f)
	Inst *f;
{
	progp->in = f;
	IGNORE(codechk());
}

insertcode(begin, end, f)
	Inst *begin, *end;
	Pfri f;
{
	Inst *i;
	for (i = end - 1; i != begin; i--) {
		*i = *(i - 1);
	}
	begin->pf = f;

	if(zzdebug) {
		Inst* p;
		printf("insert code: what follows is the entire code so far\n");
		for (p = prog; p < progp; ++p) {
			debugzz(p);
		}
		printf("end of insert code debugging\n");
	}
}

#if defined(DOS) || defined(__GO32__) || defined (WIN32) || (MAC && !defined(DARWIN))
static int ntimes;
#endif
extern int	intset;

execute(p)	/* run the machine */
	Inst *p;
{
	Inst *pcsav;
	
	BBSPOLL
	for (pc = p; pc->in != STOP && !hoc_returning; )
	{
#if defined(DOS)
		if (++ntimes > 10) {
			ntimes = 0;
#if 0
			kbhit(); /* DOS can't capture interrupt when number crunching*/
#endif
		}
#endif
#if defined(__GO32__) || (defined(WIN32) && !defined(CYGWIN))
		if (++ntimes > 10) {
			ntimes = 0;
			hoc_check_intupt(1);
		}
#endif
#if MAC && !defined(DARWIN)
		/* there is significant overhead here */
		if (++ntimes > 100) {
			ntimes = 0;
			hoc_check_intupt(1);
		}
#endif
		if (intset)
			execerror("interrupted", (char *)0);
		/* (*((pc++)->pf))(); DEC 5000 increments pc after the return!*/
		pcsav = pc++;
		(*((pcsav)->pf))();
	}
}
