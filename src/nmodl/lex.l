%{
/* /local/src/master/nrn/src/nmodl/lex.l,v 4.2 1997/11/05 17:59:02 hines Exp */

#include <../../nmodlconf.h>

#undef output
#undef unput

#ifdef FLEX_SCANNER
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	{ \
	int c = Getc(); \
	result = (c==EOF) ? YY_NULL : (buf[0]=c, 1); \
	}
#else
#undef input
#endif


#include "modl.h"
#include "parse1.h"

int lexcontext = 0;
extern int Getc();
extern int unGetc();
extern int unput();

#define debug(arg1,arg2) /*printf("lex:arg1|%s|\n", arg2);*/
%}
D	[0-9]
E	[Ee][-+]?{D}+
%%
[a-zA-Z][a-zA-Z0-9_]*'+ { /*PRIME possibly high order*/
	char *cp, buf[100];
	int i;
	Symbol *base;

	/* count the primes  and create SCoP name*/
#if __TURBOC__ || SYSV || VMS || defined(MINGW)
#define index strchr
#endif
	cp = index(yytext, '\'');
	i = strlen(yytext) - (cp - yytext);
	*cp = '\0';
	if (i > 1) {
		Sprintf(buf, "D%d%s", i, yytext);
	}else{
		Sprintf(buf, "D%s", yytext);
	}
	/* determine if really a constant */
	if ((i = input()) == '0') {
		Strcat(buf, "0");
		yylval.qp = putintoken(buf, NAME);
		return NAME;
	}else{
		unput(i);
	}
			
	/*create symbol if needed and store in intoken list*/
	yylval.qp = putintoken(buf, PRIME);
	if (SYM(yylval.qp)->type != PRIME) {
		diag(yytext, " needed as derivative variable");
#if LINT
goto yyfussy;
#endif
	}
	/* the high order prime might not be explicitly declared at this
	time. But the base state may be an array. If it is, then make
	the PRIME an array of the same dimension. */
	if ((base = lookup(yytext))) {
		if (base->subtype & ARRAY) {
			SYM(yylval.qp)->subtype |= ARRAY;
			SYM(yylval.qp)->araydim = base->araydim;
		}
	}
	return PRIME;
}

WHILE	|
IF	|
ELSE	{ /* translate to lower case */
	char *cp;
	for (cp = yytext; *cp; cp++) {
		*cp += 'a' - 'A';
	}
	yylval.qp = putintoken(yytext, NAME);
	return SYM(yylval.qp)->type;
	}

[a-zA-Z][a-zA-Z0-9_]* { /*NAME*/
	Symbol *s;

	yylval.qp = putintoken(yytext, NAME);
	s = SYM(yylval.qp);
	if (s->type == PRIME)
		diag(yytext, " being used as derivative variable");
	return s->type;
}

{D}+	{ /*INTEGER*/
	yylval.qp = putintoken(yytext, INTEGER); /* Numbers are not looked for */
	return INTEGER;
}


{D}+"."{D}*({E})? |
{D}*"."{D}+({E})? |
{D}+{E} { /*REAL*/
	yylval.qp = putintoken(yytext, REAL); /* Numbers are not looked for */
	return REAL;
}

\"[^\"]*\" { /* STRING */
	yylval.qp = putintoken(yytext, STRING);
	return STRING;
		/* can't quote \" */
}

\>	{ yylval.qp = putintoken(yytext, 0); return GT;}
\>=	{ yylval.qp = putintoken(yytext, 0); return GE;}
\<	{ yylval.qp = putintoken(yytext, 0); return LT;}
\<=	{ yylval.qp = putintoken(yytext, 0); return LE;}
==	{ yylval.qp = putintoken(yytext, 0); return EQ;}
!=	{ yylval.qp = putintoken(yytext, 0); return NE;}
!	{ yylval.qp = putintoken(yytext, 0); return NOT;}
\&\&	{ yylval.qp = putintoken(yytext, 0); return AND;}
\|\|	{ yylval.qp = putintoken(yytext, 0); return OR;}

\<-\>	{ yylval.qp = putintoken(yytext, 0); return REACT1;}

\~\+	{ /* syntactic sugar for equation addition */
	yylval.qp = putintoken(yytext, 0);
	if (lexcontext == NONLINEAR) return NONLIN1;
	if (lexcontext == LINEAR) return LIN1;
	diag("equation addition can't occur in this type of block", (char *)0);
	}

\~	{ /* syntactic sugar for equations */
	yylval.qp = putintoken(yytext, 0);
	if (lexcontext == NONLINEAR) return NONLIN1;
	if (lexcontext == LINEAR) return LIN1;
	if (lexcontext == PARTIAL) return yytext[0];
	if (lexcontext == KINETIC) return REACTION;
	diag("equations can't occur in this type of block", (char *)0);
	}

[ \t]	{/*ignore spacing characters*/;}

\r\n	{/*ignore newline*/}
\r	{/*ignore newline*/}
\n	{/*ignore newline*/}

:.*	{/*ignore inline comments*/;}

\?.*	{/*ignore inline comments*/;}

\{	{ yylval.qp = putintoken("{", 0); return yytext[0]; }
\}	{ yylval.qp = putintoken("}", 0); return yytext[0]; }
.	{ yylval.qp = putintoken(yytext, 0); return yytext[0]; }

%%


#ifndef FLEX_SCANNER
int input()
{
	int c;
	c = Getc();
	if (c == EOF) {
		c = 0;
	}
	return c;
}
#endif


int unput(c) int c; {
	return unGetc(c);
}


int output(c) int c; {
#if LINT
	IGNORE(c);
#endif
	diag("internal error: ", "called output");
	return 0;
}

int yywrap() {
	return 1;
}

char * inputtopar()
{				/* returns string up to and not including
				 * next closing parenthesis */
	/* main usage is to read the units string */
	char            *cp;
	int             c;

	cp = buf;
	while ((c = input()) != ')' && c != '\n' && c != 0) {
		*cp++ = c;
	}
	if (c != ')')
		diag("Syntax error:", " no closing ')'");
	*cp = '\0';
	unput(c);
	return stralloc(buf, (char *) 0);
}
