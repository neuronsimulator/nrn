#include <../../nrnconf.h>
/* * Last edited: Aug  5 17:37 1994 (ngoddard) */
/* vector_sparse for solving vectorized kinetic blocks */
/******************************************************************************
 *
 * File: sparse.c
 *
 * Copyright (c) 1989, 1990
 *   Duke University
 *
 ******************************************************************************/

#ifndef LINT
static char RCSid[] = "vsparse.c,v 1.5 1999/01/04 12:46:53 hines Exp";
#endif

#include "errcodes.h"
#include <stdio.h>
#include <math.h>
#include <assert.h>
#include <stdlib.h>
#if defined(HAVE_MALLOC_H)
#include <malloc.h>
#endif

extern void _modl_set_dt(double);

#define SW_DATA 0
#define SOLVE 0
#define _INSTANCE_LOOP for (_ix = _ix1; _ix < _ix2; ++_ix)
#define BIG_INSTANCE_LOOP for (_ix = 0; _ix < count; ++_ix)


# define	rowst	vec_spar_rowst
# define	diag	vec_spar_diag
# define	neqn	vec_spar_neqn
# define	varord	vec_spar_varord
# define	matsol	vec_spar_matsol
# define	getelm	vec_spar_getelm
# define	bksub	vec_spar_bksub
# define	prmat	vec_spar_prmat
# define	subrow	vec_spar_subrow
# define	remelm	vec_spar_remelm

/*
   NOTE THIS!  Element rows and columns go from 1 to neqn+1
   But the jacobian rows and cols go from 0 to neqn !!
*/

typedef struct Elm {
	unsigned row;		/* Row location */
	unsigned col;		/* Column location */
	double *value;		/* The value */
	struct Elm *r_up;	/* Link to element in same column */
	struct Elm *r_down;	/* 	in solution order */
	struct Elm *c_left;	/* Link to left element in same row */
	struct Elm *c_right;	/*	in solution order (see getelm) */
} Elm;
#define ELM0	(Elm *)0

typedef struct Item {
	Elm 		*elm;
	unsigned	norder; /* order of a row */
	struct Item	*next;
	struct Item	*prev;
} Item;
#define ITEM0	(Item *)0

typedef Item List;	/* list of mixed items */

typedef struct SparseObj { /* all the state information */
  Elm**	rowst;
  Elm**	diag;
  unsigned neqn;
  unsigned* varord;
  int (*oldfun)();
  int phase;
  /* don't really need the rest */
  Item** roworder;
  List* orderlist;
  int do_flag;
  /* new stuff for vectorization */
  int ninst;			/* number of instances in this sparse object */
  double ** jacob;		/* jacobian elements */
  double ** rhs;		/* right hand side */
  double ** space;		/* work space in caller */
  double * err;			/* error for each instance */
  int * converged;		/* 0 until converged */
  double * r_subrow;		/* work space in subrow() */
} SparseObj;


static Elm **rowst;/* link to first element in row (solution order) 1..n+1*/
static Elm **diag;/* link to pivot element in row (solution order) 1..n+1*/
static unsigned neqn;			/* number of equations */
static unsigned *varord;	/* row and column order for pivots */
static double **rhs;  /* initially- right hand side finally - answer 0..n*/

static int phase; /* 0-solution phase; 1-count phase; 2-build list phase */
static double **jacob; /* jacobian values for all instances
			   indexed jacob[row*neqn+col][instance] */
static double *r_subrow; /* workspace for subrow routine allocated in crea_spar_obj*/

static Item **roworder; /* roworder[i] is pointer to order item for row i.
			Does not have to be in orderlist 1..n+1*/
static List *orderlist; /* list of rows sorted by norder
			that haven't been used */

static int do_flag;
static int ninst;

/* note: solution order refers to the following
	diag[varord[row]]->row = row = diag[varord[row]]->col
	rowst[varord[row]]->row = row
	varord[el->row] < varord[el->c_right->row]
	varord[el->col] < varord[el->r_down->col]
*/
	
extern void *emalloc();		/* in sparse.c */
static int vector_matsol();
static Elm *getelm();


static void sparseobj2local();
static void local2sparseobj();
static double d;

double * _vector_getelm( row, col)
  int  row, col;
{
  row++; col++;			/* note: start at 1, not 0 */
	if (!phase) {
	  fprintf(stderr, "vector_getelm called in phase 0.. abort\n");
	  abort();
	}
	return &(getelm( (unsigned)row, (unsigned)col, ELM0)->value[0]);
}

SparseObj * create_sparseobj(ninst,neqn,linflag)
     int ninst,neqn,linflag;
{
  int i, j, k;
  SparseObj * so = (SparseObj *) malloc (sizeof(SparseObj));
  double ** t;

/* from sparse.c */
	so->rowst = 0;
	so->diag = 0;
	so->neqn = 0;
	so->varord = 0;
	so->oldfun = 0;
	so->phase = 0;
	so->roworder = 0;
	so->orderlist = 0;
	so->do_flag = 0;
/* end */

  so->ninst = ninst;		/* to enable check on growth */

  /* allocate the jacobians */
  k = (neqn)*(neqn);
  so->jacob = t = (double **) malloc(k*sizeof(double*));
  t[0] = (double *) calloc(ninst*k, sizeof(double));
  for (i = 0; i < k; i++)
    /*  t[i] = (double *) calloc(ninst, sizeof(double));*/
    t[i] = t[0] + ninst*i;

  /*********************************/
  /* allocate the right hand sides */
  k = neqn;
  so->rhs = t = (double **) malloc((neqn)*sizeof(double*));
  t[0] = (double *) calloc( k*ninst, sizeof(double));
  for (i = 0; i < k; i++)
    /* t[i] = (double *) calloc(ninst, sizeof(double)); */
    t[i] = t[0] + ninst * i ;

  /*********************************/
  /* allocate the workspace */
  k = neqn;
  so->space = t = (double **) malloc( (neqn)*sizeof(double*));
 for (i = 0; i < k; i++)
    t[i] = (double *) calloc(ninst, sizeof(double));

  if (!linflag)
  /**************************************************/
  /* allocate the error vector and converged vector */
    {  
      so->err = (double *) calloc(ninst,sizeof(double));
      so->converged = (int *) calloc(ninst,sizeof(int));
    }

  /* allocate the temporary space needed in subrow() */
  so->r_subrow = (double *) calloc( ninst, sizeof(double) );

  /* initialize oldfun */
  so->oldfun = NULL;

  return so;
}

static void subrow();
static void bksub();
static void free_elm();
static void create_coef_list();
static void init_coef_list();
static void increase_order();
static void reduce_order();
static void spar_minorder();
static void get_next_pivot();
static void freelist();
static void check_assert();
static void re_link();
static void delete();

/*-----------------------------------------------------------------------------
 * THIS NEEDS UPDATING
 *  sparse()
 *
 *  Abstract: 
 *  This is an experimental numerical method for SCoP-3 which integrates kinetic
 *  rate equations.  It is intended to be used only by models generated by MODL,
 *  and its identity is meant to be concealed from the user.
 *
 *
 *  Calling sequence:
 *	sparse(n, s, d, t, dt, fun, prhs, linflag)
 *
 *  Arguments:
 * 	n		number of state variables
 * 	s		array of pointers to the state variables
 * 	d		array of pointers to the derivatives of states
 * 	t		pointer to the independent variable
 * 	dt		the time step
 * 	fun		pointer to the function corresponding to the
 *			kinetic block equations
 * 	prhs		pointer to right hand side vector (answer on return)
 *			does not have to be allocated by caller.
 * 	linflag		solve as linear equations
 *			when nonlinear, all states are forced >= 0
 * 
 *		
 *  Returns:	nothing
 *
 *  Functions called: IGNORE(), printf(), create_coef_list(), fabs()
 *
 *  Files accessed:  none
 *
*/

int _vector_sparse(base, bound, count, jacobp, spacep, sparseobj, neqns, state,
	       deriv, data, t, delta_t, vfun, fun, rhsp, linflag)
     int base;			/* first instances */
     int bound;			/* last instance + 1 */
     int count;			/* total number of instances */
     double *** jacobp;		/* pointer to list of jacobians */
				/* accessed as *jacobp[element][instance] */
     double *** spacep;		/* pointer to workpace */
				/* accessed as *spacep[variable][instance] */
     void ** sparseobj;		/* pointer to sparse obj for this mechanism */
     int neqns;			/* number of state variables */
     int state[];		/* index of state vars in data */
     int deriv[];		/* index of derivs in data */
     double * data[];		/* list of intance data, inst is first idx! */
				/* accessed as data[instance][parameter] */
     double * t;		/* pointer to time */
     double delta_t;		/* delta t */
     int (*vfun)();		/* function to compute jacobian entries */
     int (*fun)();		/* function which calls _vector_getelm */
     double *** rhsp;		/* pointer to list of right hand sides */
				/* accessed as *rhsp[variable][instance] */
     int linflag;		/* if != 0 then linear */
{
  int i, j, k, ierr;
  double * err=0;
  SparseObj* so;
  int nconverged = 0;
  int * converged=0;
  int zero = 0;
  static SparseObj* old_sparseobj = NULL;
   
  so = (SparseObj *) *sparseobj;	/* translater uses void pointer */
  ninst = count;
  if (!so)
    {				/* first time in this mechanism */
      so = create_sparseobj(ninst,neqns,linflag);
      *sparseobj = (void *) so;		/* so that it is set on next call */
      *jacobp = so->jacob;
      jacob = so->jacob;
      *rhsp = so->rhs;
      rhs = so->rhs;
      *spacep = so->space;
    }
  else				/* check that haven't increased size */
    if (so->ninst < bound)	/* only care about ones being used here */
      {
	fprintf(stderr, "**Fatal error: number of instances increased in vector_sparse\n");
	abort();
      }

  if (so != old_sparseobj)
    {
      sparseobj2local(so);
      old_sparseobj = so;
    }
  if (so->oldfun != fun) {
    so->oldfun = fun;
    create_coef_list(base,bound, count,neqns, fun); /* calls fun twice */
    local2sparseobj(so);
  }

  if (!linflag)
    {
      err = so->err;
      converged = so->converged;
    }
  
  
/*=====================================================*/
/*==================================================== */
/*          performance evaluation stuff               */
/*==================================================== */
#if 1                                                /**/
#define FLOWMARK(arg)                                /**/
#endif                                               /**/
                                                     /**/
#if SW_DATA                                          /**/
#define data1(i,j) data[j][i]                         /**/
#else                                                /**/
#define data1(i,j) data[i][j]                         /**/
#endif                                               /**/
/*==================================================== */
/*==================================================== */
  
  for (i=0; i<neqns; i++)
    { /*save old state in deriv space for now */
      FLOWMARK("loop1");
      for (k = base; k < bound; k++)
	data1(k,deriv[i]) = data1(k,state[i]);
      FLOWMARK(&zero);
    }
  
  if (!linflag)
    {
      FLOWMARK("loop2");
      for (k = base; k < bound; k++)
	converged[k] = 0;
      FLOWMARK(&zero);
    }
  
  for (nconverged = j = 0; nconverged < bound-base; j++)
    {
      init_coef_list(base,bound);		/* ?? */
      (*vfun)(base,bound);
      if((ierr = vector_matsol(so,base,bound,converged)) != 0)
	{
	  return ierr;
	}
      
      if (!linflag)
	{
	  FLOWMARK("loop3");
	  for (k = base; k < bound; k++)
	    err[k] = 0;
	  FLOWMARK(&zero);
	}
      
      for (i=0; i<neqns; i++)
	{
	  FLOWMARK("loop4a");
	  if (linflag)
	    {
	      for (k = base; k < bound; k++)
		data1(k,state[i]) += rhs[i][k];
	    }
	  else			/* !linflag */
	    {
	      for (k = base; k < bound; k++)
		{
		  if (!(converged[k]))
		    data1(k,state[i]) += rhs[i][k];
		  if (rhs[i][k] < 0)
		    err[k] -= rhs[i][k];
		  else
		    err[k] += rhs[i][k];
    /* stability of nonlinear kinetic schemes sometimes requires this */
		  if (data1(k,state[i]) < 0.)
		    data1(k,state[i]) = 0.;
		}
	    }
	  FLOWMARK(&zero);
	  
	}
      
      if (linflag)
	break;
      
      FLOWMARK("loop5");
      for (k = base; k < bound; k++)
	if (!(converged[k]))
	  if (err[k] < CONVERGE)
	    {
	      converged[k] = 1;
	      nconverged++;
	    }
      FLOWMARK(&zero);
      
      if (j > MAXSTEPS)
	{
	  return EXCEED_ITERS;
	}
    }
  init_coef_list(base,bound);
  (*vfun)(base,bound);
  for (i=0; i<neqns; i++)
    { /*restore Dstate at t+dt*/
      FLOWMARK("loop6");
      for (k = base; k < bound; k++)
	data1(k,deriv[i]) = (data1(k,state[i]) - data1(k,deriv[i]))/delta_t;
      FLOWMARK(&zero);
    }
  return SUCCESS;
}


#if LINT
#define IGNORE(arg)	{if (arg);}
#else
#define IGNORE(arg)	arg
#endif

#if __TURBOC__ || VMS
#define Free(arg)	free((void *)arg)
#else
#define Free(arg)	free((char *)arg)
#endif


static int vector_matsol(so,_ix1,_ix2,converged)
      SparseObj *so; int _ix1,_ix2; int *converged;
{
        int _ix;
	register Elm *pivot, *el;
	unsigned i;

	/* Upper triangularization */
	for (i=1 ; i <= neqn ; i++)
	{
		pivot = diag[i];
             _INSTANCE_LOOP{
		if (fabs( pivot->value[_ix] ) <= ROUNDOFF) return SINGULAR;
		}

		/* Eliminate all elements in pivot column */
		for (el = pivot->r_down ; el ; el = el->r_down)
			subrow(_ix1, _ix2, pivot, el);
	}
	bksub(_ix1,_ix2);
	return(SUCCESS);
}


static void subrow(_ix1, _ix2, pivot, rowsub)
         int _ix1, _ix2;
          Elm *pivot, *rowsub;
{
  int _ix;
  register Elm *el;

  _INSTANCE_LOOP {
    r_subrow[_ix] = (rowsub->value)[_ix] / (pivot->value)[_ix];
    rhs[rowsub->row-1][_ix] -= rhs[pivot->row-1][_ix] * r_subrow[_ix];
  }
  for (el = pivot->c_right ; el ; el = el->c_right) {
    for (rowsub = rowsub->c_right; rowsub->col != el->col;
	 rowsub = rowsub->c_right) {
      ;
    }
    _INSTANCE_LOOP
      rowsub->value[_ix] -= el->value[_ix] * r_subrow[_ix];
  }
}

static void bksub(_ix1, _ix2)
         int _ix1, _ix2;
{
  int _ix;
  unsigned i;
  Elm *el;

  for (i = neqn ; i >= 1 ; i--)
    {
      for (el = diag[i]->c_right ; el ; el = el->c_right)
	{
	  _INSTANCE_LOOP
	    rhs[el->row-1][_ix] -= (el->value)[_ix]* rhs[el->col-1][_ix];
	}
      _INSTANCE_LOOP 
	rhs[diag[i]->row-1][_ix] /= (diag[i]->value)[_ix];
    }
}

static void prmat()
{
	unsigned i, j;
	Elm *el;

	IGNORE(printf("\n        "));
	for (i=10 ; i <= neqn ; i += 10)
		IGNORE(printf("         %1d", (i%100)/10));
	IGNORE(printf("\n        "));
	for (i=1 ; i <= neqn; i++)
		IGNORE(printf("%1d", i%10));
	IGNORE(printf("\n\n"));
	for (i=1 ; i <= neqn ; i++)
	{
		IGNORE(printf("%3d %3d ", diag[i]->row, i));
		j = 0;
		for (el = rowst[i] ;el ; el = el->c_right)
		{
			for ( j++ ; j < varord[el->col] ; j++)
				IGNORE(printf(" "));
			IGNORE(printf("*"));
		}
		IGNORE(printf("\n"));
	}
	IGNORE(fflush(stdin));
}

static void initeqn(_ix1, _ix2, count, maxeqn) /* reallocate space for matrix */
     int _ix1, _ix2, count ;
     unsigned maxeqn;
{
  int _ix = _ix1;
  register unsigned i;
  double *val;

  if (maxeqn == neqn) return;
  free_elm();
  if (rowst)
    Free(rowst);
  if (diag)
    Free(diag);
  if (varord)
    Free(varord);
  rowst = diag = (Elm **)0;
  varord = (unsigned *)0;
  rowst = (Elm **)emalloc((maxeqn + 1)*sizeof(Elm *));
  diag = (Elm **)emalloc((maxeqn + 1)*sizeof(Elm *));
  varord = (unsigned *)emalloc((maxeqn + 1)*sizeof(unsigned));
  for (i=1 ; i<= maxeqn ; i++)
    {
      varord[i] = i;
      diag[i] = (Elm *)emalloc(sizeof(Elm));
      diag[i]->value = (jacob[(i-1)*(maxeqn+1)]); /* = row*maxeqn+col */
      rowst[i] = diag[i];
      diag[i]->row = i;
      diag[i]->col = i;
      diag[i]->r_down = diag[i]->r_up = ELM0;
      diag[i]->c_right = diag[i]->c_left = ELM0;
      BIG_INSTANCE_LOOP{	/* ??? NHG */
	diag[i]->value[_ix] = 0.;
	rhs[i-1][_ix] = 0.;
      }
    }
  neqn = maxeqn;
}


static void free_elm() {
	unsigned i;
	Elm *el;
	
	/* free all elements */
	for (i=1; i <= neqn; i++)
	{
		for (el = rowst[i]; el; el=el->c_right)
			Free(el);
		rowst[i] = ELM0;
		diag[i] = ELM0;
	}
}


/* see check_assert in minorder for info about how this matrix is supposed
to look.  In new is nonzero and an element would otherwise be created, new
is used instead. This is because linking an element is highly nontrivial
The biggest difference is that elements are no longer removed and this
saves much time allocating and freeing during the solve phase
*/

static Elm * getelm( row, col, new)
   register Elm *new;
   unsigned row, col;		/* 1..n+1 */
   /* return pointer to row col element maintaining order in rows */
{
        int _ix = 0;
	register Elm *el, *elnext;
	unsigned vrow, vcol;
	
	vrow = varord[row];
	vcol = varord[col];
	
	if (vrow == vcol) {
		return diag[vrow]; /* a common case */
	}
	if (vrow > vcol) { /* in the lower triangle */
		/* search downward from diag[vcol] */
		for (el=diag[vcol]; ; el = elnext) {
			elnext = el->r_down;
			if (!elnext) {
				break;
			}else if (elnext->row == row) { /* found it */
				return elnext;
			}else if (varord[elnext->row] > vrow) {
				break;
			}
		}
		/* insert below el */
		if (!new) {
			new = (Elm *)emalloc(sizeof(Elm));
                        new->value = jacob[(row-1)*neqn + col-1];
			increase_order(row);
		}
		new->r_down = el->r_down;
		el->r_down = new;
		new->r_up = el;
		if (new->r_down) {
			new->r_down->r_up = new;
		}
		/* search leftward from diag[vrow] */
		for (el=diag[vrow]; ; el = elnext) {
			elnext = el->c_left;
			if (!elnext) {
				break;
			} else if (varord[elnext->col] < vcol) {
				break;
			}
		}
		/* insert to left of el */
		new->c_left = el->c_left;
		el->c_left = new;
		new->c_right = el;
		if (new->c_left) {
			new->c_left->c_right = new;
		}else{
			rowst[vrow] = new;
		}
	} else { /* in the upper triangle */
		/* search upward from diag[vcol] */
		for (el=diag[vcol]; ; el = elnext) {
			elnext = el->r_up;
			if (!elnext) {
				break;
			}else if (elnext->row == row) { /* found it */
				return elnext;
			}else if (varord[elnext->row] < vrow) {
				break;
			}
		}
		/* insert above el */
		if (!new) {
			new = (Elm *)emalloc(sizeof(Elm));
                        new->value = jacob[(row-1)*neqn + col-1];
			increase_order(row);
		}
		new->r_up = el->r_up;
		el->r_up = new;
		new->r_down = el;
		if (new->r_up) {
			new->r_up->r_down = new;
		}
		/* search right from diag[vrow] */
		for (el=diag[vrow]; ; el = elnext) {
			elnext = el->c_right;
			if (!elnext) {
				break;
			}else if (varord[elnext->col] > vcol) {
				break;
			}
		}
		/* insert to right of el */
		new->c_right = el->c_right;
		el->c_right = new;
		new->c_left = el;
		if (new->c_right) {
			new->c_right->c_left = new;
		}
	}
	new->row = row;
	new->col = col;
	return new;
}

static void create_coef_list(_ix1, _ix2, count, n, fun)
	int _ix1, _ix2, n, count;
	int (*fun)();
{
	initeqn(_ix1, _ix2, count, (unsigned)n);
	phase = 1;
	(*fun)(_ix1, _ix2);
	spar_minorder();
	phase = 2;
	(*fun)(_ix1, _ix2);
	phase = 0;
}


static void init_coef_list(_ix1, _ix2) 
	int _ix1, _ix2;
{
        int _ix;
        unsigned i;
	Elm *el;
	
	for (i=1; i<=neqn; i++) {
		for (el = rowst[i]; el; el = el->c_right) {
		_INSTANCE_LOOP	(el->value)[_ix] = 0.;
		}
	}
}


static Item *newitem();
static List *newlist();

static void insert();

static void init_minorder() {
	/* matrix has been set up. Construct the orderlist and orderfind
	   vector.
	*/
	unsigned i, j;
	Elm *el;
	
	do_flag = 1;
	if (roworder) Free(roworder);
	roworder = (Item **)emalloc((neqn+1)*sizeof(Item *));
	if (orderlist) freelist(orderlist);
	orderlist = newlist();
	for (i=1; i<=neqn; i++) {
		roworder[i] = newitem();
	}
	for (i=1; i<=neqn; i++) {
		for (j=0, el = rowst[i]; el; el = el->c_right) {
			j++;
		}
		roworder[diag[i]->row]->elm = diag[i];
		roworder[diag[i]->row]->norder = j;
		insert(roworder[diag[i]->row]);
	}
}

static void increase_order(row) unsigned row; {
	/* order of row increases by 1. Maintain the orderlist. */
	Item *order;

	if(!do_flag) return;
	order = roworder[row];
	delete(order);
	order->norder++;
	insert(order);
}

static void reduce_order(row) unsigned row; {
	/* order of row decreases by 1. Maintain the orderlist. */
	Item *order;

	if(!do_flag) return;
	order = roworder[row];
	delete(order);
	order->norder--;
	insert(order);
}

static void spar_minorder() { /* Minimum ordering algorithm to determine the order
			that the matrix should be solved. Also make sure
			all needed elements are present.
			This does not mess up the matrix
		*/
	unsigned i;

	check_assert();
	init_minorder();
	for (i=1; i<=neqn; i++) {
		get_next_pivot(i);
	}
	do_flag = 0;
	check_assert();
}

static void get_next_pivot(i) unsigned i; {
	/* get varord[i], etc. from the head of the orderlist. */
	Item *order;
	Elm *pivot, *el;
	unsigned j;

	order = orderlist->next;
	assert(order != orderlist);
	
	if ((j=varord[order->elm->row]) != i) {
		/* push order lists down by 1 and put new diag in empty slot */
		assert(j > i);
		el = rowst[j];
		for (; j > i; j--) {
			diag[j] = diag[j-1];
			rowst[j] = rowst[j-1];
			varord[diag[j]->row] = j;
		}
		diag[i] = order->elm;
		rowst[i] = el;
		varord[diag[i]->row] = i;
		/* at this point row links are out of order for diag[i]->col
		   and col links are out of order for diag[i]->row */
		re_link(i);
	}


	/* now make sure all needed elements exist */
	for (el = diag[i]->r_down; el; el = el->r_down) {
		for (pivot = diag[i]->c_right; pivot; pivot = pivot->c_right) {
			IGNORE(getelm(el->row, pivot->col, ELM0));
		}
		reduce_order(el->row);
	}

#if 0
{int j; Item *or;
	printf("%d  ", i);
	for (or = orderlist->next, j=0; j<5 && or != orderlist; j++, or=or->next) {
		printf("(%d, %d)  ", or->elm->row, or->norder);
	}
	printf("\n");
}
#endif
	delete(order);
}

/* The following routines support the concept of a list.
modified from modl
*/

/* Implementation
  The list is a doubly linked list. A special item with element 0 is
  always at the tail of the list and is denoted as the List pointer itself.
  list->next point to the first item in the list and
  list->prev points to the last item in the list.
	i.e. the list is circular
  Note that in an empty list next and prev points to itself.

It is intended that this implementation be hidden from the user via the
following function calls.
*/

static Item *
newitem()
{
	Item *i;
	i = (Item *)emalloc(sizeof(Item));
	i->prev = ITEM0;
	i->next = ITEM0;
	i->norder = 0;
	i->elm = (Elm *)0;
	return i;
}

static List *
newlist()
{
	Item *i;
	i = newitem();
	i->prev = i;
	i->next = i;
	return (List *)i;
}

static void freelist(list)	/*free the list but not the elements*/
	List *list;
{
	Item *i1, *i2;
	for (i1 = list->next; i1 != list; i1 = i2) {
		i2 = i1->next;
		Free(i1);
	}
	Free(list);
}

static void linkitem(item, i)	/*link i before item*/
	Item *item;
	Item *i;
{
	i->prev = item->prev;
	i->next = item;
	item->prev = i;
	i->prev->next = i;
}


static void insert(item)
	Item *item;
{
	Item *i;

	for (i = orderlist->next; i != orderlist; i = i->next) {
		if (i->norder >= item->norder) {
			break;
		}
	}
	linkitem(i, item);
}

static void delete(item)
	Item *item;
{
		
	item->next->prev = item->prev;
	item->prev->next = item->next;
	item->prev = ITEM0;
	item->next = ITEM0;
}

static
void check_assert() {
	/* check that all links are consistent */
	unsigned i;
	Elm *el;
	
	for (i=1; i<=neqn; i++) {
		assert(diag[i]);
		assert(diag[i]->row == diag[i]->col);
		assert(varord[diag[i]->row] == i);
		assert(rowst[i]->row == diag[i]->row);
		for (el = rowst[i]; el; el = el->c_right) {
			if (el == rowst[i]) {
				assert(el->c_left == ELM0);
			}else{
			   assert(el->c_left->c_right == el);
			   assert(varord[el->c_left->col] < varord[el->col]);
			}
		}
		for (el = diag[i]->r_down; el; el = el->r_down) {
			assert(el->r_up->r_down == el);
			assert(varord[el->r_up->row] < varord[el->row]);
		}
		for (el = diag[i]->r_up; el; el = el->r_up) {
			assert(el->r_down->r_up == el);
			assert(varord[el->r_down->row] > varord[el->row]);
		}
	}
}

	/* at this point row links are out of order for diag[i]->col
	   and col links are out of order for diag[i]->row */
static void re_link(i) unsigned i; {

	Elm *el, *dright, *dleft, *dup, *ddown, *elnext;
	
	for (el=rowst[i]; el; el = el->c_right) {
		/* repair hole */
		if (el->r_up) el->r_up->r_down = el->r_down;
		if (el->r_down) el->r_down->r_up = el->r_up;
	}

	for (el=diag[i]->r_down; el; el = el->r_down) {
		/* repair hole */
		if (el->c_right) el->c_right->c_left = el->c_left;
		if (el->c_left) el->c_left->c_right = el->c_right;
		else rowst[varord[el->row]] = el->c_right;
	}

	for (el=diag[i]->r_up; el; el = el->r_up) {
		/* repair hole */
		if (el->c_right) el->c_right->c_left = el->c_left;
		if (el->c_left) el->c_left->c_right = el->c_right;
		else rowst[varord[el->row]] = el->c_right;
	}

   /* matrix is consistent except that diagonal row elements are unlinked from
   their columns and the diagonal column elements are unlinked from their
   rows.
   For simplicity discard all knowledge of links and use getelm to relink
   */
	rowst[i] = diag[i];
	dright = diag[i]->c_right;
	dleft = diag[i]->c_left;
	dup = diag[i]->r_up;
	ddown = diag[i]->r_down;
	diag[i]->c_right = diag[i]->c_left = ELM0;
	diag[i]->r_up = diag[i]->r_down = ELM0;
	for (el=dright; el; el = elnext) {
		elnext = el->c_right;
		IGNORE(getelm(el->row, el->col, el));
	}
	for (el=dleft; el; el = elnext) {
		elnext = el->c_left;
		IGNORE(getelm(el->row, el->col, el));
	}
	for (el=dup; el; el = elnext) {
		elnext = el->r_up;
		IGNORE(getelm(el->row, el->col, el));
	}
	for (el=ddown; el; el = elnext){
		elnext = el->r_down;
		IGNORE(getelm(el->row, el->col, el));
	}
}

static void sparseobj2local(so)
	SparseObj* so;
{
	rowst = so->rowst;
	diag = so->diag;
	neqn = so->neqn;
	varord = so->varord;
	phase = so->phase;
	roworder = so->roworder;
	orderlist = so->orderlist;
	do_flag = so->do_flag;
   /*******************************/
   /* new stuff for vectorization */
  jacob =  so->jacob; 
  rhs =  so->rhs;  
  r_subrow =  so->r_subrow;
}

static void local2sparseobj(so)
	SparseObj* so;
{
	so->rowst = rowst;
	so->diag = diag;
	so->neqn = neqn;
	so->varord = varord;
	so->phase = phase;
	so->roworder = roworder;
	so->orderlist = orderlist;
	so->do_flag = do_flag;
   /*******************************/
   /* new stuff for vectorization */
  so->jacob =  jacob; 
  so->rhs =  rhs;  
  so->r_subrow =  r_subrow;
}

/* copied from scopmath/ssimplic.c */
static int check_state(base, bound, n, s, data)
	int n, *s, base, bound;
	double **data;
{
	int i, flag, k;
	
	flag = 1;
	for (i=0; i<n; i++) {
		for (k = base; k < bound; k++) {
			if ( data1(k, s[i]) < -1e-6) {
				data1(k, s[i]) = 0.;
				flag = 0;
			}
		}
	}
	return flag;
}

/* copied from scopmath/ssimplic.c */
/* parameter list exactly the same as _vector_sparse */
int
_vector__ss_sparse(base, bound, count, jacobp, spacep, sparseobj, neqns, state,
	       deriv, data, t, delta_t, vfun, fun, rhsp, linflag)
     int base;			/* first instances */
     int bound;			/* last instance + 1 */
     int count;			/* total number of instances */
     double *** jacobp;		/* pointer to list of jacobians */
				/* accessed as *jacobp[element][instance] */
     double *** spacep;		/* pointer to workpace */
				/* accessed as *spacep[variable][instance] */
     void ** sparseobj;		/* pointer to sparse obj for this mechanism */
     int neqns;			/* number of state variables */
     int state[];		/* index of state vars in data */
     int deriv[];		/* index of derivs in data */
     double * data[];		/* list of intance data, inst is first idx! */
				/* accessed as data[instance][parameter] */
     double * t;		/* pointer to time */
     double delta_t;		/* delta t */
     int (*vfun)();		/* function to compute jacobian entries */
     int (*fun)();		/* function which calls _vector_getelm */
     double *** rhsp;		/* pointer to list of right hand sides */
				/* accessed as *rhsp[variable][instance] */
     int linflag;		/* if != 0 then linear */
{
  int err, i;
  double ss_dt;
  
  ss_dt=1e9;
  _modl_set_dt(ss_dt);
	
  if (linflag)
    { /*iterate linear solution*/
      err = _vector_sparse(base, bound, count, jacobp, spacep, sparseobj,
			   neqns, state, deriv, data, t, ss_dt, vfun,
			   fun, rhsp, 0);
    }
  else
    {
#define NIT 7
      for (i = 0; i < NIT; i++)
	{
	  err = _vector_sparse(base, bound, count, jacobp, spacep, sparseobj,
			       neqns, state, deriv, data, t, ss_dt, vfun,
			       fun, rhsp, 1);
	    if (err)
	      {
		break;	/* perhaps we should re-start */
	      }
	  if (check_state(base, bound, neqns, state, data))
	    {
	      err = _vector_sparse(base, bound, count, jacobp, spacep,
				   sparseobj, neqns, state, deriv, data,
				   t, ss_dt, vfun, fun, rhsp, 0);
	      break;
	    }
	}		
      if (i >= NIT)
	err = 1;
    }
  _modl_set_dt(delta_t);
  return err;
}

