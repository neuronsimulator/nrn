%{
/* symbolically differentiate an expression with respect to a state
   and also transform an expression linear in the state into the form
   a*state + b

  The only thing that makes this less than elegant is dealing with 0.0
*/

#include <../../nmodlconf.h>
#include <string.h>
#include "modl.h"
#include "difeqdef.h"

/* every yy gets changed to diffeq_yy before compiling */

static int yylex(), yyparse();
static void yyerror();
static int d_invalid, eq_invalid;
static char lbuf[4][1000];
static Item* qexpr; /* yylex finds tokens here;*/
static Symbol* state;
static List* result;

#define b1 sprintf(lbuf[0],
#define b2 sprintf(lbuf[1],
#define b3 sprintf(lbuf[2],
#define b4 sprintf(lbuf[3],

static void replace(), initbuf(), free4();
static int zero();
static char *expr(), *de(), *a(), *b();
static List* list4();
%}

%union {
	char* cp;
	List* list; /*  expression, d(expression)/dstate, a*state, b */
}

%token <list> ATOM
%type <list> e arglist arg

%left '+' '-'
%left '*' '/'
%left UNARYMINUS

%%
top: {d_invalid = 0; eq_invalid = 0; } e {result = $2;}
	;
e:	  ATOM {$$ = $1;}
	| '(' e ')' { $$ = $2; initbuf();
		b1 "( %s )", expr($$));
		if (!zero(de($$))) {b2 "( %s )", de($$));}
		if (!zero(a($$))) {b3 "( %s )", a($$));}
		if (!zero(b($$))) {b4 "( %s )", b($$));}
		replace($$);
	    }
		
	| ATOM '(' arglist ')' { $$ = $3; initbuf();
		b1 "%s ( %s )", expr($1), expr($$));
		{b4 "%s ( %s )", expr($1), expr($$));}
		free4($1); replace($$);
	    }
	| '-' e %prec UNARYMINUS { $$ = $2; initbuf();
		b1 "- %s", expr($$));
		if (!zero(de($$))) {b2 "- %s", de($$));}
		if (!zero(a($$))) {b3 "- %s", a($$));}
		if (!zero(b($$))) {b4 "- %s", b($$));}
		replace($$);
	    }
	| e '+' e { $$ = $1; initbuf();
		b1 "%s + %s", expr($1), expr($3));
		if (!zero(de($1)) && !zero(de($3))) {b2 "%s + %s", de($1), de($3));
		}else if (!zero(de($1))) {b2 "%s", de($1));
		}else if (!zero(de($3))) {b2 "%s", de($3));}
		if (!zero(a($1)) && !zero(a($3))) {b3 "%s + %s", a($1), a($3));
		}else if (!zero(a($1))) {b3 "%s", a($1));
		}else if (!zero(a($3))) {b3 "%s", a($3));}
		if (!zero(b($1)) && !zero(b($3))) {b4 "%s + %s", b($1), b($3));
		}else if (!zero(b($1))) {b4 "%s", b($1));
		}else if (!zero(b($3))) {b4 "%s", b($3));}
		free4($3); replace($$);
	    }
	| e '-' e { $$ = $1; initbuf();
		b1 "%s - %s", expr($1), expr($3));

		if (!zero(de($1)) && !zero(de($3))) {b2 "%s - %s", de($1), de($3));
		}else if (!zero(de($1))) {b2 "%s", de($1));
		}else if (!zero(de($3))) {b2 "( - %s )", de($3));}

		if (!zero(a($1)) && !zero(a($3))) {b3 "%s - %s", a($1), a($3));
		}else if (!zero(a($1))) {b3 "%s", a($1));
		}else if (!zero(a($3))) {b3 "( - %s)", a($3));}

		if (!zero(b($1)) && !zero(b($3))) {b4 "%s - %s", b($1), b($3));
		}else if (!zero(b($1))) {b4 "%s", b($1));
		}else if (!zero(b($3))) {b4 "( - %s )", b($3));}
		free4($3); replace($$);
	    }
	| e '*' e { $$ = $1; initbuf();
		b1 "%s * %s", expr($1), expr($3));
		if (!zero(de($1)) && !zero(de($3))) {
b2 "((%s)*(%s) + (%s)*(%s))", de($1), expr($3), expr($1), de($3));
		}else if (!zero(de($1))) {b2 "(%s)*(%s)", de($1), expr($3));
		}else if (!zero(de($3))) {b2 "(%s)*(%s)", expr($1), de($3));}

		if (!zero(a($1)) && !zero(a($3))) {eq_invalid = 1;
		}else if (!zero(a($1))) {
			if (!zero(b($3))) {b3 "(%s)*(%s)", a($1),b($3));}
		}else if (!zero(a($3))) {
			if (!zero(b($1))) {b3 "(%s)*(%s)", b($1),a($3));}}

		if (!zero(b($1)) && !zero(b($3))) {
			b4 "(%s)*(%s)", b($1), b($3));}
		free4($3); replace($$);
	    }
	| e '/' e { $$ = $1; initbuf();
		b1 "%s / %s", expr($1), expr($3));

		if (!zero(de($3))) { d_invalid = 1;
		}else if (!zero(de($1))) { b2 "( %s ) / %s", de($1), expr($3));}

		if (!zero(a($3))) { eq_invalid = 1;
		}else if (!zero(a($1))) { b3 "( %s ) / %s", a($1), expr($3));}

		if (!zero(b($1))) { b4 "( %s ) / %s", b($1), expr($3));}
		free4($3); replace($$);
	    }
	;

arglist: /*nothing*/ {
		$$ = list4("", "0.0", "0.0", "");
		}
	| arg { $$ = $1; }
	| arglist arg {
		$$ = $2;
		b1 "%s %s", expr($1), expr($2));
		b4 "%s %s", expr($1), expr($2));
		free4($1); replace($$);
		}
	| arglist ',' arg {
		$$ = $3;
		b1 "%s , %s", expr($1), expr($3));
		b4 "%s , %s", expr($1), expr($3));
		free4($1); replace($$);
		}
	;	
arg:	e { $$ = $1; initbuf();
		b1 "%s", expr($1));
		if (!zero(de($$))) { d_invalid = 1; }
		if (!zero(de($$))) { eq_invalid = 1; }
		{b4 "%s", expr($$));}
	    }
	;
%%

static int zero(cp) char* cp; {
	return (strcmp(cp, "0.0") == 0) ? 1 : 0;
}

static char* expr(lst) List* lst; {
	Item* q = lst->next;
	return STR(q);
}
static char* de(lst) List* lst; {
	Item* q = lst->next->next;
	return STR(q);
}
static char* a(lst) List* lst; {
	Item* q = lst->next->next->next;
	return STR(q);
}
static char* b(lst) List* lst; {
	Item* q = lst->next->next->next->next;
	return STR(q);
}

static void replace(lst) List* lst; {
	int i;
	Item* q = lst->next;
	for (i=0; i < 4; ++i) {
		free(STR(q));
		replacstr(q, lbuf[i]);
		q = q->next;
	}
#if 0
fprintf(stderr,
"replace expr|%s| de|%s| a|%s| b|%s|\n",
lbuf[0], lbuf[1], lbuf[2], lbuf[3]);
#endif
}

static void initbuf() {
	int i;
	for (i=0; i < 4; ++i) {
		strcpy(lbuf[i], "0.0");
	}
}

static List* list4(s1, s2, s3, s4) char *s1, *s2, *s3, *s4; {
	List* lst = newlist();
	lappendstr(lst, s1);
	lappendstr(lst, s2);
	lappendstr(lst, s3);
	lappendstr(lst, s4);
	return lst;
}

static void free4(lst) List* lst; {
	Item* q;
	List* ls = lst;
	ITERATE(q, lst) {
		free(STR(q));
	}
	freelist(&ls);
}

static void yyerror(s) char* s; { assert(0); }

static void fullname(buf) char* buf; {
	/* handle case of name [...] with qexpr pointing to final item */
	Item* q = qexpr;
	strcpy(buf, SYM(q)->name);
	if (q->next->itemtype == SYMBOL && strcmp(SYM(q->next)->name, "[") == 0) {
		q = q->next;
		strcat(buf, "[");
		for (;;) {
			q = q->next;
			if (q->itemtype == SYMBOL) {
				strcat(buf, SYM(q)->name);
				if (strcmp(SYM(q)->name, "]") == 0) {
					break;
				}
			}else if (q->itemtype == ITEM || q->itemtype == LIST) {
				assert(0);
			}else{ /* had better be a STRING */
				strcat(buf, STR(q));
			}
		}
	}
	qexpr = q;
}

static int yylex() {
	Symbol* s;
	char buf[256];
	int rval = 0;
	if (qexpr->itemtype == 0) {
		return 0;
	}
	switch (qexpr->itemtype) {
	case SYMBOL:
		s = SYM(qexpr);
		if (s == state) {
			fullname(buf);
			yylval.list = list4(buf, "1.0", "1.0", "0.0");
			rval = ATOM;
		}else{
			switch (s->name[0]) {
			case '+':
			case '-':
			case '*':
			case '/':
			case '(':
			case ')':
			case ',':
				rval = s->name[0];
				break;
			default:
				fullname(buf);
				yylval.list = list4(buf, "0.0", "0.0", buf);
				rval = ATOM;
				break;
			}
		}
		break;
	default: /* had better be a STRING */
		if (strcmp(STR(qexpr), "") == 0) {
			qexpr = qexpr->next;
			return yylex();
		}
		yylval.list = list4(STR(qexpr), "0.0", "0.0", STR(qexpr));
		rval = ATOM;
		break;
	}

#if 0
if (rval == ATOM) {
fprintf(stderr,
"expr|%s| de|%s| a|%s| b|%s|\n",
expr(yylval.list), de(yylval.list),
a(yylval.list), b(yylval.list));
}else{
fprintf(stderr, "|%s|\n", SYM(qexpr)->name);
}
#endif
	qexpr = qexpr->next;
	return rval;
}

/*----------- interface to outside world -------------*/
void cvode_parse(s, e) Symbol* s; List* e; {
	state = s;
	qexpr = e->next;
	yyparse();
#if 0
fprintf(stderr,
"cvode_parse d_invalid=%d eq_invalid=%d\nexpr|%s|\nde|%s|\na|%s|\nb|%s|\n",
d_invalid, eq_invalid, expr(result), de(result), a(result), b(result));
#endif
	b4 "- ( %s ) / ( %s )", b(result), a(result));
	replacstr(result->prev, lbuf[3]);
}
char* cvode_deriv() {
	if (result && !d_invalid) {
		return de(result);
	}else{
		return (char*)0;
	}
}
char* cvode_eqnrhs() {
	if (result && !eq_invalid) {
		return b(result);
	}else{
		return (char*)0;
	}
}
