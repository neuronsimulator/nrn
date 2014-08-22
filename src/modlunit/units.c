#include <../../nmodlconf.h>
/* /local/src/master/nrn/src/modlunit/units.c,v 1.5 1997/11/24 16:19:13 hines Exp */
/* Mostly from Berkeley */
#include	<stdio.h>
#include	<stdlib.h>
#include	<signal.h>
#include	<string.h>
#include	"units.h"
#include	<assert.h>

#if defined(CYGWIN)
#include "../mswin/extra/d2upath.c"
#endif
#if defined(WIN32)
#include <windows.h>
#endif

int	unitonflag = 1;
static int	UnitsOn = 0;
extern double	fabs();
extern void diag();
#define	IFUNITS	{if (!UnitsOn) return;}
#define OUTTOLERANCE(arg1,arg2) (fabs(arg2/arg1 - 1.) > 1.e-5)

#define	NTAB	601
/* if MODLUNIT environment variable not set then look in the following places*/
#if MAC
static char	*dfile	= ":lib:nrnunits.lib";
#else
#if defined(NEURON_DATA_DIR)
static char	*dfile	= NEURON_DATA_DIR"/lib/nrnunits.lib";
#else
static char	*dfile	= "/usr/lib/units";
#endif
#endif
#if defined(__TURBOC__) || defined(__GO32__)
static char	*dfilealt	= "/nrn/lib/nrnunits.lib";
#else
#if MAC
static char *dfilealt = "::lib:nrnunits.lib";
#else
static char	*dfilealt	= "../../share/lib/nrnunits.lib";
#endif
#endif
static char	*unames[NDIM];
static double	getflt();
static void fperr();
static int	lookup();
static struct	table	*hash();

static void chkfperror();
static void units();
static int pu();
static int convr();
static void init();
static int get();

extern void Unit_push(char*);

static struct table
{
	double	factor;
#if -1 == '\377'
	char	dim[NDIM];
#else
	signed char	dim[NDIM];
#endif
	char	*name;
} table[NTAB];
static char	names[NTAB*10];
static struct prefix
{
	double	factor;
	char	*pname;
} prefix[] = 
{
	1e-18,	"atto",
	1e-15,	"femto",
	1e-12,	"pico",
	1e-9,	"nano",
	1e-6,	"micro",
	1e-3,	"milli",
	1e-2,	"centi",
	1e-1,	"deci",
	1e1,	"deka",
	1e2,	"hecta",
	1e2,	"hecto",
	1e3,	"kilo",
	1e6,	"mega",
	1e6,	"meg",
	1e9,	"giga",
	1e12,	"tera",
	0.0,	0
};
static FILE	*inpfile;
static int	fperrc;
static int	peekc;
static int	dumpflg;

static char *pc;

static int Getc(inp)
	FILE *inp;
{
	if (inp != stdin) {
#if MAC
		int c = getc(inp);
		if (c == '\r') { c  = '\n';}
		return c;
#else
		return getc(inp);
#endif
	}else if (pc && *pc) {
		return (int)(*pc++);
	}else{
		return (int)('\n');
	}
}

#define UNIT_STK_SIZE	20
static struct unit unit_stack[UNIT_STK_SIZE], *usp;

static char* neuronhome() {
#if defined(WIN32)
	int i;
	static char buf[256];
	GetModuleFileName(NULL, buf, 256);
	for (i=strlen(buf); i >= 0 && buf[i] != '\\'; --i) {;}
	buf[i] = '\0'; // /neuron.exe gone
	//      printf("setneuronhome |%s|\n", buf);
	for (i=strlen(buf); i >= 0 && buf[i] != '\\'; --i) {;}
	buf[i] = '\0'; // /bin gone
#if defined(CYGWIN)
	{
	char* u = hoc_dos2unixpath(buf);
	strcpy(buf, hoc_dos2unixpath(u));
	free(u);
	}
#endif
	return buf;
#else
	return getenv("NEURONHOME");
#endif
}

void unit_pop() {
	IFUNITS
	assert(usp >= unit_stack);
	--usp;
}

void unit_swap() { /*exchange top two elements of stack*/
	struct unit *up;
	int i, j;
	double d;
	
	IFUNITS
	assert(usp > unit_stack);
	up = usp -1;

	d = usp->factor;
	usp->factor = up->factor;
	up->factor = d;

	for (i=0; i<NDIM; i++) {
		j = usp->dim[i];
		usp->dim[i] = up->dim[i];
		up->dim[i] = j;
	}
}	
	
double
unit_mag() {	/* return unit magnitude that is on stack */
	return usp->factor;
}

void unit_mag_mul(d)
	double d;
{
	usp->factor *= d;
}

void punit() {
	struct unit *	i;
	for (i=usp; i!=unit_stack-1; --i) {
		printf("%s\n", Unit_str(i));
	}
}

void ucopypop(up)
	struct unit *up;
{
	int i;
	for (i=0; i<NDIM; i++) {
		up->dim[i] = usp->dim[i];
	}
	up->factor = usp->factor;
	up->isnum = usp->isnum;
	unit_pop();
}

void ucopypush(up)
	struct unit *up;
{
	int i;
	Unit_push("");
	for (i=0; i<NDIM; i++) {
		usp->dim[i] = up->dim[i];
	}
	usp->factor = up->factor;
	usp->isnum = up->isnum;
}

void Unit_push(string)
	char *string;
{
	IFUNITS
	assert(usp < unit_stack + (UNIT_STK_SIZE - 1));
	++usp;
	pc = string;
	if (string) {
		usp->isnum = 0;
	}else{
		pc = "";
		usp->isnum = 1;
	}
	convr(usp);
/*printf("unit_push %s\n", string); units(usp);*/
}

void unit_push_num(d)
	double d;
{
	Unit_push("");
	usp->factor = d;
}

void unitcheck(s)
	char *s;
{
	Unit_push(s);
	unit_pop();
}

char *
unit_str() {
	/* return top of stack as units string */
	if (!UnitsOn) return "";
	return Unit_str(usp);
}

void install_units(s1, s2) /* define s1 as s2 */
	char *s1, *s2;
{
	struct table *tp;
	int i;
	
	IFUNITS
	Unit_push(s2);
	tp = hash(s1);
	if (tp->name) {
		printf("Redefinition of units (%s) to:", s1);
		units(usp);
		printf(" is ignored.\nThey remain:");
		Unit_push(s1); units(usp);		
		diag("Units redefinition", (char *)0);
	}
	tp->name = s1;
	tp->factor = usp->factor;
	for (i=0; i<NDIM; i++) {
		tp->dim[i] = usp->dim[i];
	}
	unit_pop();
}

void check_num()
{
	struct unit * up = usp -1;
	/*EMPTY*/
	if (up->isnum && usp->isnum) {
		;
	} else {
		up->isnum = 0;
	}
}
		
void unit_mul() {
	/* multiply next element by top of stack and leave on stack */
	struct unit *up;
	int i;
	
	IFUNITS
	assert(usp > unit_stack);
	check_num();
	up = usp -1;
	for (i=0; i<NDIM; i++) {
		up->dim[i] += usp->dim[i];
	}
	up->factor *= usp->factor;
	unit_pop();
}

void unit_div() {
	/* divide next element by top of stack and leave on stack */
	struct unit *up;
	int i;
	
	IFUNITS
	assert(usp > unit_stack);
	check_num();
	up = usp -1;
	for (i=0; i<NDIM; i++) {
		up->dim[i] -= usp->dim[i];
	}
	up->factor /= usp->factor;
	unit_pop();
}

void Unit_exponent(val)
	int val;
{
	/* multiply top of stack by val and leave on stack */
	int i;
	double d;
	
	IFUNITS
	assert(usp >= unit_stack);
	for (i=0; i<NDIM; i++) {
		usp->dim[i] *= val;
	}
	d = usp->factor;
	for (i=1; i<val; i++) {
		usp->factor *= d;
	}
}

int
unit_cmp_exact() { /* returns 1 if top two units on stack are same */
	struct unit *up;
	int i;

	{if (!UnitsOn) return 0;}
	up = usp - 1;
   if (unitonflag) {
	if (up->dim[0] == 9 || usp->dim[0] == 9) {
		return 1;
	}
	for (i=0; i<NDIM; i++) {
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
static void print_unit_expr(i) int i; {}

void Unit_cmp() {
	/*compares top two units on stack. If not conformable then
	gives error. If not same factor then gives error.
	*/
	struct unit *up;
	int i;
	
	IFUNITS
	assert(usp > unit_stack);
	up = usp - 1;
	if (usp->isnum) {
		unit_pop();
		return;
	}
	if (up->isnum) {
		for (i=0; i < NDIM; i++) {
			up->dim[i] = usp->dim[i];
		}
		up->factor = usp->factor;
		up->isnum = 0;
	}
if (unitonflag && up->dim[0] != 9 && usp->dim[0] != 9) {
	for (i=0; i<NDIM; i++) {
		if (up->dim[i] != usp->dim[i]) {
			chkfperror();
			print_unit_expr(2);
			fprintf(stderr, "\nunits:");
			units(usp);
			print_unit_expr(1);
			fprintf(stderr, "\nunits:");
			units(up);
diag("The units of the previous two expressions are not conformable","\n");
		}
	}
	if (OUTTOLERANCE(up->factor, usp->factor)) {
		chkfperror();
fprintf(stderr, "The previous primary expression with units: %s\n\
is missing a conversion factor and should read:\n  (%g)*(",
Unit_str(usp), usp->factor/up->factor);
		print_unit_expr(2);
		diag(")\n", (char *)0);
	}
}
	unit_pop();
	return;
}

int unit_diff() {
	/*compares top two units on stack. If not conformable then
	return 1, if not same factor return 2 if same return 0
	*/
	struct unit *up;
	int i;
	
	if (!UnitsOn) return 0;
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
		for (i=0; i < NDIM; i++) {
			up->dim[i] = usp->dim[i];
		}
		up->factor = usp->factor;
		up->isnum = 0;
	}
	for (i=0; i<NDIM; i++) {
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

static void chkfperror()
{
	if (fperrc) {
		diag("underflow or overflow in units calculation", (char *)0);
	}
}

void dimensionless() {
	/* ensures top element is dimensionless */
	int i;
	
	IFUNITS
	assert(usp >= unit_stack);
	if (usp->dim[0] == 9) {return;}
	for (i=0; i<NDIM; i++) {
		if (usp->dim[i] != 0) {
			units(usp);
diag("The previous expression is not dimensionless", (char *)0);
		}
	}
}

void unit_less() {
	/* ensures top element is dimensionless  with factor 1*/
	IFUNITS
	if (unitonflag) {
		dimensionless();
		if (usp->dim[0]!=9 && OUTTOLERANCE(usp->factor,1.0)) {
fprintf(stderr, "The previous expression needs the conversion factor (%g)\n",
1./(usp->factor));
			diag("", (char *)0);
		}
	}
	unit_pop();
}

void unit_stk_clean() {
	IFUNITS
	usp = unit_stack - 1;
}
	
#if MODL||NMODL||HMODL||SIMSYS
extern void unit_init();

void modl_units() {
	static int first=1;
	unitonflag = 1;
	if (first) {
		unit_init();
		first = 0;
	}
}

#endif

void unit_init() {
	char* s;
	char buf[256];
	inpfile = (FILE*)0;
	UnitsOn = 1;
	s = getenv("MODLUNIT");
	if (s) {
		if ((inpfile = fopen(s, "r")) == (FILE *)0) {
diag("Bad MODLUNIT environment variable. Cant open:", s);
		}
	}
	if (!inpfile && (inpfile = fopen(dfile, "r")) == (FILE *)0) {
		if ((inpfile = fopen(dfilealt, "r")) == (FILE *)0) {
			s = neuronhome();
			if (s) {
				sprintf(buf, "%s/lib/nrnunits.lib", s);
				inpfile = fopen(buf, "r");
			}
		}
	}
	if (!inpfile) {
fprintf(stderr, "Set a MODLUNIT environment variable path to the units table file\n");
fprintf(stderr, "Cant open units table in either of:\n%s\n", buf);
			diag(dfile, dfilealt);
	}
	signal(8, fperr);
	init();
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
	init();

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

static void units(up)
struct unit *up;
{
	printf("\t%s\n", Unit_str(up));
}

static char *ucp;
char *Unit_str(up)
struct unit *up;
{
	register struct unit *p;
	register int f, i;
	static char buf[256];

	p = up;
	sprintf(buf, "%g ", p->factor);
	{int seee=0; for (ucp=buf; *ucp; ucp++) {
		if (*ucp == 'e') seee=1;
		if (seee) *ucp = ucp[1];
	} if (seee) ucp--;}
	f = 0;
	for(i=0; i<NDIM; i++)
		f |= pu(p->dim[i], i, f);
	if(f&1) {
		*ucp++ = '/';
		f = 0;
		for(i=0; i<NDIM; i++)
			f |= pu(-p->dim[i], i, f);
	}
	*ucp = '\0';
	return buf;
}

static int pu(u, i, f)
{

	if(u > 0) {
		if(f&2)
			*ucp++ = '-';
		if(unames[i]) {
			sprintf(ucp,"%s", unames[i]);
			ucp += strlen(ucp);
		} else{
			sprintf(ucp,"*%c*", i+'a');
			ucp += strlen(ucp);
		}
		if(u > 1)
			*ucp++ = (u+'0');
			return(2);
	}
	if(u < 0)
		return(1);
	return(0);
}

static int convr(up)
struct unit *up;
{
	register struct unit *p;
	register int c;
	register char *cp;
	char name[20];
	int den, err;

	p = up;
	for(c=0; c<NDIM; c++)
		p->dim[c] = 0;
	p->factor = getflt();
	if(p->factor == 0.) {
		p->factor = 1.0;
	}
	err = 0;
	den = 0;
	cp = name;

loop:
	switch(c=get()) {

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
		if(cp != name) {
			*cp++ = 0;
			cp = name;
			err |= lookup(cp, p, den, c);
		}
		if(c == '/')
			den++;
		if(c == '\n')
			return(err);
		goto loop;
	}
	*cp++ = c;
	goto loop;
}

static int lookup(name, up, den, c)
char *name;
struct unit *up;
{
	register struct unit *p;
	register struct table *q;
	register int i;
	char *cp1, *cp2;
	double e;

	p = up;
	e = 1.0;
loop:
	q = hash(name);
	if(q->name) {
		l1:
		if(den) {
			p->factor /= q->factor*e;
			for(i=0; i<NDIM; i++)
				p->dim[i] -= q->dim[i];
		} else {
			p->factor *= q->factor*e;
			for(i=0; i<NDIM; i++)
				p->dim[i] += q->dim[i];
		}
		if(c >= '2' && c <= '9') {
			c--;
			goto l1;
		}
		return(0);
	}
	for(i=0; (cp1 = prefix[i].pname) != 0; i++) {
		cp2 = name;
		while(*cp1 == *cp2++)
			if(*cp1++ == 0) {
				cp1--;
				break;
			}
		if(*cp1 == 0) {
			e *= prefix[i].factor;
			name = cp2-1;
			goto loop;
		}
	}
	/*EMPTY*/
	for(cp1 = name; *cp1; cp1++);
	if(cp1 > name+1 && *--cp1 == 's') {
		*cp1 = 0;
		goto loop;
	}
	fprintf(stderr, "Need declaration in UNITS block of the form:\n\
	(%s)	(units)\n", name);
	diag("Cannot recognize the units: ", name);
/*	printf("cannot recognize %s\n", name);*/
	return(1);
}

static int equal(s1, s2)
char *s1, *s2;
{
	register char *c1, *c2;

	c1 = s1;
	c2 = s2;
	while(*c1++ == *c2)
		if(*c2++ == 0)
			return(1);
	return(0);
}

static void init()
{
	register char *cp;
	register struct table *tp, *lp;
	int c, i, f, t;
	char *np;

	cp = names;
	for(i=0; i<NDIM; i++) {
		np = cp;
		*cp++ = '*';
		*cp++ = i+'a';
		*cp++ = '*';
		*cp++ = 0;
		lp = hash(np);
		lp->name = np;
		lp->factor = 1.0;
		lp->dim[i] = 1;
	}
	lp = hash("");
	lp->name = cp-1;
	lp->factor = 1.0;

l0:
	c = get();
	if(c == 0) {
#if 0
		printf("%d units; %d bytes\n\n", i, cp-names);
#endif
		if(dumpflg)
		for(tp = table; tp < table+NTAB; tp++) {
			if(tp->name == 0)
				continue;
			printf("%s", tp->name);
			units((struct unit *)tp);
		}
		fclose(inpfile);
		inpfile = stdin;
		return;
	}
	if(c == '/') {
		while(c != '\n' && c != 0)
			c = get();
		goto l0;
	}
	if(c == '\n')
		goto l0;
	np = cp;
	while(c != ' ' && c != '\t') {
		*cp++ = c;
		c = get();
		if (c==0)
			goto l0;
		if(c == '\n') {
			*cp++ = 0;
			tp = hash(np);
			if(tp->name)
				goto redef;
			tp->name = np;
			tp->factor = lp->factor;
			for(c=0; c<NDIM; c++)
				tp->dim[c] = lp->dim[c];
			i++;
			goto l0;
		}
	}
	*cp++ = 0;
	lp = hash(np);
	if(lp->name)
		goto redef;
	convr((struct unit *)lp);
	lp->name = np;
	f = 0;
	i++;
	if(lp->factor != 1.0)
		goto l0;
	for(c=0; c<NDIM; c++) {
		t = lp->dim[c];
		if(t>1 || (f>0 && t!=0))
			goto l0;
		if(f==0 && t==1) {
			if(unames[c])
				goto l0;
			f = c+1;
		}
	}
	if(f>0)
		unames[f-1] = np;
	goto l0;

redef:
	printf("redefinition %s\n", np);
	goto l0;
}

static double
getflt()
{
	register int c, i, dp;
	double d, e;
	int f;

	d = 0.;
	dp = 0;
	do
		c = get();
	while(c == ' ' || c == '\t');

l1:
	if(c >= '0' && c <= '9') {
		d = d*10. + c-'0';
		if(dp)
			dp++;
		c = get();
		goto l1;
	}
	if(c == '.') {
		dp++;
		c = get();
		goto l1;
	}
	if(dp)
		dp--;
	if(c == '+' || c == '-') {
		f = 0;
		if(c == '-')
			f++;
		i = 0;
		c = get();
		while(c >= '0' && c <= '9') {
			i = i*10 + c-'0';
			c = get();
		}
		if(f)
			i = -i;
		dp -= i;
	}
	e = 1.;
	i = dp;
	if(i < 0)
		i = -i;
	while(i--)
		e *= 10.;
	if(dp < 0)
		d *= e; else
		d /= e;
	if(c == '|')
		return(d/getflt());
	peekc = c;
	return(d);
}

static int get()
{
	register int c;

	/*SUPPRESS 560*/
	if((c=peekc) != 0) {
		peekc = 0;
		return(c);
	}
	c = Getc(inpfile);
	if (c == '\r') { c = Getc(inpfile); }
	if (c == EOF) {
		if (inpfile == stdin) {
			printf("\n");
			exit(0);
		}
		return(0);
	}
	return(c);
}

static struct table *
hash(name)
char *name;
{
	register struct table *tp;
	register char *np;
	register unsigned h;

	h = 0;
	np = name;
	while(*np)
		h = h*57 + *np++ - '0';
	if( ((int)h)<0) h= -(int)h;
	h %= NTAB;
	tp = &table[h];
l0:
	if(tp->name == 0)
		return(tp);
	if(equal(name, tp->name))
		return(tp);
	tp++;
	if(tp >= table+NTAB)
		tp = table;
	goto l0;
}

static void fperr(sig) int sig;
{

	signal(8, fperr);
	fperrc++;
}
