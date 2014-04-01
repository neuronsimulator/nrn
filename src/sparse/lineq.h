# define	rowst	spar_rowst
# define	colst	spar_colst
# define	neqn	spar_neqn
# define	eqord	spar_eqord
# define	varord	spar_varord
# define	rhs	spar_rhs
# define	matsol	spar_matsol
# define	getelm	spar_getelm
# define	bksub	spar_bksub
# define	prmat	spar_prmat
# define	subrow	spar_subrow
# define	remelm	spar_remelm

#include <stdio.h>

struct elm
{
	unsigned row;		/* Row location */
	unsigned col;		/* Column location */
	double value;		/* The value */
	struct elm *r_up;	/* Link to element in same column */
	struct elm *r_down;	/* 	not ordered list */
	struct elm *c_left;	/* Link to left element in same row */
	struct elm *c_right;	/*	this list is ordered (see getelm) */
};

#define ELM0	(struct elm *)0
extern struct elm **rowst;		/* link to first element in row */
extern struct elm **colst;		/* link to a column element */
extern unsigned neqn;			/* number of equations */
extern unsigned *eqord;			/* row order for pivots */
extern unsigned *varord;			/* column order for pivots */
extern double *rhs;			/* initially- right hand side
					finally - answer */
extern int matsol(void);
extern struct elm *getelm(struct elm*, unsigned, unsigned);
extern void remelm(struct elm*);
extern void subrow(struct elm*, struct elm*);
extern void bksub(void);
extern void prmat(void);
