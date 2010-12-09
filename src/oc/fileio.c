#include <../../nrnconf.h>
/* /local/src/master/nrn/src/oc/fileio.c,v 1.34 1999/09/14 13:11:46 hines Exp */

#include	<stdio.h>
#include	<stdlib.h>
#include	"hoc.h"
#include "hocstr.h"
#include	"hoclist.h"
#include	"parse.h"
#include	<setjmp.h>
#include	<errno.h>

extern FILE		*fin;
extern int		pipeflag;
extern jmp_buf		begin;
extern double		hoc_ac_;
extern char* neuron_home;
extern double chkarg();

FILE	*frin;
FILE	*fout;

extern char** hoc_pgargstr();
extern char* expand_env_var();
#if 0 && MAC
#include <stdarg.h>

void debugfile(const char* format, ...) {
	va_list args;
	static FILE* df;
	if (!df) {
		df = fopen("debugfile", "w");
	}
	va_start(args, format);
	vfprintf(df, format, args);
	fflush(df);
}
#endif

int hoc_stdout() {
#if defined(WIN32) && !defined(CYGWIN)
	extern FILE* hoc_redir_stdout;
	if (ifarg(1)) {
		if (hoc_redir_stdout) {
			hoc_execerror("stdout already switched", (char*)0);
		}
		hoc_redir_stdout = fopen(gargstr(1), "wb");
	}else if (hoc_redir_stdout) {
		fclose(hoc_redir_stdout);
		hoc_redir_stdout = (FILE*)0;
	}
	ret();
	pushx ( (hoc_redir_stdout) ? (double)fileno(hoc_redir_stdout) : 1.);
#else
#if defined(MAC) && !defined(DARWIN)
	hoc_execerror("hoc_stdout", "not implemented for MAC");
#else /*UNIX */
	static int prev = -1;
	if (ifarg(1)) {
		FILE* f1;
		if (prev != -1) {
			hoc_execerror("stdout already switched", (char*)0);
		}
		prev = dup(1);
		if (prev < 0) {
			hoc_execerror("Unable to backup stdout", (char*)0);
		}
		f1 = fopen(gargstr(1), "wb");
		
		if (!f1) {
			hoc_execerror("Unable to open ", gargstr(1));
		}
		if (dup2(fileno(f1), 1) < 0) {
			hoc_execerror("Unable to attach stdout to ", gargstr(1));
		}
		fclose(f1);
	}else if (prev > -1) {
		int i;
		if (dup2(prev, 1) < 0) {
			hoc_execerror("Unable to restore stdout", (char*)0);
		}
		close(prev);
		prev = -1;
	}
	ret();
	pushx((double)fileno(stdout));
#endif
#endif
}

int
ropen()		/* open file for reading */
{
	double d;
	char *fname;

	if (ifarg(1))
		fname = gargstr(1);
	else
		fname = "";
	d = 1.;
	if (frin != stdin)
		IGNORE(fclose(frin));
	frin = stdin;
	if (fname[0] != 0) {
		if ((frin = fopen(fname, "r")) == (FILE *)0)
		{
			char* retry;
			retry = expand_env_var(fname);
			if ((frin = fopen(retry, "r")) == (FILE *)0) {
				d = 0.;
				frin = stdin;
			}
		}
	}
	errno = 0;
	ret();
	pushx(d);
}

int
wopen()		/* open file for writing */
{
	char *fname;
	double d;

	if (ifarg(1))
		fname = gargstr(1);
	else
		fname = "";
	d = 1.;
	if (fout != stdout)
 	        ERRCHK(IGNORE(fclose(fout));) 
	fout = stdout;
	if (fname[0] != 0)
 		ERRCHK(if ((fout = fopen(expand_env_var(fname), "w")) == (FILE *)0)
		{
			d = 0.;
			fout = stdout;
		})
	errno = 0;
	ret();
	pushx(d);
}

char* expand_env_var(s) char* s; {
	static HocStr* hs;
	char* cp1;
	char* cp2;
	int n;
	int begin = 1; /* only needed for mac when necessary to prepend a : */
	if (!hs) { hs = hocstr_create(256); }
	hocstr_resize(hs, strlen(s));
	for (cp1=s, cp2 = hs->buf + begin; *cp1; ++cp1) {
		if (*cp1 == '$' && cp1[1] == '(') {
			char* cp3;
			char buf[100];
			cp1 += 2;
			for (cp3 = buf; *cp1 && *cp1 != ')'; ++cp1) {
				*cp3++ = *cp1;
			}
			if (*cp1) {
				*cp3 = '\0';
				if (strcmp(buf, "NEURONHOME") == 0) {
					cp3 = neuron_home;
				}else{
					cp3 = getenv(buf);
				}
				if (cp3) {
					n = cp2 - hs->buf;
					hocstr_resize(hs, n + strlen(cp3) + strlen(s));
					cp2 = hs->buf + n;
					while(*cp3) {
						*cp2++ = *cp3++;
					}
				}
			}else{
				--cp1;
			}
		}else{
			*cp2++ = *cp1;
		}
	}
	*cp2 = '\0';
#if MAC && !defined(DARWIN)
/* convert / to : */
	for (cp1 = hs->buf+begin; *cp1; ++cp1) {
		if (*cp1 == '/') {
			*cp1 = ':';
		}
	}
/* if a : in the original name then assume already mac and done */
/* if $ in original name then done since NEURONHOME already has correct prefix */
/* if first is : then remove it, otherwise prepend it */
	if (!strchr(s, ':') && !strchr(s, '$')) {
		if (hs->buf[begin] == ':') {
			begin = 2;
		}else{
			begin = 0;
			hs->buf[0] = ':';
		}
	}
	for (cp1 = hs->buf+begin , cp2 = cp1; *cp1 ;) {
		if (cp1[0] == ':' && cp1[1] == '.') {
			if (cp1[2] == ':') {
				cp1 += 2;
				continue;
			}else if (cp1[2] == '.' && cp1[3] == ':') {
				cp1 += 3;
				*cp2++ = ':';
				continue;
			}
		}
		*cp2++ = *cp1++;
	}
	*cp2 = '\0';
#endif
	return hs->buf + begin;
}

char hoc_xopen_file_[200];

char* hoc_current_xopen() {
	return hoc_xopen_file_;
}

int hoc_xopen1(fname, rcs)	/* read and execute a hoc program */
	char* fname;
	char* rcs;
{
	FILE *savfin;
	int savpipflag, save_lineno;
	char st[200];
	extern hoc_lineno;


#if 1
	if (rcs) {
	   if (rcs[0] != '\0') {
		Sprintf(st, "co -p%s %s > %s-%s", rcs, fname,
			fname, rcs);
 		ERRCHK(if (system(st) != 0) { 
		    hoc_execerror("st", "\nreturned error in hoc_co system call");
		})
		Sprintf(st, "%s-%s", fname, rcs);
		fname = st;
	   }
	}else if (hoc_retrieving_audit()) {
		hoc_xopen_from_audit(fname);
		return;
	}
#endif
	savpipflag = pipeflag;
	savfin = fin;
	pipeflag = 0;

#if !defined(__MWERKS__)
 	errno = EINTR; 
 	while (errno == EINTR) {
#endif 
	   errno = 0; 
#if MAC
	   if ((fin = fopen(fname, "rb")) == NULL) {
#else
	   if ((fin = fopen(fname, "r")) == NULL) {
#endif
		char* retry;
		fname = retry = expand_env_var(fname);
#if MAC
		if ((fin = fopen(retry, "rb")) == NULL) {
#else
		if ((fin = fopen(retry, "r")) == NULL) {
#endif
			fin = savfin;
			pipeflag = savpipflag;
			execerror("Can't open ", retry);
		}
	   }
#if !defined(__MWERKS__)
	}
#endif

	save_lineno = hoc_lineno;
	hoc_lineno = 0;
	strcpy(st, hoc_xopen_file_);
	strcpy(hoc_xopen_file_, fname);
	if (fin) {
		hoc_audit_from_xopen1(fname, rcs);
		IGNORE(hoc_xopen_run((Symbol *)0, (char *)0));
	}
	if (fin && fin != stdin)
 		ERRCHK(IGNORE(fclose(fin));)
	fin = savfin;		
	pipeflag = savpipflag;
	if (fname == st) {
		unlink(st);
	}
	hoc_xopen_file_[0] = '\0';
	hoc_lineno = save_lineno;
	strcpy(hoc_xopen_file_, st);
}

xopen()		/* read and execute a hoc program */
{

	if (ifarg(2)) {
		hoc_xopen1(gargstr(1), gargstr(2));
	}else{
		hoc_xopen1(gargstr(1), 0);
	}
	ret();
	pushx(1.);
}


Fprint()	/* fprintf function */
{
	char* buf;
	double d;

	sprint(&buf, 1);
	d = (double)fprintf(fout, "%s", buf);
	ret();
	pushx(d);
}

PRintf()	/* printf function */
{
	char* buf;
	double d;

	sprint(&buf, 1);
	d = (int)strlen(buf);
	NOT_PARALLEL_SUB(plprint(buf);)
	fflush(stdout);
	ret();
	pushx(d);
}


hoc_Sprint()    /* sprintf function */
{ 
	char **cpp;
	char *buf;
	/* This is not guaranteed safe since we may be pointing to double */
	cpp = hoc_pgargstr(1);
	sprint(&buf, 2);
	hoc_assign_str(cpp, buf);
        ret();
	pushx(1.);
}

double hoc_scan(fi)
	FILE* fi;
{
	double d;
	char fs[256];

	for(;;) {
		if (fscanf(fi, "%255s", fs) == EOF) {
			execerror("EOF in fscan", (char *)0);
		}
		if (fs[0] == 'i' || fs[0] == 'n' || fs[0] == 'I' || fs[0] == 'N') {
			 continue;
		}
		if (sscanf(fs, "%lf", &d) == 1) {
			/* but if at end of line, leave at beginning of next*/
			fscanf(fi, "\n");
			break;
		}
	}
	return d;
}

int
Fscan()		/* read a number from input file */
{
	double d;
	FILE *fi;

	if (frin == stdin) {
		fi = fin;
	}else{
		fi = frin;
	}
	d = hoc_scan(fi);
	ret();
	pushx(d);
}

int
hoc_Getstr()	/* read a line (or word) from input file */
{
	char* buf;
	char **cpp;
	FILE* fi;	
	int word = 0;
	if (frin == stdin) {
		fi = fin;
	}else{
		fi = frin;
	}
	cpp = hoc_pgargstr(1);
	if (ifarg(2)) {
		word = (int)chkarg(2, 0., 1.);
	}
	if (word) {
		buf = hoc_tmpbuf->buf;
		if(fscanf(fi, "%s", buf) != 1) {
			execerror("EOF in getstr", (char*)0);
		}
	}else{
		if ((buf = fgets_unlimited(hoc_tmpbuf, fi)) == (char *)0) {
			execerror("EOF in getstr", (char *)0);
		}
	}
	hoc_assign_str(cpp, buf);
	ret();
	pushx((double)strlen(buf));
}

int
sprint(ppbuf, argn)	/* convert args to right type for conversion */
	char** ppbuf;
	int argn;	/* argument number where format is */
{
	static HocStr* hs;
	char *pbuf, *pfmt, *pfrag, frag[120];
	int n, convflag, lflag, didit, hoc_argtype();
	char *fmt, *cp;

	if (!hs) { hs = hocstr_create(512); }
	fmt = gargstr(argn++);
 	convflag = lflag = didit = 0;
	pbuf = hs->buf;
	pfrag = frag;
	*pfrag = 0;
	*pbuf = 0;

	for (pfmt = fmt; *pfmt; pfmt++)
	{
		*pfrag++ = *pfmt;
		*pfrag = 0;
		if (convflag) { switch (*pfmt)
		 {

		case 'l':
			lflag += 1;
			break;

		case 'd':
		case 'o':
		case 'x':
			if (lflag) {
				if (lflag == 1) {
					pfrag[1] = pfrag[0];
					pfrag[0] = pfrag[-1];
					pfrag[-1] = 'l';
				}
				Sprintf(pbuf, frag, (long long)*getarg(argn));
			}else{
				Sprintf(pbuf, frag, (int)*getarg(argn));
			}
			didit = 1;
			break;

		case 'c':
			Sprintf(pbuf, frag, (char)*getarg(argn));
			didit = 1;
			break;

		case 'f':
		case 'e':
		case 'g':
			Sprintf(pbuf, frag, *getarg(argn));
			didit = 1;
			break;

		case 's':
			if (hoc_is_object_arg(argn)) {
				char* hoc_object_name();
				Object** hoc_objgetarg();
				cp = hoc_object_name(*hoc_objgetarg(argn));
			}else{
				cp = gargstr(argn);
			}
			n = pbuf - hs->buf;
			hocstr_resize(hs, n + strlen(cp) + 100);
			pbuf = hs->buf + n;
			Sprintf(pbuf, frag, cp);
			didit = 1;
			break;

		case '%':
			lflag = 0;
			convflag = 0;
			break;

		default:
			break;
		 }
		}else if (*pfmt == '%') {
			convflag = 1;
		}else if (pfrag - frag > 100) {
			n = pbuf - hs->buf;
			hocstr_resize(hs, n + strlen(frag) + 100);
			pbuf = hs->buf + n;
			Sprintf(pbuf, frag);
			pfrag = frag;
			*pfrag = 0;
			while (*pbuf) {	pbuf++; }
		}

		if (didit)
		{
			int n;
			argn++;
			lflag = 0;
			convflag = 0;
			didit = 0;
			pfrag = frag;
			*pfrag = 0;
			while (*pbuf) {	pbuf++; }
			n = pbuf - hs->buf;
			hocstr_resize(hs, n + 100);
			pbuf = hs->buf + n;
		}
	}
	if (pfrag != frag)
		Sprintf(pbuf, frag);
	*ppbuf = hs->buf;
}

#if defined(__TURBOC__) || defined(__GO32__) || defined(WIN32) || defined(MAC)
static FILE* oc_popen(char* cmd, char* type) {
	FILE* fp;
	char buf[256];
	sprintf(buf, "sh %s > hocload.tmp", cmd);
	if (system(buf) != 0) {
		return (FILE*)0;
	}else if ( (fp = fopen("hocload.tmp", "r")) == (FILE*)0) {
		return (FILE*)0;
	}else{
		return fp;
	}
}
static void oc_pclose(FILE* fp) {
	fclose(fp);
	unlink("hocload.tmp");
}
#else
#define oc_popen popen
#define oc_pclose pclose
#endif

static hoc_load(stype)
	char* stype;
{
	int i=1;
	char* s;
	Symbol* sym;
	char cmd[256];
	FILE* p;
	char file[256], *f;

	while(ifarg(i)) {
		s = gargstr(i);
		++i;
		sym = hoc_lookup(s);
		if (!sym || sym->type == UNDEF) {
			sprintf(cmd, "$NEURONHOME/lib/hocload.sh %s %s %d", stype, s, hoc_pid());
			p = oc_popen(cmd, "r");
			if (p) {
				f = fgets(file, 256, p);
				if (f) {
					f[strlen(f)-1] = '\0';
				}
				oc_pclose(p);
				if (f) {
		fprintf(stderr, "Getting %s from %s\n", s, f);
					hoc_Load_file(0,f);
				}else{
fprintf(stderr, "Couldn't find a file that declares %s\n", s);
				}
			}else{
				hoc_execerror("can't run:", cmd);
			}
		}
	}
}

Pfri p_hoc_load_java;
hoc_load_java() {
	int r = 0;
	if (p_hoc_load_java) {	
		r =  (*p_hoc_load_java)();
	}
	ret();
	pushx((double)r);
}

hoc_load_proc() {
	hoc_load("proc");
	ret();
	pushx(1.);
}
hoc_load_func() {
	hoc_load("func");
	ret();
	pushx(1.);
}
hoc_load_template() {
	hoc_load("begintemplate");
	ret();
	pushx(1.);
}

hoc_load_file() {
	int iarg = 1;
	int i = 0;
	if (hoc_is_double_arg(iarg)) {
		i = (int)chkarg(iarg, 0., 1.);
		iarg = 2;
	}
	if (!ifarg(iarg+1) || !hoc_lookup(gargstr(iarg+1))) {
		i = hoc_Load_file(i, gargstr(iarg));
	}
	ret();
	pushx((double)i);
}

hoc_Load_file(always, name) int always; char* name; {
	/*
	  if always is 0 then
	  xopen only if file of that name not already loaded with one of
		the load_xxx functions
		and search in the current, $HOC_LIBRARY_PATH,
		$NEURONHOME/lib/hoc directories (in that order) for
		the file if there is no directory prefix.
		Temporarily change to the directory containing the file so
		that it can xopen files relative to its location.
	*/
	static hoc_List* loaded;
	hoc_Item* q;
	int b, is_loaded;
	int goback;
	char expname[512];
	char *base;
	char path[1000], old[1000], fname[1000], cmd[200];
	FILE* f;

	old[0] = '\0';
	goback = 0;
	/* has the file already been loaded */
	is_loaded = 0;
	if (!loaded) {
		loaded = hoc_l_newlist();
	}
	ITERATE(q, loaded) {
		if (strcmp(STR(q), name) == 0) {
			if (!always) {
				return 1;
			}else{
				is_loaded = 1;
			}
		}
	}
	
	/* maybe the name already has an explicit path */
	strncpy(expname, expand_env_var(name), 512);
	name = expname;
	if ((base = strrchr(name, '/')) != NULL) {
		strncpy(path, name, base-name);
		path[base-name] = '\0';
		++base;
		f = fopen(name, "r");
	}else{
		base = name;
		path[0] = '\0';
		/* otherwise find the file in the default directories */
		f = fopen(base, "r"); /* cwd */
#if !MAC
		if (!f) { /* try HOC_LIBRARY_PATH */
			int i;
			char* hlp;
			hlp = getenv("HOC_LIBRARY_PATH");
			while(hlp && *hlp) {
				char* cp =  strchr(hlp, ':');
				if (!cp) {
					cp = strchr(hlp, ' ');
				}
				if (!cp) {
					cp = hlp + strlen(hlp);
				}
				strncpy(path, hlp, cp-hlp);
				path[cp-hlp] = '\0';
				if (*cp) {
					hlp = cp+1;
				}else{
					hlp = 0;
				}
				if (path[0]) {
					sprintf(fname, "%s/%s", path, base);
					f = fopen(expand_env_var(fname), "r");
					if (f) {
						break;
					}
				}else{
					break;
				}
			}
		}
#endif
		if (!f) { /* try NEURONHOME/lib/hoc */
			sprintf(path, "$(NEURONHOME)/lib/hoc");
			sprintf(fname, "%s/%s", path, base);
			f = fopen(expand_env_var(fname), "r");
		}
	}
	/* add the name to the list of loaded packages */
	if (f) {
		if (!is_loaded) {
			hoc_l_lappendstr(loaded, name);
		}
		fclose(f);
		b = 1;
	}else{
		b = 0;
		hoc_warning("Couldn't find:", name);
		path[0] = '\0';
	}
	/* change to the right directory*/
	if (b && path[0]) {
		goback = (getcwd(old, 1000) != 0);
		errno = 0;
		if (chdir(expand_env_var(path)) == -1) {
			hoc_warning("Couldn't change directory to:", path);
			path[0] = '\0';
			b = 0;
		}
/*printf("load_file cd to %s\n", path);*/
	}
	/* xopen the file */
	if (b) {
/*printf("load_file xopen %s\n", base);*/
		sprintf(cmd, "hoc_ac_ = execute1(\"{xopen(\\\"%s\\\")}\")\n", base);
		b = hoc_oc(cmd);
		b = (int)hoc_ac_;
	}
	/* change back */
	if (path[0] && goback) {
		if (hoc_chdir(old) == -1) {
			hoc_warning("Couldn't change directory back to:", old);
			b = 0;
		}
/*printf("load_file cd back to %s\n", old);*/
	}
	
	return b;
}

hoc_getcwd() {
	int len;
	static char* buf;
	if (!buf) {
		buf = emalloc(1000);
	}
	if (!getcwd(buf, 1000)) {
		hoc_execerror("getcwd failed. Perhaps the path length is > 1000", (char*)0);
	}
#if defined(WIN32)
{extern char* hoc_back2forward();
	strcpy(buf, hoc_back2forward(buf));
}
#endif
	len = strlen(buf);
#if defined(MAC)
	if (buf[len-1] != ':') {
		buf[len] = ':';
		buf[len+1] = '\0';
	}
#else
	if (buf[len-1] != '/') {
		buf[len] = '/';
		buf[len+1] = '\0';
	}
#endif
	hoc_ret();
	hoc_pushstr(&buf);
}

machine_name()
{
#if !defined(__GO32__) && !defined(WIN32) && !defined(MAC)
  				       /*----- functions called -----*/
				       /*----- local  variables -----*/
    char	buf[20];

    gethostname(buf, 20);
    hoc_assign_str(hoc_pgargstr(1), buf); 
#endif
    ret();
    pushx(0.);
}

int hoc_chdir(path) char* path; {
	return chdir(expand_env_var(path));
}

hoc_Chdir() {
	int i = hoc_chdir(gargstr(1));
	ret();
	pushx((double)i);
}
