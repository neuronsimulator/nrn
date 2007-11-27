#include <../../nmodlconf.h>
/*
 * e:  an extended version of the Unix editor.
 * version 6.1.
 * Author:  Dana S. Nau, Duke University, Durham, NC 27706
 */

/*
 * Modified by Dennis Rockwell and David Leonard for phs to include
 * error messages, idiot proofing (requiring confirmation before
 * deleting large blocks of lines or exiting without writing if changes
 * have been made), and to save the results of the edit session
 * in ed.hangup if the editor recieves a hangup signal.
 */
/* Char needs to be a signed char */
#include <stdlib.h>
#include <unistd.h>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#else
#include <sys/fcntl.h>
#endif

#if -1 == '\377'
typedef char Char;
#else
typedef signed char Char;
#endif

#ifndef NULL
#define	NULL	(0)
#endif

#include	<signal.h>

#define	NARGS	100
#define	PWSIZ	80
#define	LINESIZE	76
#define	FNSIZE	128
#define	LBSIZE	512
#define	ESIZE	128
#define	GBSIZE	256
#define	NBRA	5
#define	EOF	(-1)

#define	CBRA	1
#define	CCHR	2
#define	CDOT	4
#define	CCL	6
#define	NCCL	8
#define	CDOL	10
#define	CEOF	11
#define	CKET	12

#define	STAR	01
#define	SUFF	'.'
#define	TILDE	'~'
#define	GRAVE	'`'
#define	echo(xstr)	if (ecflag) write(2, xstr, 1);

#define	error(enum)	errfunc(enum)
#define	READ	0
#define	WRITE	1

#define	LMARG	'!'
#define	LJUNK	'['
#define	RJUNK	']'

int	changes;
int	nhsize =	11;
int	nh1 =	5;
int	nh2 =	5;
Char	pmode =	'n';
Char	nlc1 =	GRAVE;
Char	vac1 =	0;
Char	*nlcptr;
Char	*vacptr;
Char	prompt[] =	"e      :";
#define	COLON	prompt[7]
Char	eqprompt[] =	"file changed; enter other command or reenter x:";
#define	EQ	eqprompt[45]
#define	EQEND	eqprompt[46]

/* error messages */

Char *emesg[] = {
	(Char *) "unknown error\n",
#define ESYN	1
	(Char *) "syntax error\n",
#define ENEGRANGE	2
	(Char *) "negative range\n",
#define ENOFNAME	3
	(Char *) "no filename set\n",
#define EFTOOBIG	4
	(Char *) "file name too long\n",
#define EMKRANGE	5
	(Char *) "letter not alphabetic\n",
#define ESINGLE	6
	(Char *) "single letter only\n",
#define ERANGE	7
	(Char *) "out of range\n",
#define EBADMODE	8
	(Char *) "bad mode letter\n",
#define EOPEN	9
	(Char *) "cannot open file\n",
#define ECREAT	10
	(Char *) "cannot open file for writing\n",
#define EMATCH	11
	(Char *) "cannot match\n",
#define EHUH	12
	(Char *) "line number range syntax error\n",
#define ENOADDR	13
	(Char *) "no address allowed\n",
#define EVAR	14
	(Char *) "bad variable\n",
#define ELINE	15
	(Char *) "line too long\n",
#define EWRITE	16
	(Char *) "write error\n",
#define ESPACE	17
	(Char *) "out of memory\n",
#define ETMP	18
	(Char *) "error in temporary file. DO NOT WRITE!\n",
#define EGLOB	19
	(Char *) "nested global commands\n",
#define EDUP	20
	(Char *) "error in dup. Please report this error.\n",
#define ESTACK	21
	(Char *) "stack error. Please report this error.\n",
#define EARGS	22
	(Char *) "too many macro arguments\n",
#define ENOMACRO	23
	(Char *) "cannot find macro\n",
#define EREGEXP	24
	(Char *) "search string format error\n",
#define EBUFOFL	25
	(Char *) "buffer overflow\n",
#define EEOF	26
	(Char *) "illegal EOF\n",
#define EINTR	27
	(Char *) "interrupted\n",
};

Char	*tempbuf;
int	ecflag;
int	goonmlh=1;
Char	*iii;
Char	*jjj;
Char	*ggg;
Char	*bufp;
Char	shname[] =	"/bin/sh";
Char	**varv;
Char	*varp;
int	*varc;
Char	gsubf;
int	eofok;
int	eqflag;

Char	lastc;
Char	savedfile[FNSIZE];
Char	file[FNSIZE];
Char	linebuf[LBSIZE];
Char	rhsbuf[LBSIZE/2];
Char	expbuf[ESIZE+4];
int	circfl;
int	*zero, *dot, *dol;
Char	*endcore, *fendcore;
int	given;
int	*addr1;
int	*addr2;
Char	genbuf[LBSIZE];
long	count;
Char	*nextip;
Char	*linebp;
int	ninbuf;
int	io;
int	junk;
Char	suffix;
RETSIGTYPE	(*onhup)();
RETSIGTYPE	(*onquit)();
int	nrflag =	1;
int	prflag =	1;
int	nlflag =	1;
int	nl0;
int	msflag =	1;
int	col;
Char	*globp;
int	dashc;
Char	**dashpp;
Char	*dashp;
int	globflag;
int	sav0;
int	stack0[16];
int	output =	1;
int	savout =	1;
int	tfile =	-1;
int	tline;
Char	*tfname;
Char	*loc1;
Char	*loc2;
Char	*locs;
Char	ibuff[512];
int	iblock =	-1;
Char	obuff[512];
int	oblock =	-1;
int	ichanged;
int	nleft;
int	errfunc();
Char	*putbuf(), *getline(), *getblock(), *place(), *putbufd();
int	*address(), *nhood(), *printnhood(), *move();
Char	useerr[] =	"Usage:  e [-|+|-command]* [file [arg]*]\n";
Char	tmperr[] =	"/tmp error\n";
int	names[27];
#define	ddot	names[26]
Char	*braslist[NBRA];
Char	*braelist[NBRA];

#include	<setjmp.h>
jmp_buf	env;

#if defined(CYGWIN)
Char	*mlh_sbrk();
#define MLHSIZE 100000
Char mlhbuf[MLHSIZE];
long mlh;
Char *
mlh_brk(addr)
	Char *addr;
{
	mlh = 0;
}
Char *
mlh_sbrk(incr)
	int incr;
{
	long mlhold = mlh;
	mlh = mlh + incr;
	if (mlh >= MLHSIZE || mlh < 0)
		return (Char *)(-1);
	return &mlhbuf[mlhold];
}
#else
#define mlh_brk brk
#define mlh_sbrk sbrk
#endif

int
main(argc, argv)
Char **argv;
{
	int quit();
	
	extern RETSIGTYPE onintr(),hupcatch();

	onquit = signal(SIGQUIT, SIG_IGN);
	onhup = signal(SIGHUP, hupcatch);
	fendcore = mlh_sbrk(60000);
	init();
	dashc = --argc;
	dashpp = argv+1;
	dashp = *dashpp;
	if (((int)signal(SIGINT, SIG_IGN) & 01) == 0)
		signal(SIGINT, onintr);
	setjmp(env);
	for(;;) {
		commands("", "", "", &argc, argv, &nlc1, &vac1);
		confirm('q',quit,1);
	}
}

commands(gg, ii, jj, argc, argv, nlcp, vacp)
int *argc;
Char *gg, *ii, *jj, **argv;
Char *nlcp, *vacp;
{
	int gettty(), getrest();
	int edit(), quit();
	register int *a1;
	register int c, temp;
	Char *stackgl;
	Char thisbuf[LBSIZE];
	int stacknl;

	tempbuf = thisbuf;
	iii = ii;
	jjj = jj;
	ggg = gg;
	varv = argv;
	varc = argc;
	nlcptr = nlcp;
	vacptr = vacp;
	for (;;) {
	if (suffix) {
		addr1 = addr2 = dot;
		switch(suffix) {
		case 'j':
			junk++;
		case 'n':
			printnhood(); /* no "a1 =" needed here! */
			break;
		case 'l':
			junk++;
		case 'p':
			squeeze(1);
			printlines();
			break;
		}
		suffix = '\0';
	}
	if(eqflag) {
		if(eqflag & 0200) eqflag = 0;
		else eqflag |= 0200;
	}
	if(!globp && !sav0 && nlflag) {
		if(dashc) {
			if((c=getchar())=='\n') error(ESYN);
			if (getchar()=='\n' && (c=='-'||c=='+')) {
				prflag = nrflag = msflag = (c=='+');
				continue;
			}
			else {
				ungetchar();
				if (c!='-') {
					initbuf(file, sizeof file);
					while (c != '\n') {
						putbuf(c);
						c = getchar();
					}
					putbuf(0);
					dashpp = NULL;
					dashc = 0;
					goto e;
				}
			}
		}
		else if(prflag) {
			write(1,prompt,sizeof prompt-1);
		}
	}
	eofok = 1;
	if((c=getchar())==EOF) return;
	else ungetchar();
	addr2 = 0;
	do {
		addr1 = addr2;
		if ((a1 = address(-1))==0) {
			c = lowcase();
			break;
		}
		addr2 = a1;
		if ((c=lowcase()) == ';') {
			dot = a1;
			if(dot<=zero) dot = zero+1;
			if(dot>dol) dot = dol;
			c = ',';
		}
	} while (c==',');
	if(addr2==0) {
		given = 0;
		addr2 = dot;
	}
	else given = 1;
	if (addr1==0) addr1 = addr2;
	if(addr2 < addr1) error(ENEGRANGE);
	a1 = addr1;
	switch(c) {

	case 'a':
		add();
		break;

	case 'c':
		getrange();
		squeeze(1);
		temp = getsuffix();
		delete();
		if (temp) append(getrest,addr1-1);
		else append(gettty,addr1-1);
		break;

	case 'd':
		getrange();
		squeeze(1);
		newline();
		delete();
		break;

	case 'e':
		setnoaddr();
		c = lowcase();
		if (c=='c') {
			if ((temp=getchar())!='\n') {
				if (getchar()!='\n') error(ESYN);
				if (temp=='0') ecflag = 0;
				else if (temp=='1') ecflag = 1;
				else error(ESYN);
			}
			else {
				putd((long) ecflag);
				putchar('\n');
			}
			break;
		}
		if((eqflag & 0177)!='e' || c==' ') {
			ungetchar();
			filename(1);
		}
		else if (c!='\n') error(ESYN);
	e:
		confirm('e',edit,0);
		break;

	case 'f':
		setnoaddr();
		if ((c = getchar()) != '\n') {
			ungetchar();
			filename(1);
			xrename();
		}
		else {
			puts(savedfile);
			putchar('\n');
		}
		break;

	case 'g':
		if(lowcase()=='s') global(3);
		else {
			ungetchar();
			global(1);
		}
		tempbuf = thisbuf;
		break;

	case 'i':
		if(given || dol>zero) {
			--addr1;
			--addr2;
		}
		add();
		break;

	case 'j':
		junk++;
		a1 = nhood();
		break;

	case 'k':
		if ((c = lowcase()) < 'a' || c > 'z')
			error(EMKRANGE);
		newline();
		squeeze(1);
		names[c-'a'] = *addr2 | 01;
		break;

	case 'm':
		if((c=lowcase())=='s') {
			temp = getchar();
			if(temp=='.' || temp=='\'') {
				while((c=getchar())!='\n') putchar(c);
				if(temp=='.') putchar('\n');
				else sends();
				break;
			}
			error(ESYN);
		}
		ungetchar();
		a1 = move(0);
		break;

	case '\n':
		if (pmode=='j' || pmode=='l') junk++;
		if (pmode=='j' || pmode=='n') {
			if(!given) addr2 += nhsize;
			a1 = printnhood();
		}
		else {
			if(!given) addr1 = addr2 += 1;
			squeeze(1);
			dot = addr2;
			printlines();
		}
		break;

	case 'n':
		switch(c=lowcase()) {

		case 'l':
			setnoaddr();
			if ((c = getchar()) != '\n') {
				while (c == ' ')
					c = getchar();
				if (c=='\n') *nlcptr = 0;
				else if(getchar()!='\n') error(ESINGLE);
				else *nlcptr = c;
			}
			else {
				putchar(*nlcptr);
				putchar('\n');
			}
			break;

		case 'p':
			setnoaddr();
			if ((c=getchar())!='\n') {
				if(c=='.') c = nrflag;
				else c -= '0';
				if((temp=getchar())=='.') temp=prflag;
				else temp -= '0';
				if((c|temp|01) != 1) error(ERANGE);
				if (getchar()!='\n') error(ESYN);
				nrflag = c;
				prflag = temp;
			}
			else {
				putd((long) nrflag);
				putd((long) prflag);
				putchar('\n');
			}
			break;

		case 's':
			setnoaddr();
			if ((c=getchar()) != '\n') {
				ungetchar();
				addr1 = (int *) getd();
				if (getchar()!=',') error(ESYN);
				addr2 = (int *) getd();
				if (getchar()!='\n') error(ESYN);
				nhsize = (nh1=(int)addr1)+(nh2=(int)addr2)+1;
			}
			else {
				putd((long) nh1);
				putchar(',');
				putd((long) nh2);
				putchar('\n');
			}
			break;

		default:
			ungetchar();
			a1 = nhood();
			break;
		}
		break;

	case 'o':
		squeeze(dol>zero);
		dot = addr2;
		if((c=lowcase())=='\n') {
			putchar(pmode);
			putchar('\n');
		}
		else {
			newline();
			if(c!='n' && c!='p' && c!='l' && c!='j') 
				error(EBADMODE);
			pmode = c;
		}
		break;

	case 'l':
		junk++;
	case 'p':
		getrange();
		squeeze(1);
		if (getchar()!='\n') error(ESYN);
		dot = addr2;
		printlines();
		break;

	case 'q':
		setnoaddr();
		if ((c=lowcase()) == '\n') confirm('q',quit,0);
		else if(
			c=='u' &&
			lowcase()=='i' &&
			lowcase()=='t' &&
			getchar()=='\n'
		) {
			quit(0);
		}
		else error(ESYN);
		break;

	case 'r':
		filename(1);
		if (file[0] == 0) error(ENOFNAME);
		if ((io = open(file,0)) < 0) error(EOPEN);
		readfile();
		break;

	case 's':
		if (lowcase()=='h') {
			setnoaddr();
			if (getchar()!='\n') error(ESYN);
			if (*varc > 0) {
				--*varc;
				for (c=0; c <= *varc; c++) {
					varv[c] = varv[c+1];
				}
			}
			break;
		}
		ungetchar();
		getrange();
		squeeze(1);
		substitute((int)globp);
		break;

	case 't':
		a1 = move(1);
		break;

	case 'u':
		unite();
		break;

	case 'v':
		if ((c=lowcase()) == 'b') {
			setnoaddr();
			if ((c = getchar()) != '\n') {
				while (c == ' ')
					c = getchar();
				if (c=='\n') *vacptr = 0;
				else if(getchar()!='\n') error(ESYN);
				else *vacptr = c;
			}
			else {
				putchar(*vacptr);
				putchar('\n');
			}
			break;
		}
		ungetchar();
		global(0);
		tempbuf = thisbuf;
		break;

	case 'w':
		getrange();
		setwide();
		if ((temp=lowcase())!='q' && temp != 'w') ungetchar();
		squeeze(dol>zero);
		filename(1);
		if (file[0]==0) error(ENOFNAME);
	if (temp == 'w') {
		if ((io = open(file, O_WRONLY | O_APPEND | O_CREAT, 0664)) >= 0) {
		}else{
			error(EOPEN);
		}
	}else{
		if ((io = creat(file, 0664)) < 0)
			error(ECREAT);
	}
		count = 0L;
		if(dol>zero) putfile();
		exfile();
		if (!given) changes = 0;
		if(temp=='q') confirm('q',quit,0);
		break;

	case 'x':
		xmacro();
		tempbuf = thisbuf;
		iii = ii;
		jjj = jj;
		ggg = gg;
		varv = argv;
		varc = argc;
		nlcptr = nlcp;
		vacptr = vacp;
		break;

	case ':':
		setnoaddr();
		if ((c = getchar()) != ' ' && c != '\n') error(ESYN);
		while (c != '\n') c = getchar();
		break;

	case '=':
		setwide();
		squeeze(0);
		newline();
		putd((long) (addr2-zero));
		putchar('\n');
		break;

	case '!':
		callunix();
		break;

	case '>':
		setnoaddr();
		if ((c=getchar())=='>') temp = 1;
		else {
			temp = 0;
			ungetchar();
		}
		filename(0);
		if (file[0] == 0) endout();
		else {
			COLON = '>';
			if(temp && (output=open(file,1))>=0) {
			        lseek(output, 0L, 2);
			}
			else if ((output = creat(file, 0664)) < 0)
				error(ECREAT);
			savout = output;
		}
		break;

	case '<':
		setnoaddr();
		filename(0);
		if (file[0]==0) error(ENOFNAME);
		dostack (&stacknl, &stackgl);
		if(open(file,0) != 0) error(EOPEN);
		commands(ggg, iii, jjj, varc, varv, nlcptr, vacptr);
		unstack(stacknl, stackgl);
		tempbuf = thisbuf;
		break;

	default:
		error(ESYN);
	}
	if(given) ddot = *a1 | 01;
	}
}

int *
address(ii)
{
	register *a1, minus, c;
	int n, relerr;

	minus = 0;
	a1 = 0;
	for (;;) {
		if (ii < 0) n = getd();
		else {
			n = ii;
			ii = -1;
		}
		if (n >= 0) {
			if (a1==0)
				a1 = zero;
			if (minus<0)
				n = -n;
			a1 += n;
			minus = 0;
		}
		relerr = 0;
		if (a1 || minus)
			relerr++;
		switch(c = getchar()) {

		case ' ':
		case '\t':
			continue;

		case '+':
			minus++;
			if (a1==0)
				a1 = dot;
			continue;

		case '-':
		case '^':
			minus--;
			if (a1==0)
				a1 = dot;
			continue;

		case '?':
		case '/':
			compile(c);
			a1 = dot;
			for (;;) {
				if (c=='/') {
					a1++;
					if (a1 > dol)
						a1 = zero;
				} else {
					a1--;
					if (a1 < zero)
						a1 = dol;
				}
				if (execute(0, a1))
					break;
				if (a1==dot && goonmlh) break;
				if (a1==dot) error(EMATCH);
			}
			break;

		case '$':
			a1 = dol;
			break;

		case '.':
			if((c=getchar())=='.') {
				for (a1=zero+1;;a1++) {
					if(a1>dol) error(ERANGE);
					if(ddot==((*a1)|01)) break;
				}
			}
			else {
				ungetchar();
				a1 = dot;
			}
			break;

		case '\'':
			if ((c = lowcase()) < 'a' || c > 'z')
				error(EMKRANGE);
			for (a1=zero+1;;a1++) {
				if(a1>dol) error(ERANGE);
				if (names[c-'a'] == (*a1|01)) break;
			}
			break;

		default:
			ungetchar();
			if (a1==0)
				return(0);
			a1 += minus;
			return(a1);
		}
		if (relerr)
			error(EHUH);
	}
}

getd()
{
	register c, nn, digits;

	nn = 0;
	digits = 0;
	while ((c=getchar()) >= '0' && c <= '9') {
		digits++;
		nn = nn*10 + c - '0';
		if (nn < 0) error(ERANGE);
	}
	ungetchar();
	if (digits) return(nn);
	return(-1);
}

getrange()
{
	register ii;

	if ((ii = getd()) > 0) {
		given = 1;
		addr2 += --ii;
	}
	else if (ii==0) error(ERANGE);
}

setwide()
{
	if(!given) {
		addr1 = zero;
		addr2 = dol;
	}
}

setnoaddr()
{
	if(given) error(ENOADDR);
}

squeeze(i)
register int i;
{
	register int *j;

	j = zero+i;
	if(j>dol) error(ERANGE);
	if(addr2<j) error(ENEGRANGE);
	if(addr1>dol) error(ERANGE);
	if(addr1<j) addr1 = j;
	if(addr2>dol) addr2 = dol;
}

newline()
{
	if (getsuffix()) error(ESYN);
}

getsuffix()
{
	register c;

	if ((c=lowcase())=='\n') return(0);
	if(c=='p' || c=='l' || c=='n' || c=='j') {
		suffix = c;
		if ((c=lowcase())=='\n') return(0);
	}
	if(c==SUFF) return (1);
	error(ESYN);
}

filename(flag)
{
	register Char *p1, *p2;
	register c;

	p1 = file;
	if(flag) {
		c = getchar();
		if(c=='\n') {
			p2 = savedfile;
			if (*p2==0)
				error(ENOFNAME);
			while (*p1++ = *p2++);
			return;
		}
		if (c!=' ')
			error(ESYN);
	}
	while ((c = getchar()) == ' ');
	if (c!='\n') do {
		if(p1 >= &file[sizeof file - 1]) error(EFTOOBIG);
		*p1++ = c;
	} while ((c = getchar()) != '\n' && c != ' ');
	*p1++ = 0;
}

xrename()
{
	register Char *p1, *p2;
	p1 = savedfile;
	p2 = file;
	while (*p1++ = *p2++);
}

readfile()
{
	int getfile();
	setwide();
	squeeze(0);
	ninbuf = 0;
	count = 0L;
	append(getfile, addr2);
	exfile();
}

int *
printnhood()
{
	register int *a;

	if(addr2<=zero) addr2=zero+1;
	a = addr2;
	addr1 = a - nh1;
	addr2 += nh2;
	squeeze(1);
	if(a>dol) a=dol;
	dot = a;
	printlines();
	return(a);
}

int *
nhood()
{
	register c;

	while ((c = getchar()) != '\n') {
		if (c=='+') addr2 += nhsize;
		else if (c=='-') addr2 -= nhsize;
		else error(ESYN);
	}
	return(printnhood());
}

printlines()
{
	register *a1;
	a1 = addr1;
	while (a1<=addr2) {
		col = 0;
		if (nrflag) putmarg (
			(a1!=dot) ? ' ' : '>',
			a1, dol,
			junk ? LJUNK : LMARG
		);
		puts(getline(*a1++));
		putchar('\n');
	}
		junk = '\0';
}

putmarg (b, a1, a2, c)
register int *a1, *a2;
Char b, c;
{
	putchar (b);
	putd ((long) ((a1-zero) & 077777));
	if (putchar((a1!=a2) ? ' ' : '$') < 7)
		while (putchar(' ') < 7);
	putchar (c);
}

exfile()
{
	close(io);
	io = -1;
	if (msflag) {
		putd(count);
		puts("; ");
		puts(savedfile);
		putchar('\n');
	}
}

int	gothome;
Char	ebin[PWSIZ];
Char	bin[PWSIZ];
Char	home[PWSIZ];
#define	PWSIZ	80
#define	pidp	(tfname+6)

gethome()
{
/* do not compile only to avoid the warning about
the `getpw' function is dangerous and should not be used.
*/
#if 0 && defined(HAVE_GETPW)
	register int i;
	register Char *s, *t;

	if (gothome) return;
	if(getpw(getuid(),home)) {
		home[0] = 0;
		return;
	}
	s = home;
	t = home;
	i = 5;
	do {
		while (*t++ != ':');
	} while (--i);  /* ignore 5 ':' */
	while (*t != ':')
		*s++ = *t++;	/* pick up name */
	*s++ = '/';		/* add slash */
	*s = '\0';
	s = bin;
	t = home;
	while (*t) *s++ = *t++;
	*s++ = 'b';
	*s++ = 'i';
	*s++ = 'n';
	*s++ = '/';
	*s++ = '\0';
	s = ebin;
	t = home;
	while (*t) *s++ = *t++;
	*s++ = 'e';
	*s++ = 'b';
	*s++ = 'i';
	*s++ = 'n';
	*s++ = '/';
	*s++ = '\0';
	gothome++;
#endif
}

Char	vbuf[LBSIZE];
Char	nnn[6];

getvar(c)
register c;
{
	register i;
	register Char *t;
	Char *s;

	echo ("{");
	if ('0' <= c && c <= '9') {
		if (c <= '0'+ *varc) varp = varv[c-'0'];
	}
	else switch (c) {
	case 'A':
		s = vbuf;
		if (*varc > 0) for (i = 1; ;) {
			t = varv[i];
			while (*t) {
				if (s >= &vbuf[sizeof vbuf -1])
					error(EVAR);
				*s++ = *t++;
			}
			if (i >= *varc) break;
			*s++ = ' ';
			i++;
		}
		*s = 0;
		varp = vbuf;
		break;
	case 'B':
		gethome();
		varp = bin;
		break;
	case 'D':
		varp = (Char *)"\177";
		break;
	case 'E':
		gethome();
		varp = ebin;
		break;
	case 'F':
		varp = savedfile;
		break;
	case 'G':
		varp = ggg;
		break;
	case 'H':
		gethome();
		varp = home;
		break;
	case 'I':
		varp = iii;
		break;
	case 'J':
		varp = jjj;
		break;
	case 'N':
		c = *varc;
		for(i=4; i>=0; i--) {
			c = ((long) c) / 10;
			nnn[i] = (((long) c) % 10) + '0';
			if (!c) break;
		}
		varp = nnn+i;
		break;

	case 'K':
		gethome();
		varp = pidp;
		break;
	}
	if (!varp) echo ("}");
	return (getchar());
}

lowcase()
{
	register c;

	c = getchar();
	if (c>='A' && c<='Z') c += 'a' - 'A';
	return(c);
}

Char	peekc;
Char	peekbs;

getchar()
{
	if (peekc) {
		lastc=peekc;
		peekc=0;
	}
	else if (varp) {
		if (*varp) lastc = *varp++;
		else {
			varp = 0;
			echo ("}");
			return (getchar());
		}
		echo (&lastc);
	}
	else if (globp) {
		if(*globp) lastc = *globp++;
		else {
			globp=0;
			lastc = EOF;
		}
	}
	else if (dashpp && !sav0) {
		if(dashc<=0) {
			dashpp = 0;
			return (getchar());
		}
		else if(*dashp) lastc = *dashp++;
		else {
			dashp = *++dashpp;
			--dashc;
			lastc = '\n';
		}
	}
	else if (peekbs) {
		lastc = peekbs;
		peekbs = 0;
	}
	else {
		nlflag = 1;
		lastc = getch();
		if(lastc=='\\') {
			if((peekbs=getch())== *nlcptr || peekbs== *vacptr) {
				lastc = peekbs;
				peekbs = 0;
			}
		}
		else if(lastc== *nlcptr) {
			lastc = '\n';
			nlflag = 0;
		}
		else if (lastc == *vacptr) {
			lastc = getch();
			return(getvar(lastc));
		}
	}

	if(lastc==EOF && !eofok) {
		lastc = '\n';
		error(EEOF);
	}
	eofok = 0;
	return(lastc);
}

ungetchar()
{
	peekc = lastc;
}

getch()
{
	Char chr;

	if (read(0,&chr,1) <= 0) {
		return(EOF);
	}
	chr &= 0177;
	echo (&chr);
	return(chr);
}

gety()
{
	register c;
	register Char *p;

	p = linebuf;
	while ((c = getchar()) != '\n') {
		*p++ = c;
		if (p >= &linebuf[LBSIZE-2])
			error(ELINE);
	}
	*p++ = 0;
}

int restflag;

getrest()
{
	if (restflag) {
		restflag = 0;
		return(EOF);
	}
	restflag++;
	gety();
	return(0);
}

gettty()
{
	register Char *gf;
/*
 * Use dashc below rather than dashpp because it becomes 0 as soon
 * as the last Character of the last dash arg is read, whereas
 * dashpp doesn't become 0 until the next Character.
 */
	if (!dashc && !globp && nlflag && !sav0 && prflag) {
		output = 1;
		putmarg ('e', dot+1, dol+1, COLON);
		sends();
		output = savout;
	}
	gf = globp;	/* Is gf really necessary? -jte */
	eofok = 1;
	if(getchar()==EOF) {
		if(gf) {
			globp = (Char *)"";
		}
		return(EOF);
	}
	else ungetchar();
	gety();
	if (linebuf[0]=='.' && linebuf[1]==0)
		return(EOF);
	return(0);
}

getfile()
{
	register c;
	register Char *lp, *fp;

	lp = linebuf;
	fp = nextip;
	do {
		if (--ninbuf < 0) {
			if ((ninbuf = read(io, genbuf, LBSIZE)-1) < 0) {
				if (lp>linebuf) {
					puts("'\\n' appended\n");
					*genbuf = '\n';
				}
				else return(EOF);
			}
			fp = genbuf;
		}
		if (lp >= &linebuf[LBSIZE]-1 && *fp != '\n') {
			*lp++ = c = '\n';
			puts("'\\n' inserted\n");
			ninbuf++;
		} else if ((*lp++ = c = *fp++ & 0177) == 0) {
			lp--;
			continue;
		}
		++count;
	} while (c != '\n');
	*--lp = 0;
	nextip = fp;
	return(0);
}

putfile()
{
	int *a1;
	register Char *fp, *lp;
	register nib;

	nib = 512;
	fp = genbuf;
	a1 = addr1;
	do {
		lp = getline(*a1++);
		for (;;) {
			if (--nib < 0) {
				if (write(io, genbuf, fp-genbuf) != fp-genbuf)
					error(EWRITE);
				nib = 511;
				fp = genbuf;
			}
			++count;
			if ((*fp++ = *lp++) == 0) {
				fp[-1] = '\n';
				break;
			}
		}
	} while (a1 <= addr2);
	if (write(io, genbuf, fp-genbuf) != fp-genbuf) error(EWRITE);
}

append(f, a)
int (*f)(), *a;
{
	register *a1, *a2, *rdot;
	int nline, tl;

	nline = 0;
	dot = a;
	while ((*f)() == 0) {
		if ((Char *)dol >= endcore) {
			if ((int)mlh_sbrk(1024) == -1)
				error(ESPACE);
			endcore += 1024;
		}
		tl = putline();
		nline++;
		a1 = ++dol;
		a2 = a1+1;
		rdot = ++dot;
		while (a1 > rdot)
			*--a2 = *--a1;
		*rdot = tl;
		changes = 1;
	}
	return(nline);
}

add()
{
	squeeze(0);
	if (getsuffix()) append(getrest,addr2);
	else append(gettty,addr2);
}

callunix()
{
	register RETSIGTYPE (*savintr)();
	register int pid, c, savint;
	int retcode;

	setnoaddr();
	initbuf(tempbuf, LBSIZE);
	while((c=getchar())!='\n') putbuf(c);
	putbuf(0);
	if ((pid = fork()) == 0) {
		signal(SIGHUP, onhup);
		signal(SIGQUIT, onquit);
		savint = 0;
		if (tempbuf[savint] == '!') {
			savint++;
			if (sav0) {
				close (0);
				dup (stack0[1]);
			}
		}
		if (!tempbuf[savint]) execl(shname,"sh","-i",0);
		else execl(shname,"sh","-c",tempbuf+savint,0);
		exit(1);
	}
	savintr = signal(SIGINT, SIG_IGN);
	while ((c = wait(&retcode)) != pid && c != -1);
	signal(SIGINT, savintr);
	if (!sav0 && msflag) {
		puts("!; ");
		puts(savedfile);
		putchar('\n');
	}
}

delete()
{
	register *a1, *a2, *a3;
	int len;

	a1 = addr1;
	a2 = addr2+1;
	a3 = dol;
	len = a2 - a1;
#if 0
	if (len > nhsize) {
		register int c;

		puts("delete > neighborhood; delete?");
		sends();
		if ((c = lowcase()) != '\n')
			 while (getchar() != '\n');
		if (c != 'y' && c != 'd')
			return;
	}
#endif
	changes = 1;
	dol -= len;
	do
		*a1++ = *a2++;
	while (a2 <= a3);
	a1 = addr1;
	if (a1 > dol)
		a1 = dol;
	dot = a1;
}

Char *
getline(tl)
{
	register Char *bp, *lp;
	register nl;

	lp = linebuf;
	bp = getblock(tl, READ);
	nl = nleft;
	tl &= ~0377;
	while (*lp++ = *bp++)
		if (--nl == 0) {
			bp = getblock(tl += 0400, READ);
			nl = nleft;
		}
	return(linebuf);
}

unite()
{
	register Char *bp, *lp;
	register nl;
	int tl, *a1;

	getrange();
	squeeze(1);
	initbuf(tempbuf, LBSIZE);
	if(getsuffix()) while((nl=getchar())!='\n') putbuf(nl);
	putbuf(0);
	lp = linebuf;
	a1 = addr1;
	while (a1<=addr2) {
		tl = *a1++;
		bp = getblock(tl, READ);
		nl = nleft;
		tl &= ~0377;
		while (*lp++ = *bp++) {
			if (lp >= &linebuf[LBSIZE-2])
				error(ELINE);
			if (--nl == 0) {
				bp = getblock(tl += 0400, READ);
				nl = nleft;
			}
		}
		lp--;
		if(a1>addr2) break;
		bufp = tempbuf;
		while(*lp++ = *bufp++)
			if(lp - linebuf >= LBSIZE-2) error(ELINE);
		lp--;
	}
	*addr2 = putline();
	if (addr1 <= --addr2) {
		changes = 1;
		delete();
	}
	else dot = addr1;
}

putline()
{
	register Char *bp, *lp;
	register nl;
	int tl;

	lp = linebuf;
	tl = tline;
	bp = getblock(tl, WRITE);
	nl = nleft;
	tl &= ~0377;
	while (*bp = *lp++) {
		if (*bp++ == '\n') {
			*--bp = 0;
			linebp = lp;
			break;
		}
		if (--nl == 0) {
			bp = getblock(tl += 0400, WRITE);
			nl = nleft;
		}
	}
	nl = tline;
	tline += (((lp-linebuf)+03)>>1)&077776;
	return(nl);
}

Char *
getblock(atl, iof)
{
	register bno, off;
	bno = (atl>>8)&037777;
	off = (atl<<1)&0774;
	if (bno >= 037777) {
		puts(tmperr);
		lastc = '\n';	/*  fixup for error routine  */
		error(ETMP);
	}
	nleft = 512 - off;
	if (bno==iblock) {
		ichanged |= iof;
		return(ibuff+off);
	}
	if (bno==oblock)
		return(obuff+off);
	if (iof==READ) {
		if (ichanged) {
			blkio(iblock, ibuff, write);
		}
		ichanged = 0;
		iblock = bno;
		blkio(bno, ibuff, read);
		return(ibuff+off);
	}
	if (oblock>=0)
		blkio(oblock, obuff, write);
	oblock = bno;
	return(obuff+off);
}

blkio(b, buf, iofcn)
register Char *buf;
register int b;
int (*iofcn)();
{
	lseek(tfile, ( (long)b * 512L), 0);
	if ((*iofcn)(tfile, buf, 512) != 512) {
		puts(tmperr);
		error(ETMP);
	}
}
confirm(c,f,arg)
Char c;
int (*f)();
{

	if(changes && !eqflag && msflag) {
		EQ = c;
		EQEND = COLON;
		write(1,eqprompt,sizeof eqprompt - 1);
		nlflag = 0;
		eqflag = c;
		eofok = 1;
		closeinputs();
		return;
	}
	(*f)(arg);
}

edit()
{
	xrename();
	init();
	changes = 0;
	addr2 = zero;
	if (file[0]==0) error(ENOFNAME);
	if ((io = open(file,0))<0) {
		if (msflag) {
			puts("new; ");
			puts(savedfile);
			putchar('\n');
		}
	}
	else readfile();
	changes = 0;
}

char temp_buf[] = "/tmp/exxxxx";

init()
{
	register Char *p;
	register pid;

	close(tfile);
	tline = 0;
	iblock = -1;
	oblock = -1;
/*
  This code used to depend on having strings be writable.  It has been fixed
  to be standard-compliant, so it doesn't dump core.
 */
/*	tfname = (Char *)"/tmp/exxxxx"; */
	tfname = (Char *)temp_buf;
	ichanged = 0;
#if 1
	pid = getpid();
	for (p = &tfname[11]; p > &tfname[6];) {
		*--p = (pid&07) + '0';
		pid >>= 3;
	}
#endif
	close(creat(tfname, 0600));
	tfile = open(tfname, O_RDWR);
	mlh_brk(fendcore);
	dot = zero = dol = (int *)fendcore;
	endcore = fendcore - 4;		/* Why is -4 here? -jte */
}

global(k)
{
	register Char *gp;
	register c;
	register int *a1;
	Char globuf[GBSIZE];

	if(globflag) error(EGLOB);
	getrange();
	if ((c=getchar())=='\n')
		error(ESYN);
	setwide();
	squeeze (dol>zero);
	compile(c);
	gp = globuf;
	if(k & 2) {
		k -= 2;
		*gp++ = 's';
		*gp++ = c;
		*gp++ = c;
	}
	while ((c = getchar()) != '\n') {
		if (c=='\\') {
			c = getchar();
			if (c!='\n')
				*gp++ = '\\';
		}
		*gp++ = c;
		if (gp >= &globuf[GBSIZE-2])
			error(ELINE);
	}
	*gp++ = '\n';
	*gp++ = 0;
	if(addr2<addr1) return;
	for (a1=zero; a1<=dol; a1++) {
		*a1 &= ~01;
		if (a1>=addr1 && a1<=addr2 && execute(0, a1)==k)
			*a1 |= 01;
	}
	globflag++;
	for (a1=zero; a1<=dol; a1++) {
		if (*a1 & 01) {
			*a1 &= ~01;
			dot = a1;
			globp = globuf;
			commands(ggg, iii, jjj, varc, varv, nlcptr, vacptr);
			a1 = zero;
		}
	}
	globflag = 0;
}

substitute(inglob)
{
	register *a1, loc, nl;
	int getsub();
	Char *stackgl;
	int stacknl;

	compsub();
	dostack (&stacknl, &stackgl);
	if (dup(stack0[1]) != 0) error(EDUP);
	nlflag = nl0;
	for (a1 = addr1; a1 <= addr2; a1++) {
		if (execute(0, a1)==0)
			continue;
		inglob |= 01;
		trysub(a1);
		if(gsubf=='g' || gsubf=='i') {
			while(*loc2 && gsubf!='q') {
				if (execute(1, (int *)NULL)==0)
					break;
				trysub(a1);
			}
		}
		loc = *a1;
		*a1 = putline();
		for(nl=0;nl<27;nl++)
			if(names[nl] == (loc|01)) names[nl] = *a1 | 01;
		nl = append(getsub, a1);
		a1 += nl;
		addr2 += nl;
		if(gsubf=='q') break;
	}
	nl0 = nlflag;
	unstack(stacknl, stackgl);
	if (inglob==0 && !goonmlh) error(EMATCH);
}

trysub(linenr)
int	*linenr;
{
	register c1;

	if(gsubf!='i') {
		dosub(rhsbuf);
		changes = 1;
		return;
	}
	junk = (pmode=='j'||pmode=='l');
	if(suffix) junk = (suffix=='j'||suffix=='l');
	col = 0;
	if (nlflag) {
		if (nrflag) putmarg ('s', linenr, dol, (junk ? LJUNK : LMARG));
		puts(linebuf);
		putchar('\n');
		underline(linebuf, loc1, loc2, '^');
	}
	for(;;) {
		switch(lowcase()) {

		case 'q':
			if(getchar()!='\n') break;
			gsubf = 'q';

		case '\n':
			dosub("&");
			goto endsub;

		case 's':
			c1 = lowcase();
			if (c1 == 'q') {
				gsubf = c1;
				c1 = getchar();
			}
			if(c1=='\n') {
				dosub(rhsbuf);
				changes = 1;
				goto endsub;
			}
			if(c1!='.') break;
			initbuf (tempbuf, LBSIZE);
			for (;;) {
				c1 = getchar();
				if (c1=='\\') c1 = getchar() | 0200;
				if (c1=='\n') break;
				putbuf(c1);
			}
			putbuf(0);
			dosub(tempbuf);
			changes = 1;
			goto endsub;
		}
		while((c1=getchar())!='\n');
		if (nlflag) underline(linebuf, loc1, loc2, '?');
	}
endsub:
	junk = 0;
}

underline (line, l1, l2, score)
Char *line, *l1, *l2, score;
{
	Char *ll;
	register Char *p, ch;
	register int i, ip;

	p = line;
	ch = ' ';
	ll = l1;
	i = 2;
	col = 0;
	if(nrflag) puts("        ");
	while (i--) {
		while (*p && p < ll) {
			ip = *p;
			if (ip>=' ' && ip<= 0177/*'\0177'*/) putchar(ch);
			else if(junk) {
				if(ip=='\t' || ip=='\b') putchar(ch);
				else {
					putchar(ch);
					putchar(ch);
					putchar(ch);
					if(ip== 0177/*'\0177'*/) putchar(ch);
				}
			}
			else putchar(*p);
			p++;
		}
		ch = score;
		ll = l2;
	}
	sends();
}

compsub()
{
	register seof, c;
	register Char *p;

	if ((seof = getchar()) == '\n') error(ESYN);
	compile(seof);
	p = rhsbuf;
	c = getchar();
	ungetchar();
	if (c=='\n') error(ESYN);
	for (;;) {
		c = getchar();
		if (c=='\\')
			c = getchar() | 0200;
		if (c=='\n') {
			ungetchar();
			break;
		}
		if (c==seof)
			break;
		*p++ = c;
		if (p >= &rhsbuf[LBSIZE/2])
			error(ELINE);
	}
	*p++ = 0;
	if((c=lowcase()) == 'g' || c=='i') {
		gsubf=c;
	}
	else {
		ungetchar();
		gsubf = 0;
	}
	newline();
	return;
}

getsub()
{
	register Char *p1, *p2;

	p1 = linebuf;
	if ((p2 = linebp) == 0)
		return(EOF);
	while (*p1++ = *p2++);
	linebp = 0;
	return(0);
}

dosub(buffer)
Char *buffer;
{
	register Char *lp, *sp, *rp;
	int c;

	lp = linebuf;
	sp = genbuf;
	while (lp < loc1)
		*sp++ = *lp++;
	rp = buffer;
	while (c = *rp++) {
		if (c=='&') {
			sp = place(sp, loc1, loc2);
			continue;
		}
		else if (c<0) {
			if((c &= 0177) == 'n') c = '\n';
			else if(c >='1' && c < NBRA+'1') {
				sp = place(sp, braslist[c-'1'], braelist[c-'1']);
				continue;
			}
		}
		*sp++ = c&0177;
		if (sp >= &genbuf[LBSIZE])
			error(ELINE);
	}
	lp = loc2;
	loc2 = sp - genbuf + linebuf;
	while (*sp++ = *lp++)
		if (sp >= &genbuf[LBSIZE])
			error(ELINE);
	lp = linebuf;
	sp = genbuf;
	while (*lp++ = *sp++);
}

Char *
place(sp, l1, l2)
register Char *sp, *l1, *l2;
{

	while (l1 < l2) {
		*sp++ = *l1++;
		if (sp >= &genbuf[LBSIZE])
			error(ELINE);
	}
	return(sp);
}

int *
move(cflag)
{
	register int *adt, *ad1, *ad2;
	int ii;
	int getcopy();

	if ((ii=getd())>=0) {
		if (getchar()==',') {
			if (ii==0) error(ESYN);
			addr2 += --ii;
			ii = -1;
		}
		else ungetchar();
	}
	squeeze(1);
	if ((adt = address(ii))==0 || adt < zero || adt > dol)
		error(ERANGE);
	newline();
	changes = 1;
	given = 1; /* set given so that ddot will be set */
	ad1 = addr1;
	ad2 = addr2;
	if (cflag) {
		ad1 = dol;
		append(getcopy, ad1++);
		ad2 = dol;
	}
	ad2++;
	if (adt<ad1) {
		dot = adt + (ad2-ad1);
		if ((++adt)==ad1)
			return(adt);
		reverse(adt, ad1);
		reverse(ad1, ad2);
		reverse(adt, ad2);
	} else if (adt >= ad2) {
		dot = adt++;
		reverse(ad1, ad2);
		reverse(ad2, adt);
		reverse(ad1, adt);
		adt += ad1 - ad2;
	} else
		error(ENEGRANGE);
	return(adt);
}

dostack(stacknl, stackgl)
register *stacknl;
register Char **stackgl;
{
	if (sav0==0) nl0 = nlflag;
	*stacknl = nlflag;
	*stackgl = globp;
	globp = 0;
	if((stack0[++sav0]=dup(0)) <= 0) {
		--sav0;
		error(ESTACK);
	}
	close(0);
}

unstack(stacknl, stackgl)
register stacknl;
register Char *stackgl;
{
	close(0);
	dup(stack0[sav0]);
	close(stack0[sav0--]);
	globp = stackgl;
	nlflag = stacknl;
	if (sav0 == 0) nlflag = nl0;
}

xmacro()
{
	register Char c;
	Char *stackgl;
	register Char *ptr;
	Char *ii, *jj, *gg;
	Char nlc, vac;
	int newc;
	int delim;
	int savsuff;
	Char *newv[NARGS];
	Char *macnam1, *macnam2, *macnam3;
	int stacknl;

	getrange();
	c = lowcase();
	if (c=='n'||c=='l'||c=='p'||c=='j') savsuff = c;
	else {
		savsuff = 0;
		ungetchar();
	}
	if ((delim=getchar())=='\n') error(ESYN);
	if ((c=getchar())=='\n') error(ESYN);

	/* ~G (were addresses given?) */
	initbuf(tempbuf, LBSIZE);
	gg = tempbuf;
	if (given) {
		putbufd((long) ((addr1 - zero) & 077777));
		putbuf (',');
		putbufd((long) ((addr2 - zero) & 077777));
	}

	/* ~I (first address) */
	ii = putbuf('\0');
	putbufd((long) ((addr1 - zero) & 077777));

	/* ~J (second address) */
	jj = putbuf('\0');
	putbufd((long) ((addr2 - zero) & 077777));

	/* one possible name of macro */
	macnam3 = putbuf('\0');
	ptr = (Char *)"/usr/ebin/";
	while (*ptr) putbuf (*ptr++);
	macnam1 = bufp;
	while (c!='\n' && c!=delim) {
		if(c=='\\') {
			if((c=getchar())!=delim) putbuf('\\');
		}
		putbuf(c);
		c=getchar();
	}
	putbuf('.');
	putbuf('e');
	putbuf('m');

	newc = 0;
	/* ~0 (name of macro) */
	macnam2 = putbuf('\0');
	gethome();
	ptr = ebin;
	while (*ptr) putbuf(*ptr++);
	newv[newc++] = bufp;
	ptr = macnam1;
	while (*ptr) putbuf (*ptr++);

	/* ~1 thru ... */
	while(c!='\n') {
		if(c==delim) {
			newv[newc++] = putbuf('\0');
			if(newc>=NARGS) error(EARGS);
		}
		else if(c=='\\') {
			if((c=getchar())!=delim) putbuf('\\');
			putbuf(c);
		}
		else putbuf(c);
		c=getchar();
	}
	putbuf('\0');
	newv[newc] = 0;
	--newc;

	dostack (&stacknl, &stackgl);
	nlc = GRAVE;
	vac = TILDE;
	if (open (macnam1, 0) == 0) commands(gg, ii, jj, &newc, newv, &nlc, &vac);
	else if (macnam1[0] != '/') {
		if (open (macnam2, 0) == 0 || open (macnam3, 0) == 0)
			commands(gg, ii, jj, &newc, newv, &nlc, &vac);
		else error(ENOMACRO);
	}
	else error(ENOMACRO);
	unstack(stacknl, stackgl);
	suffix = savsuff;
}

reverse(a1, a2)
register int *a1, *a2;
{
	register int t;

	for (;;) {
		t = *--a2;
		if (a2 <= a1)
			return;
		*a2 = *a1;
		*a1++ = t;
	}
}

getcopy()
{
	if (addr1 > addr2)
		return(EOF);
	getline(*addr1++);
	return(0);
}

compile(eof)
register int eof;
{
	register c;
	register Char *ep;
	Char *lastep;
	Char bracket[NBRA], *bracketp;
	int nbra;
	int cclcnt;
	int tempc;

	ep = expbuf;
	bracketp = bracket;
	nbra = 0;
	if ((c = getchar())=='\n') {
		ungetchar();
		c = eof;
	}
	if (c==eof) {
		if (*ep==0)
			error(EREGEXP);
		return;
	}
	circfl = 0;
	if (c=='^') {
		c = getchar();
		circfl++;
	}
	if (c=='*')
		goto cerror;
	ungetchar();
	for (;;) {
		if (ep >= &expbuf[ESIZE])
			goto cerror;
		c = getchar();
		if (c=='\n') {
			ungetchar();
			c = eof;
		}
		if (c==eof) {
			*ep++ = CEOF;
			return;
		}
		if (c!='*')
			lastep = ep;
		switch (c) {

		case '\\':
			if ((c = getchar())=='(') {
				if (nbra >= NBRA)
					goto cerror;
				*bracketp++ = nbra;
				*ep++ = CBRA;
				*ep++ = nbra++;
				continue;
			}
			if (c == ')') {
				if (bracketp <= bracket)
					goto cerror;
				*ep++ = CKET;
				*ep++ = *--bracketp;
				continue;
			}
			*ep++ = CCHR;
			if (c=='\n')
				goto cerror;
			*ep++ = c;
			continue;

		case '.':
			*ep++ = CDOT;
			continue;

		case '\n':
			goto cerror;

		case '*':
			if (*lastep==CBRA || *lastep==CKET)
				error(EREGEXP);
			*lastep |= STAR;
			continue;

		case '$':
			tempc = getchar();
			ungetchar();
			if (tempc != eof && tempc != '\n')
				goto defChar;
			*ep++ = CDOL;
			continue;

		case '[':
			*ep++ = CCL;
			*ep++ = 0;
			cclcnt = 1;
			if ((c=getchar()) == '^') {
				c = getchar();
				ep[-2] = NCCL;
			}
			do {
				if (c == '\n')
					goto cerror;
/*
 *	Handle the escaped '-'
 */
				if (c == '-' && *(ep-1) == '\\')
					*(ep-1) = '-';
/*
 *	Handle ranges of Characters (e.g. a-z)
 */
				else if (
					(tempc = getchar()) != ']' &&
					c == '-' && cclcnt > 1 &&
					tempc != '\n' &&
					(c = *(ep-1)) <= tempc
				) {
					while (++c <= tempc) {
						*ep++ = c;
						cclcnt++;
						if (ep >= &expbuf[ESIZE])
							goto cerror;
					}
				}
/*
 *	Normal case.  Add Character to buffer
 */
				else {
					ungetchar();
					*ep++ = c;
					cclcnt++;
					if (ep >= &expbuf[ESIZE])
						goto cerror;
				}
			} while ((c = getchar()) != ']');
			lastep[1] = cclcnt;
			continue;

		defChar:
		default:
			*ep++ = CCHR;
			*ep++ = c;
		}
	}
	cerror:
	expbuf[0] = 0;
	error(EREGEXP);
}

execute(gf, addr)
int *addr;
{
	register Char *p1, *p2, c;

	if (gf) {
		if (circfl)
			return(0);
		p1 = linebuf;
		p2 = genbuf;
		while (*p1++ = *p2++);
		locs = p1 = loc2;
	} else {
		if (addr==zero)
			return(0);
		p1 = getline(*addr);
		locs = NULL;
	}
	p2 = expbuf;
	if (circfl) {
		loc1 = p1;
		return(advance(p1, p2));
	}
	/* fast check for first Character */
	if (*p2==CCHR) {
		c = p2[1];
		do {
			if (*p1!=c)
				continue;
			if (advance(p1, p2)) {
				loc1 = p1;
				return(1);
			}
		} while (*p1++);
		return(0);
	}
	/* regular algorithm */
	do {
		if (advance(p1, p2)) {
			loc1 = p1;
			return(1);
		}
	} while (*p1++);
	return(0);
}

advance(lp, ep)
register Char *lp, *ep;
{
	register Char *curlp;

	for (;;) switch (*ep++) {

	case CCHR:
		if (*ep++ == *lp++)
			continue;
		return(0);

	case CDOT:
		if (*lp++)
			continue;
		return(0);

	case CDOL:
		if (*lp==0)
			continue;
		return(0);

	case CEOF:
		loc2 = lp;
		return(1);

	case CCL:
		if (cclass(ep, *lp++, 1)) {
			ep += *ep;
			continue;
		}
		return(0);

	case NCCL:
		if (cclass(ep, *lp++, 0)) {
			ep += *ep;
			continue;
		}
		return(0);

	case CBRA:
		braslist[*ep++] = lp;
		continue;

	case CKET:
		braelist[*ep++] = lp;
		continue;

	case CDOT|STAR:
		curlp = lp;
		while (*lp++);
		goto star;

	case CCHR|STAR:
		curlp = lp;
		while (*lp++ == *ep);
		ep++;
		goto star;

	case CCL|STAR:
	case NCCL|STAR:
		curlp = lp;
		while (cclass(ep, *lp++, ep[-1]==(CCL|STAR)));
		ep += *ep;
		goto star;

	star:
		do {
			lp--;
			if (lp==locs)
				break;
			if (advance(lp, ep))
				return(1);
		} while (lp > curlp);
		return(0);

	default:
		error(EREGEXP);
	}
}

cclass(set, c, af)
register Char *set, c;
{
	register n;

	if (c == 0)
		return(0);
	n = *set++;
	while (--n)
		if (*set++ == c)
			return(af);
	return(!af);
}

quit(value)
{
	unlink(tfname);
	exit(value);
}

putd(nn)
long nn;
{
	register int rem;
	long quo;

	quo = nn / 10;
	rem = nn % 10;
	if (quo)
		putd(quo);
	putchar(rem + '0');
}

Char *
putbufd(nn)
long nn;
{
	register int rem;
	long quo;

	quo = nn / 10;
	rem = nn % 10;
	if (quo)
		putbufd(quo);
	return(putbuf(rem+'0'));
}

Char	*toofar;

initbuf(buffer, bufsize)
register Char *buffer;
register bufsize;
{
	toofar = &buffer[bufsize];
	bufp = buffer;
}


Char *
putbuf(c)
Char c;
{
	if (bufp >= toofar) error(EBUFOFL);
	*bufp++ = c;
	return(bufp);
}

puts(as)
Char *as;
{
	register Char *sp;
	register c;

	sp = as;
	while (c = *sp++) putchar(c);
}

Char	line[70];
Char	*linp =	line;

putchar(ac)
{
	register Char *lp;
	register c;

	lp = linp;
	c = ac;
	if (junk) {
		if (c=='\n') {
			*lp++ = RJUNK;
			*lp++ = c;
			goto out;
		}
		if (col >= LINESIZE) {
			col = 0;
			*lp++ = '\\';
			*lp++ = '\n';
		}
		col++;
		if (c=='\t') {
			c = '>';
			goto esc;
		}
		if (c=='\b') {
			c = '<';
		esc:
			*lp++ = '-';
			*lp++ = '\b';
			*lp++ = c;
			goto out;
		}
		if (c<' ' && c!= '\n') {
			*lp++ = '\\';
			*lp++ = (c>>3)+'0';
			*lp++ = (c&07)+'0';
			col += 2;
			goto out;
		}
		if (c==127) {
			*lp++ = '\\';
			*lp++ = '1';
			*lp++ = '7';
			*lp++ = '7';
			col += 3;
			goto out;
		}
	}
	*lp++ = c;
out:
	linp = lp;
	if (c == '\n' || lp >= &line[sizeof line - 6]) sends();
	return (linp - line);
}

sends()
{
	write(output, line, linp-line);
	linp = line;
}

RETSIGTYPE
hupcatch()
{
	if ((io = creat("ed.hangup", 0666)) < 0)
		exit(1);
	addr1 = zero;
	addr2 = dol;
	squeeze(dol>zero);
	putfile();
	exit(0);
}

RETSIGTYPE
onintr(sig) int sig;
{
	/*ARGSUSED*/
	signal(SIGINT, onintr);
	linp = line;
	lastc = '\n';
	error(EINTR);
}

errfunc(errnum)
{

/*
 * use dashpp below rather than dashc, because dashpp is still
 * positive after reading the last Char of the last dash arg,
 * whereas dashc is 0 at that point.
 */
	if(dashpp) {
		write(1,useerr,sizeof useerr - 1);
		quit(2);
	}
	junk = '\0';
	suffix = 0;
	endout();
	puts(emesg[errnum]);
	lseek(0, 0L, 2);
	if(globp || sav0) lastc = '\n';
	closeinputs();
	ungetchar();
	while(getchar()!='\n');
	if (io > 0) {
		close(io);
		io = -1;
	}
	longjmp(env,1);
}

closeinputs()
{
	globflag = 0;
	globp = 0;
	dashpp = 0;
	dashc = 0;
	while(sav0>1) {
		close(stack0[sav0--]);
	}
	if(sav0==1) {
		close(0);
		dup(stack0[sav0]);
		close(stack0[sav0--]);
	}
}

endout()
{
	if(savout != 1) {
		close(savout);
		savout = output = 1;
		COLON = ':';
	}
}
