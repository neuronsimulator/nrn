#ifndef _MODL_H_
#define _MODL_H_

/* subtypes */
#define KEYWORD 01
#define PARM 02
#define INDEP 04
#define DEP 010 /* also in usage field */
#define STAT 020
#define ARRAY 040
#define FUNCT 0100 /* also in usage field */
#define PROCED 0200
#define NEGATIVE 0400
#define SEMI 01 /* ";" */

/* @todo: we define these are now as tokens, probably we dont need it (?) */
//#define BEGINBLK 02		/* "{" */
//#define ENDBLK	04		/* "}" */

#define DERF 01000
#define KINF 02000
#define NLINF 04000
#define DISCF 010000
#define STEP1 020000
#define PARF 040000
#define EXTDEF 0100000
#define LINF 0200000
#define UNITDEF 0400000L
#define EXTDEF2 01000000L    /* functions that can take array or function name arguments */
#define nmodlCONST 02000000L /* constants that do not appear in .var file */
#define EXTDEF3 04000000L    /* get two extra reset arguments at beginning */
#define INTGER 010000000L    /* must be cast to double in expr */
#define EXTDEF4 020000000L   /* get extra NrnThread* arg at beginning */
#define EXTDEF5 040000000L   /* not threadsafe from the extdef list */
#define EXPLICIT_DECL 01     /* usage field, variable occurs in input file */

#endif
