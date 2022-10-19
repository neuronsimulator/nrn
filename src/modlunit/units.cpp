#include <../../nmodlconf.h>
/* /local/src/master/nrn/src/modlunit/units.c,v 1.5 1997/11/24 16:19:13 hines Exp */
/* Mostly from Berkeley */
#include "nrnassrt.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include "units.h"
#include <assert.h>

/**
  The strategy for dynamic units selection between Legacy and modern units
  is to maintain two complete tables respectively. Legacy and modern in the
  nrnunits.lib.in file are distinquished by, e.g.,
  @LegacyY@faraday                        9.6485309+4 coul
  @LegacyN@faraday                        96485.3321233100184 coul
  The reason for two complete tables, as opposed to a main table and several
  short legacy and modern tables, is that units are often defined in terms
  of modified units. eg, "R = (k-mole) (joule/degC)"

  Nmodl, via the parser, uses only unit_pop, unit_mag, Unit_push,
  install_units, unit_div, and modl_units.

  The issue of unit magnitude arises only when declaring a unit factor as in
  the gasconstant (R) above or with the equivalent "name = (unit) -> (unit)"
  syntax. If the magnitude difers between legacy and modern, then instead of
  emitting code like "static double FARADAY = 96485.3;\n" we can emit
  #define FARADAY _nrnunit_FARADAY_[_nrnunit_use_legacy_]
  static double _nrnunit_FARADAY_[2] = {96485.3321233100184, 96485.3};
**/

/* modlunit can do its thing in the old way */
#if !defined(NRN_DYNAMIC_UNITS)
#define NRN_DYNAMIC_UNITS 0
#endif

#ifdef MINGW
#include "../mswin/extra/d2upath.cpp"
#endif
#if defined(WIN32)
#include <windows.h>
#endif

int unitonflag = 1;
static int UnitsOn = 0;
extern "C" {
extern double fabs(double);
}  // extern "C"
extern void diag(const char*, const char*);
#define IFUNITS       \
    {                 \
        if (!UnitsOn) \
            return;   \
    }
#define OUTTOLERANCE(arg1, arg2) (fabs(arg2 / arg1 - 1.) > 1.e-5)

#define NTAB 601

#if NRN_DYNAMIC_UNITS
#define SUFFIX ".in"
#else
#define SUFFIX ""
#endif

/* if MODLUNIT environment variable not set then look in the following places*/
#if MAC
static const char* dfile = ":lib:nrnunits.lib" SUFFIX;
#else
#if defined(NEURON_DATA_DIR)
static const char* dfile = NEURON_DATA_DIR "/lib/nrnunits.lib" SUFFIX;
#else
static const char* dfile = "/usr/lib/units";
#endif
#endif
#if defined(__TURBOC__) || defined(__GO32__)
static const char* dfilealt = "/nrn/lib/nrnunits.lib" SUFFIX;
#else
#if MAC
static const char* dfilealt = "::lib:nrnunits.lib" SUFFIX;
#else
static const char* dfilealt = "../../share/lib/nrnunits.lib" SUFFIX;
#endif
#endif
static char* unames[NDIM];
double getflt();
void fperr(int);
int lookup(char* name, unit* up, int den, int c);
struct table* hash_table(const char*);

void chkfperror();
void units(unit*);
int pu(int, int, int);
int convr(unit*);
void units_cpp_init();
int get();

extern void Unit_push(const char*);

static struct table {
    double factor;
#if - 1 == '\377'
    char dim[NDIM];
#else
    signed char dim[NDIM];
#endif
    char* name;
} * table;

static char* names;

#if NRN_DYNAMIC_UNITS
static struct dynam {
    struct table* table; /* size NTAB */
    char* names;         /* size NTAB*10 */
} dynam[2];
#endif

static struct prefix {
    double factor;
    const char* pname;
} prefix[] = {{1e-18, "atto"},
              {1e-15, "femto"},
              {1e-12, "pico"},
              {1e-9, "nano"},
              {1e-6, "micro"},
              {1e-3, "milli"},
              {1e-2, "centi"},
              {1e-1, "deci"},
              {1e1, "deka"},
              {1e2, "hecta"},
              {1e2, "hecto"},
              {1e3, "kilo"},
              {1e6, "mega"},
              {1e6, "meg"},
              {1e9, "giga"},
              {1e12, "tera"},
              {0.0, nullptr}};
static FILE* inpfile;
static int fperrc;
static int peekc;
static int dumpflg;

static const char* pc;

static int Getc(FILE* inp) {
    if (inp != stdin) {
#if MAC
        int c = getc(inp);
        if (c == '\r') {
            c = '\n';
        }
        return c;
#else
        return getc(inp);
#endif
    } else if (pc && *pc) {
        return (int) (*pc++);
    } else {
        return (int) ('\n');
    }
}

#define UNIT_STK_SIZE 20
static struct unit unit_stack[UNIT_STK_SIZE], *usp{nullptr};

static char* neuronhome() {
#if defined(WIN32)
    int i;
    static char buf[256];
    GetModuleFileName(NULL, buf, 256);
    for (i = strlen(buf); i >= 0 && buf[i] != '\\'; --i) {
        ;
    }
    buf[i] = '\0';  // /neuron.exe gone
    //      printf("setneuronhome |%s|\n", buf);
    for (i = strlen(buf); i >= 0 && buf[i] != '\\'; --i) {
        ;
    }
    buf[i] = '\0';  // /bin gone
    {
        char* u = hoc_dos2unixpath(buf);
        strcpy(buf, hoc_dos2unixpath(u));
        free(u);
    }
    return buf;
#else
    return getenv("NEURONHOME");
#endif
}


static char* ucp;
const char* Unit_str(unit* up) {
    struct unit* p;
    int f, i;
    static char buf[256];

    p = up;
    sprintf(buf, "%g ", p->factor);
    {
        int seee = 0;
        for (ucp = buf; *ucp; ucp++) {
            if (*ucp == 'e')
                seee = 1;
            if (seee)
                *ucp = ucp[1];
        }
        if (seee)
            ucp--;
    }
    f = 0;
    for (i = 0; i < NDIM; i++)
        f |= pu(p->dim[i], i, f);
    if (f & 1) {
        *ucp++ = '/';
        f = 0;
        for (i = 0; i < NDIM; i++)
            f |= pu(-p->dim[i], i, f);
    }
    *ucp = '\0';
    return buf;
}

void unit_pop() {
    IFUNITS
    assert(usp >= unit_stack);
    if (usp == unit_stack) {
        usp = nullptr;
    } else {
        --usp;
    }
}

void unit_swap() { /*exchange top two elements of stack*/
    struct unit* up;
    int i, j;
    double d;

    IFUNITS
    assert(usp > unit_stack);
    up = usp - 1;

    d = usp->factor;
    usp->factor = up->factor;
    up->factor = d;

    for (i = 0; i < NDIM; i++) {
        j = usp->dim[i];
        usp->dim[i] = up->dim[i];
        up->dim[i] = j;
    }
}

double unit_mag() { /* return unit magnitude that is on stack */
    return usp->factor;
}

void unit_mag_mul(double d) {
    usp->factor *= d;
}

void punit() {
    struct unit* i;
    for (i = usp; i != unit_stack - 1; --i) {
        printf("%s\n", Unit_str(i));
    }
}

void ucopypop(unit* up) {
    int i;
    for (i = 0; i < NDIM; i++) {
        up->dim[i] = usp->dim[i];
    }
    up->factor = usp->factor;
    up->isnum = usp->isnum;
    unit_pop();
}

void ucopypush(unit* up) {
    int i;
    Unit_push("");
    for (i = 0; i < NDIM; i++) {
        usp->dim[i] = up->dim[i];
    }
    usp->factor = up->factor;
    usp->isnum = up->isnum;
}

void Unit_push(const char* str) {
    IFUNITS
    assert(usp < unit_stack + (UNIT_STK_SIZE - 1));
    if (usp) {
        ++usp;
    } else {
        usp = unit_stack;
    }
    pc = str;
    if (str) {
        usp->isnum = 0;
    } else {
        pc = "";
        usp->isnum = 1;
    }
    convr(usp);
    /*printf("unit_push %s\n", str); units(usp);*/
}

void unit_push_num(double d) {
    Unit_push("");
    usp->factor = d;
}

void unitcheck(char* s) {
    Unit_push(s);
    unit_pop();
}

const char* unit_str() {
    /* return top of stack as units string */
    if (!UnitsOn)
        return "";
    return Unit_str(usp);
}

static void install_units_help(char* s1, char* s2) /* define s1 as s2 */
{
    struct table* tp;
    int i;

    IFUNITS
    Unit_push(s2);
    tp = hash_table(s1);
    if (tp->name) {
        printf("Redefinition of units (%s) to:", s1);
        units(usp);
        printf(" is ignored.\nThey remain:");
        Unit_push(s1);
        units(usp);
        diag("Units redefinition", (char*) 0);
    }
    tp->name = s1;
    tp->factor = usp->factor;
    for (i = 0; i < NDIM; i++) {
        tp->dim[i] = usp->dim[i];
    }
    unit_pop();
}

static void switch_units(int legacy) {
#if NRN_DYNAMIC_UNITS
    table = dynam[legacy].table;
    names = dynam[legacy].names;
#endif
}

void install_units(char* s1, char* s2) {
#if NRN_DYNAMIC_UNITS
    int i;
    for (i = 0; i < 2; ++i) {
        switch_units(i);
        install_units_help(s1, s2);
    }
#else
    install_units_help(s1, s2);
#endif
}

void check_num() {
    struct unit* up = usp - 1;
    /*EMPTY*/
    if (up->isnum && usp->isnum) {
        ;
    } else {
        up->isnum = 0;
    }
}

void unit_mul() {
    /* multiply next element by top of stack and leave on stack */
    struct unit* up;
    int i;

    IFUNITS
    assert(usp > unit_stack);
    check_num();
    up = usp - 1;
    for (i = 0; i < NDIM; i++) {
        up->dim[i] += usp->dim[i];
    }
    up->factor *= usp->factor;
    unit_pop();
}

void unit_div() {
    /* divide next element by top of stack and leave on stack */
    struct unit* up;
    int i;

    IFUNITS
    assert(usp > unit_stack);
    check_num();
    up = usp - 1;
    for (i = 0; i < NDIM; i++) {
        up->dim[i] -= usp->dim[i];
    }
    up->factor /= usp->factor;
    unit_pop();
}

void Unit_exponent(int val) {
    /* multiply top of stack by val and leave on stack */
    int i;
    double d;

    IFUNITS
    assert(usp >= unit_stack);
    for (i = 0; i < NDIM; i++) {
        usp->dim[i] *= val;
    }
    d = usp->factor;
    for (i = 1; i < val; i++) {
        usp->factor *= d;
    }
}

int unit_cmp_exact() { /* returns 1 if top two units on stack are same */
    struct unit* up;
    int i;

    {
        if (!UnitsOn)
            return 0;
    }
    up = usp - 1;
    if (unitonflag) {
        if (up->dim[0] == 9 || usp->dim[0] == 9) {
            return 1;
        }
        for (i = 0; i < NDIM; i++) {
            if (up->dim[i] != usp->dim[i]) {
                chkfperror();
                return 0;
            }
        }
        if (OUTTOLERANCE(up->factor, usp->factor)) {
            chkfperror();
            return 0;
        }
    }
    return 1;
}

/* ARGSUSED */
static void print_unit_expr(int i) {}

void Unit_cmp() {
    /*compares top two units on stack. If not conformable then
    gives error. If not same factor then gives error.
    */
    struct unit* up;
    int i;

    IFUNITS
    assert(usp > unit_stack);
    up = usp - 1;
    if (usp->isnum) {
        unit_pop();
        return;
    }
    if (up->isnum) {
        for (i = 0; i < NDIM; i++) {
            up->dim[i] = usp->dim[i];
        }
        up->factor = usp->factor;
        up->isnum = 0;
    }
    if (unitonflag && up->dim[0] != 9 && usp->dim[0] != 9) {
        for (i = 0; i < NDIM; i++) {
            if (up->dim[i] != usp->dim[i]) {
                chkfperror();
                print_unit_expr(2);
                fprintf(stderr, "\nunits:");
                units(usp);
                print_unit_expr(1);
                fprintf(stderr, "\nunits:");
                units(up);
                diag("The units of the previous two expressions are not conformable", (char*) "\n");
            }
        }
        if (OUTTOLERANCE(up->factor, usp->factor)) {
            chkfperror();
            fprintf(stderr,
                    "The previous primary expression with units: %s\n\
is missing a conversion factor and should read:\n  (%g)*(",
                    Unit_str(usp),
                    usp->factor / up->factor);
            print_unit_expr(2);
            diag(")\n", (char*) 0);
        }
    }
    unit_pop();
    return;
}

int unit_diff() {
    /*compares top two units on stack. If not conformable then
    return 1, if not same factor return 2 if same return 0
    */
    struct unit* up;
    int i;

    if (!UnitsOn)
        return 0;
    assert(usp > unit_stack);
    up = usp - 1;
    if (up->dim[0] == 9 || usp->dim[0] == 9) {
        unit_pop();
        return 0;
    }
    if (usp->isnum) {
        unit_pop();
        return 0;
    }
    if (up->isnum) {
        for (i = 0; i < NDIM; i++) {
            up->dim[i] = usp->dim[i];
        }
        up->factor = usp->factor;
        up->isnum = 0;
    }
    for (i = 0; i < NDIM; i++) {
        if (up->dim[i] != usp->dim[i]) {
            return 1;
        }
    }
    if (OUTTOLERANCE(up->factor, usp->factor)) {
        return 2;
    }
    unit_pop();
    return 0;
}

void chkfperror() {
    if (fperrc) {
        diag("underflow or overflow in units calculation", (char*) 0);
    }
}

void dimensionless() {
    /* ensures top element is dimensionless */
    int i;

    IFUNITS
    assert(usp >= unit_stack);
    if (usp->dim[0] == 9) {
        return;
    }
    for (i = 0; i < NDIM; i++) {
        if (usp->dim[i] != 0) {
            units(usp);
            diag("The previous expression is not dimensionless", (char*) 0);
        }
    }
}

void unit_less() {
    /* ensures top element is dimensionless  with factor 1*/
    IFUNITS
    if (unitonflag) {
        dimensionless();
        if (usp->dim[0] != 9 && OUTTOLERANCE(usp->factor, 1.0)) {
            fprintf(stderr,
                    "The previous expression needs the conversion factor (%g)\n",
                    1. / (usp->factor));
            diag("", (char*) 0);
        }
    }
    unit_pop();
}

void unit_stk_clean() {
    IFUNITS
    usp = nullptr;
}

// allow the outside world to call either modl_units() or unit_init().
static void units_alloc() {
    int i;
    static int units_alloc_called = 0;
    if (!units_alloc_called) {
        units_alloc_called = 1;
#if NRN_DYNAMIC_UNITS
        for (i = 0; i < 2; ++i) {
            dynam[i].table = (struct table*) calloc(NTAB, sizeof(struct table));
            assert(dynam[i].table);
            dynam[i].names = (char*) calloc(NTAB * 10, sizeof(char));
            assert(dynam[i].names);
            switch_units(i);
        }
#else
        table = (struct table*) calloc(NTAB, sizeof(struct table));
        assert(table);
        names = (char*) calloc(NTAB * 10, sizeof(char));
        assert(names);
#endif
    }
}

extern void unit_init();

void modl_units() {
    int i;
    static int first = 1;
    unitonflag = 1;
    if (first) {
        units_alloc();
#if NRN_DYNAMIC_UNITS
        for (i = 0; i < 2; ++i) {
            switch_units(i);
            unit_init();
        }
#else
        unit_init();
#endif
        first = 0;
    }
}

void unit_init() {
    char* s;
    char buf[1024];
    inpfile = (FILE*) 0;
    units_alloc();
    UnitsOn = 1;
    s = getenv("MODLUNIT");
    if (s) {
        /* note that on mingw, even if MODLUNIT set to /cygdrive/c/...
         * it ends up here as c:/... and that is good*/
        /* printf("MODLUNIT=|%s|\n", s); */
        sprintf(buf, "%s%s", s, SUFFIX);
        if ((inpfile = fopen(buf, "r")) == (FILE*) 0) {
            diag("Bad MODLUNIT environment variable. Cant open:", buf);
        }
    }
#if defined(__MINGW32__)
    if (!inpfile) {
        s = strdup(neuronhome());
        if (s) {
            if (strncmp(s, "/cygdrive/", 10) == 0) {
                /* /cygdrive/x/... to c:/... */
                sprintf(buf, "%c:%s/lib/nrnunits.lib" SUFFIX, s[10], s + 11);
            } else {
                sprintf(buf, "%s/lib/nrnunits.lib" SUFFIX, s);
            }
            inpfile = fopen(buf, "r");
            free(s);
        }
    }
#else
    if (!inpfile && (inpfile = fopen(dfile, "r")) == (FILE*) 0) {
        if ((inpfile = fopen(dfilealt, "r")) == (FILE*) 0) {
            s = neuronhome();
            if (s) {
                sprintf(buf, "%s/lib/nrnunits.lib" SUFFIX, s);
                inpfile = fopen(buf, "r");
            }
        }
    }
#endif

    if (!inpfile) {
        fprintf(stderr, "Set a MODLUNIT environment variable path to the units table file\n");
        fprintf(stderr, "Cant open units table in either of:\n%s\n", buf);
        diag(dfile, dfilealt);
    }
    signal(8, fperr);
    units_cpp_init();
    unit_stk_clean();
}

#if 0
void main(argc, argv)
char *argv[];
{
	register i;
	register char *file;
	struct unit u1, u2;
	double f;

	if(argc>1 && *argv[1]=='-') {
		argc--;
		argv++;
		dumpflg++;
	}
	file = dfile;
	if(argc > 1)
		file = argv[1];
	if ((inpfile = fopen(file, "r")) == NULL) {
		printf("no table\n");
		exit(1);
	}
	signal(8, fperr);
	units_cpp_init();

loop:
	fperrc = 0;
	printf("you have: ");
	if(convr(&u1))
		goto loop;
	if(fperrc)
		goto fp;
loop1:
	printf("you want: ");
	if(convr(&u2))
		goto loop1;
	for(i=0; i<NDIM; i++)
		if(u1.dim[i] != u2.dim[i])
			goto conform;
	f = u1.factor/u2.factor;
	if(fperrc)
		goto fp;
	printf("\t* %e\n", f);
	printf("\t/ %e\n", 1./f);
	goto loop;

conform:
	if(fperrc)
		goto fp;
	printf("conformability\n");
	units(&u1);
	units(&u2);
	goto loop;

fp:
	printf("underflow or overflow\n");
	goto loop;
}
#endif

void units(unit* up) {
    printf("\t%s\n", Unit_str(up));
}

int pu(int u, int i, int f) {
    if (u > 0) {
        if (f & 2)
            *ucp++ = '-';
        if (unames[i]) {
            sprintf(ucp, "%s", unames[i]);
            ucp += strlen(ucp);
        } else {
            sprintf(ucp, "*%c*", i + 'a');
            ucp += strlen(ucp);
        }
        if (u > 1)
            *ucp++ = (u + '0');
        return (2);
    }
    if (u < 0)
        return (1);
    return (0);
}

int convr(unit* up) {
    struct unit* p;
    int c;
    char* cp;
    char name[20];
    int den, err;

    p = up;
    for (c = 0; c < NDIM; c++)
        p->dim[c] = 0;
    p->factor = getflt();
    if (p->factor == 0.) {
        p->factor = 1.0;
    }
    err = 0;
    den = 0;
    cp = name;

loop:
    switch (c = get()) {
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '-':
    case '/':
    case ' ':
    case '\t':
    case '\n':
        if (cp != name) {
            *cp++ = 0;
            cp = name;
            err |= lookup(cp, p, den, c);
        }
        if (c == '/')
            den++;
        if (c == '\n')
            return (err);
        goto loop;
    }
    *cp++ = c;
    goto loop;
}

int lookup(char* name, unit* up, int den, int c) {
    struct unit* p;
    struct table* q;
    int i;
    char* cp2;
    double e;

    p = up;
    e = 1.0;
loop:
    q = hash_table(name);
    if (q->name) {
    l1:
        if (den) {
            p->factor /= q->factor * e;
            for (i = 0; i < NDIM; i++)
                p->dim[i] -= q->dim[i];
        } else {
            p->factor *= q->factor * e;
            for (i = 0; i < NDIM; i++)
                p->dim[i] += q->dim[i];
        }
        if (c >= '2' && c <= '9') {
            c--;
            goto l1;
        }
        return (0);
    }
    {
        const char* cp1{};
        for (i = 0; (cp1 = prefix[i].pname) != 0; i++) {
            cp2 = name;
            while (*cp1 == *cp2++)
                if (*cp1++ == 0) {
                    cp1--;
                    break;
                }
            if (*cp1 == 0) {
                e *= prefix[i].factor;
                name = cp2 - 1;
                goto loop;
            }
        }
    }
    /*EMPTY*/
    char* cp1;
    for (cp1 = name; *cp1; cp1++)
        ;
    if (cp1 > name + 1 && *--cp1 == 's') {
        *cp1 = 0;
        goto loop;
    }
    fprintf(stderr,
            "Need declaration in UNITS block of the form:\n\
	(%s)	(units)\n",
            name);
    diag("Cannot recognize the units: ", name);
    /*	printf("cannot recognize %s\n", name);*/
    return (1);
}

static int equal(const char* c1, const char* c2) {
    while (*c1++ == *c2)
        if (*c2++ == 0)
            return (1);
    return (0);
}

void units_cpp_init() {
    char* cp;
    struct table *tp, *lp;
    int c, i, f, t;
    char* np;

    cp = names;
    for (i = 0; i < NDIM; i++) {
        np = cp;
        *cp++ = '*';
        *cp++ = i + 'a';
        *cp++ = '*';
        *cp++ = 0;
        lp = hash_table(np);
        lp->name = np;
        lp->factor = 1.0;
        lp->dim[i] = 1;
    }
    lp = hash_table("");
    lp->name = cp - 1;
    lp->factor = 1.0;

l0:
    c = get();
    if (c == 0) {
#if 0
		printf("%d units; %ld bytes\n\n", i, cp-names);
#endif
        if (dumpflg)
            for (tp = table; tp < table + NTAB; tp++) {
                if (tp->name == 0)
                    continue;
                printf("%s", tp->name);
                units((struct unit*) tp);
            }

        fclose(inpfile);
        inpfile = stdin;
        return;
    }
    if (c == '/') {
        while (c != '\n' && c != 0)
            c = get();
        goto l0;
    }

#if NRN_DYNAMIC_UNITS
    if (c == '@') {
        /**
           Dynamic unit line beginning with @LegacyY@ or @LegacyN@.
           If the Y or N does not match the modern or legacy table, skip the
           entire line. For a match, just leave file at char after the final '@'.
        **/
        int i;
        int legacy;
        char legstr[7];
        char y_or_n;
        for (i = 0; i < 6; ++i) {
            legstr[i] = get();
        }
        legstr[6] = '\0';
        assert(strcmp(legstr, "Legacy") == 0);
        y_or_n = get();
        assert(y_or_n == 'Y' || y_or_n == 'N');
        legacy = (y_or_n == 'Y') ? 1 : 0;
        nrn_assert(get() == '@');
        if (dynam[legacy].table != table) { /* skip the line */
            while (c != '\n' && c != 0) {
                c = get();
            }
            goto l0;
        }
        c = get();
    }
#endif

    if (c == '\n')
        goto l0;

    np = cp;
    while (c != ' ' && c != '\t') {
        *cp++ = c;
        c = get();
        if (c == 0)
            goto l0;
        if (c == '\n') {
            *cp++ = 0;
            tp = hash_table(np);
            if (tp->name)
                goto redef;
            tp->name = np;
            tp->factor = lp->factor;
            for (c = 0; c < NDIM; c++)
                tp->dim[c] = lp->dim[c];
            i++;
            goto l0;
        }
    }
    *cp++ = 0;
    lp = hash_table(np);
    if (lp->name)
        goto redef;
    convr((struct unit*) lp);
    lp->name = np;
    f = 0;
    i++;
    if (lp->factor != 1.0)
        goto l0;
    for (c = 0; c < NDIM; c++) {
        t = lp->dim[c];
        if (t > 1 || (f > 0 && t != 0))
            goto l0;
        if (f == 0 && t == 1) {
            if (unames[c])
                goto l0;
            f = c + 1;
        }
    }
    if (f > 0)
        unames[f - 1] = np;
    goto l0;

redef:
    printf("redefinition %s\n", np);
    goto l0;
}

#if NRN_DYNAMIC_UNITS
/* Translate string to double using a2f for modern units
   to allow consistency with BlueBrain/nmodl
*/
double modern_getflt() {
    int c;
    char str[100];
    char* cp;
    double d_modern;

    assert(table == dynam[0].table);

    cp = str;
    do
        c = get();
    while (c == ' ' || c == '\t');

l1:
    if (c >= '0' && c <= '9') {
        *cp++ = c;
        c = get();
        goto l1;
    }
    if (c == '.') {
        *cp++ = c;
        c = get();
        goto l1;
    }
    if (c == '+' || c == '-') {
        *cp++ = 'e';
        *cp++ = c;
        c = get();
        while (c >= '0' && c <= '9') {
            *cp++ = c;
            c = get();
        }
    }
    *cp = '\0';
    d_modern = atof(str);
    if (c == '|') {
        d_modern /= modern_getflt();
        return d_modern;
    }
    peekc = c;
    return (d_modern);
}
#endif /* NRN_DYNAMIC_UNITS */

double getflt() {
    int c, i, dp;
    double d, e;
    int f;

#if NRN_DYNAMIC_UNITS
    if (table == dynam[0].table) {
        return modern_getflt();
    }
#endif /* NRN_DYNAMIC_UNITS */
    d = 0.;
    dp = 0;
    do
        c = get();
    while (c == ' ' || c == '\t');

l1:
    if (c >= '0' && c <= '9') {
        d = d * 10. + c - '0';
        if (dp)
            dp++;
        c = get();
        goto l1;
    }
    if (c == '.') {
        dp++;
        c = get();
        goto l1;
    }
    if (dp)
        dp--;
    if (c == '+' || c == '-') {
        f = 0;
        if (c == '-')
            f++;
        i = 0;
        c = get();
        while (c >= '0' && c <= '9') {
            i = i * 10 + c - '0';
            c = get();
        }
        if (f)
            i = -i;
        dp -= i;
    }
    e = 1.;
    i = dp;
    if (i < 0)
        i = -i;
    while (i--)
        e *= 10.;
    if (dp < 0)
        d *= e;
    else
        d /= e;
    if (c == '|')
        return (d / getflt());
    peekc = c;
    return (d);
}

int get() {
    int c;

    /*SUPPRESS 560*/
    if ((c = peekc) != 0) {
        peekc = 0;
        return (c);
    }
    c = Getc(inpfile);
    if (c == '\r') {
        c = Getc(inpfile);
    }
    if (c == EOF) {
        if (inpfile == stdin) {
            printf("\n");
            exit(0);
        }
        return (0);
    }
    return (c);
}

struct table* hash_table(const char* name) {
    struct table* tp;
    const char* np;
    unsigned h;

    h = 0;
    np = name;
    while (*np)
        h = h * 57 + *np++ - '0';
    if (((int) h) < 0)
        h = -(int) h;
    h %= NTAB;
    tp = &table[h];
l0:
    if (tp->name == 0)
        return (tp);
    if (equal(name, tp->name))
        return (tp);
    tp++;
    if (tp >= table + NTAB)
        tp = table;
    goto l0;
}

void fperr(int sig) {
    signal(8, fperr);
    fperrc++;
}

static double dynam_unit_mag(int legacy, char* u1, char* u2) {
    double result;
    switch_units(legacy);
    Unit_push(u1);
    Unit_push(u2);
    unit_div();
    result = unit_mag();
    unit_pop();
    return result;
}

void nrnunit_dynamic_str(char* buf, const char* name, char* u1, char* u2) {
#if NRN_DYNAMIC_UNITS

    double legacy = dynam_unit_mag(1, u1, u2);
    double modern = dynam_unit_mag(0, u1, u2);
    sprintf(buf,
            "\n"
            "#define %s _nrnunit_%s[_nrnunit_use_legacy_]\n"
            "static double _nrnunit_%s[2] = {%a, %g};\n",
            name,
            name,
            name,
            modern,
            legacy);

#else

    Unit_push(u1);
    Unit_push(u2);
    unit_div();
#if (defined(LegacyFR) && LegacyFR == 1)
    sprintf(buf, "static double %s = %g;\n", name, unit_mag());
#else
    sprintf(buf, "static double %s = %.12g;\n", name, unit_mag());
#endif
    unit_pop();

#endif
}
