
/**************************************************************************
**
** Copyright (C) 1993 David E. Steward & Zbigniew Leyk, all rights reserved.
**
**			     Meschach Library
** 
** This Meschach Library is provided "as is" without any express 
** or implied warranty of any kind with respect to this software. 
** In particular the authors shall not be liable for any direct, 
** indirect, special, incidental or consequential damages arising 
** in any way from use of the software.
** 
** Everyone is granted permission to copy, modify and redistribute this
** Meschach Library, provided:
**  1.  All copies contain this copyright notice.
**  2.  All modified copies shall carry a notice stating who
**      made the last modification and the date of such modification.
**  3.  No charge is made for this software or works derived from it.  
**      This clause shall not be construed as constraining other software
**      distributed on the same medium as this software, nor is a
**      distribution fee considered a charge.
**
***************************************************************************

Date		Author		Modification
18 Feb 2000	Gary Holt	Removed definition of u_int since it's now
				handled in config.h.  Was causing compilation
				warnings unnecessarily.
*/

/*
		Type definitions for general purpose maths package
*/

#ifndef	MATRIXH

/* RCS id: $Id: matrix.h 616 2004-04-24 21:28:33Z hines $ */

#define	MATRIXH	

#if defined(__cplusplus)
extern "C" {
#endif

#include	"machine.h"
#include        "err.h"
#include 	"meminfo.h"

#define m_move mesch_m_move
#define OUT mesch_out

#if defined(__MWERKS__) && !defined(_MSC_VER)
#include <types.h>
#else
#include <sys/types.h>
#endif
/* unsigned integer type */
/* This is no longer needed; it's defined in config.h if the compiler hasn't
 * defined it already. */
/* #ifndef U_INT_DEF */
/* typedef	unsigned int	u_int; */
/* #define U_INT_DEF */
/* #endif */

/* vector definition */
typedef	struct	{
		u_int	dim, max_dim;
		Real	*ve;
		} VEC;

/* matrix definition */
typedef	struct	{
		u_int	m, n;
		u_int	max_m, max_n, max_size;
		Real	**me,*base;	/* base is base of alloc'd mem */
		} MAT;

/* band matrix definition */
typedef struct {
               MAT   *mat;       /* matrix */
               int   lb,ub;    /* lower and upper bandwidth */
               } BAND;


/* permutation definition */
typedef	struct	{
		u_int	size, max_size, *pe;
		} PERM;

/* integer vector definition */
typedef struct	{
		u_int	dim, max_dim;
		int	*ive;
	        } IVEC;


#if 1
#include <stdlib.h>
#else
#ifndef MALLOCDECL
#ifndef ANSI_C
extern	char	*malloc(), *calloc(), *realloc();
#else
extern	void	*malloc(size_t),
		*calloc(size_t,size_t),
		*realloc(void *,size_t);
#endif
#endif
#endif

#ifndef ANSI_C
extern void m_version();
#else
void	m_version( void );
#endif

#ifndef ANSI_C
/* allocate one object of given type */
#define	NEW(type)	((type *)calloc(1,sizeof(type)))

/* allocate num objects of given type */
#define	NEW_A(num,type)	((type *)calloc((unsigned)(num),sizeof(type)))

 /* re-allocate arry to have num objects of the given type */
#define	RENEW(var,num,type) \
    ((var)=(type *)((var) ? \
		    realloc((char *)(var),(unsigned)(num)*sizeof(type)) : \
		    calloc((unsigned)(num),sizeof(type))))

#define	MEMCOPY(from,to,n_items,type) \
    MEM_COPY((char *)(from),(char *)(to),(unsigned)(n_items)*sizeof(type))

#else
/* allocate one object of given type */
#define	NEW(type)	((type *)calloc((size_t)1,(size_t)sizeof(type)))

/* allocate num objects of given type */
#define	NEW_A(num,type)	((type *)calloc((size_t)(num),(size_t)sizeof(type)))

 /* re-allocate arry to have num objects of the given type */
#define	RENEW(var,num,type) \
    ((var)=(type *)((var) ? \
		    realloc((char *)(var),(size_t)((num)*sizeof(type))) : \
		    calloc((size_t)(num),(size_t)sizeof(type))))

#define	MEMCOPY(from,to,n_items,type) \
 MEM_COPY((char *)(from),(char *)(to),(unsigned)(n_items)*sizeof(type))

#endif

/* type independent min and max operations */
#ifndef max
#define	max(a,b)	((a) > (b) ? (a) : (b))
#endif
#ifndef min
#define	min(a,b)	((a) > (b) ? (b) : (a))
#endif


#undef TRUE
#define	TRUE	1
#undef FALSE
#define	FALSE	0


/* for input routines */
#define MAXLINE 81


/* Dynamic memory allocation */

/* Should use M_FREE/V_FREE/PX_FREE in programs instead of m/v/px_free()
   as this is considerably safer -- also provides a simple type check ! */

#ifndef ANSI_C

extern	VEC *v_get(), *v_resize();
extern	MAT *m_get(), *m_resize();
extern	PERM *px_get(), *px_resize();
extern	IVEC *iv_get(), *iv_resize();
extern	int m_free(),v_free();
extern  int px_free();
extern  int iv_free();
extern  BAND *bd_get(), *bd_resize();
extern  int bd_free();

#else

/* get/resize vector to given dimension */
extern	VEC *v_get(int), *v_resize(VEC *,int);
/* get/resize matrix to be m x n */
extern	MAT *m_get(int,int), *m_resize(MAT *,int,int);
/* get/resize permutation to have the given size */
extern	PERM *px_get(int), *px_resize(PERM *,int);
/* get/resize an integer vector to given dimension */
extern	IVEC *iv_get(int), *iv_resize(IVEC *,int);
/* get/resize a band matrix to given dimension */
extern  BAND *bd_get(int,int,int), *bd_resize(BAND *,int,int,int);

/* free (de-allocate) (band) matrices, vectors, permutations and 
   integer vectors */
extern  int iv_free(IVEC *);
extern	int m_free(MAT *),v_free(VEC *),px_free(PERM *);
extern   int bd_free(BAND *);

#endif


/* MACROS */

/* macros that also check types and sets pointers to NULL */
#define	M_FREE(mat)	( m_free(mat),	(mat)=(MAT *)NULL )
#define V_FREE(vec)	( v_free(vec),	(vec)=(VEC *)NULL )
#define	PX_FREE(px)	( px_free(px),	(px)=(PERM *)NULL )
#define	IV_FREE(iv)	( iv_free(iv),	(iv)=(IVEC *)NULL )

#define MAXDIM  	2001


/* Entry level access to data structures */
#ifdef DEBUG

/* returns x[i] */
#define	v_entry(x,i)	(((i) < 0 || (i) >= (x)->dim) ? \
			 error(E_BOUNDS,"v_entry"), 0.0 : (x)->ve[i] )

/* x[i] <- val */
#define	v_set_val(x,i,val) ((x)->ve[i] = ((i) < 0 || (i) >= (x)->dim) ? \
			    error(E_BOUNDS,"v_set_val"), 0.0 : (val))

/* x[i] <- x[i] + val */
#define	v_add_val(x,i,val) ((x)->ve[i] += ((i) < 0 || (i) >= (x)->dim) ? \
			    error(E_BOUNDS,"v_add_val"), 0.0 : (val))

/* x[i] <- x[i] - val */
#define	v_sub_val(x,i,val) ((x)->ve[i] -= ((i) < 0 || (i) >= (x)->dim) ? \
			    error(E_BOUNDS,"v_sub_val"), 0.0 : (val))

/* returns A[i][j] */
#define	m_entry(A,i,j)	(((i) < 0 || (i) >= (A)->m || \
			  (j) < 0 || (j) >= (A)->n) ? \
			 error(E_BOUNDS,"m_entry"), 0.0 : (A)->me[i][j] )

/* A[i][j] <- val */
#define	m_set_val(A,i,j,val) ((A)->me[i][j] = ((i) < 0 || (i) >= (A)->m || \
					       (j) < 0 || (j) >= (A)->n) ? \
			      error(E_BOUNDS,"m_set_val"), 0.0 : (val) )

/* A[i][j] <- A[i][j] + val */
#define	m_add_val(A,i,j,val) ((A)->me[i][j] += ((i) < 0 || (i) >= (A)->m || \
						(j) < 0 || (j) >= (A)->n) ? \
			      error(E_BOUNDS,"m_add_val"), 0.0 : (val) )

/* A[i][j] <- A[i][j] - val */
#define	m_sub_val(A,i,j,val) ((A)->me[i][j] -= ((i) < 0 || (i) >= (A)->m || \
						(j) < 0 || (j) >= (A)->n) ? \
			      error(E_BOUNDS,"m_sub_val"), 0.0 : (val) )
#else

/* returns x[i] */
#define	v_entry(x,i)		((x)->ve[i])

/* x[i] <- val */
#define	v_set_val(x,i,val)	((x)->ve[i]  = (val))

/* x[i] <- x[i] + val */
#define	v_add_val(x,i,val)	((x)->ve[i] += (val))

 /* x[i] <- x[i] - val */
#define	v_sub_val(x,i,val)	((x)->ve[i] -= (val))

/* returns A[i][j] */
#define	m_entry(A,i,j)		((A)->me[i][j])

/* A[i][j] <- val */
#define	m_set_val(A,i,j,val)	((A)->me[i][j]  = (val) )

/* A[i][j] <- A[i][j] + val */
#define	m_add_val(A,i,j,val)	((A)->me[i][j] += (val) )

/* A[i][j] <- A[i][j] - val */
#define	m_sub_val(A,i,j,val)	((A)->me[i][j] -= (val) )

#endif


/* I/O routines */
#ifndef ANSI_C

extern	void v_foutput(),m_foutput(),px_foutput();
extern  void iv_foutput();
extern	VEC *v_finput();
extern	MAT *m_finput();
extern	PERM *px_finput();
extern	IVEC *iv_finput();
extern	int fy_or_n(), fin_int(), yn_dflt(), skipjunk();
extern	double fin_double();

#else

/* print x on file fp */
void v_foutput(FILE *fp,VEC *x),
       /* print A on file fp */
	m_foutput(FILE *fp,MAT *A),
       /* print px on file fp */
	px_foutput(FILE *fp,PERM *px);
/* print ix on file fp */
void iv_foutput(FILE *fp,IVEC *ix);

/* Note: if out is NULL, then returned object is newly allocated;
        Also: if out is not NULL, then that size is assumed */

/* read in vector from fp */
VEC *v_finput(FILE *fp,VEC *out);
/* read in matrix from fp */
MAT *m_finput(FILE *fp,MAT *out);
/* read in permutation from fp */
PERM *px_finput(FILE *fp,PERM *out);
/* read in int vector from fp */
IVEC *iv_finput(FILE *fp,IVEC *out);

/* fy_or_n -- yes-or-no to question in string s
        -- question written to stderr, input from fp 
        -- if fp is NOT a tty then return y_n_dflt */
int fy_or_n(FILE *fp,char *s);

/* yn_dflt -- sets the value of y_n_dflt to val */
int yn_dflt(int val);

/* fin_int -- return integer read from file/stream fp
        -- prompt s on stderr if fp is a tty
        -- check that x lies between low and high: re-prompt if
                fp is a tty, error exit otherwise
        -- ignore check if low > high           */
int fin_int(FILE *fp,char *s,int low,int high);

/* fin_double -- return double read from file/stream fp
        -- prompt s on stderr if fp is a tty
        -- check that x lies between low and high: re-prompt if
                fp is a tty, error exit otherwise
        -- ignore check if low > high           */
double fin_double(FILE *fp,char *s,double low,double high);

/* it skips white spaces and strings of the form #....\n
   Here .... is a comment string */
int skipjunk(FILE *fp);

#endif


/* MACROS */

/* macros to use stdout and stdin instead of explicit fp */
#define	v_output(vec)	v_foutput(stdout,vec)
#define	v_input(vec)	v_finput(stdin,vec)
#define	m_output(mat)	m_foutput(stdout,mat)
#define	m_input(mat)	m_finput(stdin,mat)
#define	px_output(px)	px_foutput(stdout,px)
#define	px_input(px)	px_finput(stdin,px)
#define	iv_output(iv)	iv_foutput(stdout,iv)
#define	iv_input(iv)	iv_finput(stdin,iv)

/* general purpose input routine; skips comments # ... \n */
#define	finput(fp,prompt,fmt,var) \
	( ( isatty(fileno(fp)) ? fprintf(stderr,prompt) : skipjunk(fp) ), \
							fscanf(fp,fmt,var) )
#define	input(prompt,fmt,var)	finput(stdin,prompt,fmt,var)
#define	fprompter(fp,prompt) \
	( isatty(fileno(fp)) ? fprintf(stderr,prompt) : skipjunk(fp) )
#define	prompter(prompt)	fprompter(stdin,prompt)
#define	y_or_n(s)	fy_or_n(stdin,s)
#define	in_int(s,lo,hi)	fin_int(stdin,s,lo,hi)
#define	in_double(s,lo,hi)	fin_double(stdin,s,lo,hi)

/* Copying routines */
#ifndef ANSI_C
extern	MAT	*_m_copy(), *m_move(), *vm_move();
extern	VEC	*_v_copy(), *v_move(), *mv_move();
extern	PERM	*px_copy();
extern	IVEC	*iv_copy(), *iv_move();
extern  BAND    *bd_copy();

#else

/* copy in to out starting at out[i0][j0] */
extern	MAT	*_m_copy(MAT *in,MAT *out,u_int i0,u_int j0),
		* m_move(MAT *in, int, int, int, int, MAT *out, int, int),
		*vm_move(VEC *in, int, MAT *out, int, int, int, int);
/* copy in to out starting at out[i0] */
extern	VEC	*_v_copy(VEC *in,VEC *out,u_int i0),
		* v_move(VEC *in, int, int, VEC *out, int),
		*mv_move(MAT *in, int, int, int, int, VEC *out, int);
extern	PERM	*px_copy(PERM *in,PERM *out);
extern	IVEC	*iv_copy(IVEC *in,IVEC *out),
		*iv_move(IVEC *in, int, int, IVEC *out, int);
extern  BAND    *bd_copy(BAND *in,BAND *out);

#endif


/* MACROS */
#define	m_copy(in,out)	_m_copy(in,out,0,0)
#define	v_copy(in,out)	_v_copy(in,out,0)


/* Initialisation routines -- to be zero, ones, random or identity */
#ifndef ANSI_C
extern	VEC     *v_zero(), *v_rand(), *v_ones();
extern	MAT     *m_zero(), *m_ident(), *m_rand(), *m_ones();
extern	PERM    *px_ident();
extern  IVEC    *iv_zero();
#else
extern	VEC     *v_zero(VEC *), *v_rand(VEC *), *v_ones(VEC *);
extern	MAT     *m_zero(MAT *), *m_ident(MAT *), *m_rand(MAT *),
						*m_ones(MAT *);
extern	PERM    *px_ident(PERM *);
extern  IVEC    *iv_zero(IVEC *);
#endif

/* Basic vector operations */
#ifndef ANSI_C
extern	VEC *sv_mlt(), *mv_mlt(), *vm_mlt(), *v_add(), *v_sub(),
		*px_vec(), *pxinv_vec(), *v_mltadd(), *v_map(), *_v_map(),
		*v_lincomb(), *v_linlist();
extern	double	v_min(), v_max(), v_sum();
extern	VEC	*v_star(), *v_slash(), *v_sort();
extern	double _in_prod(), __ip__();
extern	void	__mltadd__(), __add__(), __sub__(), 
                __smlt__(), __zero__();
#else

extern	VEC	*sv_mlt(double,VEC *,VEC *),	/* out <- s.x */
		*mv_mlt(MAT *,VEC *,VEC *),	/* out <- A.x */
		*vm_mlt(MAT *,VEC *,VEC *),	/* out^T <- x^T.A */
		*v_add(VEC *,VEC *,VEC *), 	/* out <- x + y */
                *v_sub(VEC *,VEC *,VEC *),	/* out <- x - y */
		*px_vec(PERM *,VEC *,VEC *),	/* out <- P.x */
		*pxinv_vec(PERM *,VEC *,VEC *),	  /* out <- P^{-1}.x */
		*v_mltadd(VEC *,VEC *,double,VEC *),   /* out <- x + s.y */
#ifdef PROTOTYPES_IN_STRUCT
		*v_map(double (*f)(double),VEC *,VEC *),
                                                 /* out[i] <- f(x[i]) */
		*_v_map(double (*f)(void *,double),void *,VEC *,VEC *),
#else
		*v_map(double (*f)(),VEC *,VEC *), /* out[i] <- f(x[i]) */
		*_v_map(double (*f)(),void *,VEC *,VEC *),
#endif
		*v_lincomb(int,VEC **,Real *,VEC *),
                                                 /* out <- sum_i s[i].x[i] */
                *v_linlist(VEC *out,VEC *v1,double a1,...);
                                              /* out <- s1.x1 + s2.x2 + ... */

/* returns min_j x[j] (== x[i]) */
extern	double	v_min(VEC *, int *),
     /* returns max_j x[j] (== x[i]) */
        v_max(VEC *, int *),
        /* returns sum_i x[i] */
        v_sum(VEC *);

/* Hadamard product: out[i] <- x[i].y[i] */
extern	VEC	*v_star(VEC *, VEC *, VEC *),
                 /* out[i] <- x[i] / y[i] */
		*v_slash(VEC *, VEC *, VEC *),
               /* sorts x, and sets order so that sorted x[i] = x[order[i]] */
		*v_sort(VEC *, PERM *);

/* returns inner product starting at component i0 */
extern	double	_in_prod(VEC *x,VEC *y,u_int i0),
                /* returns sum_{i=0}^{len-1} x[i].y[i] */
                __ip__(Real *,Real *,int);

/* see v_mltadd(), v_add(), v_sub() and v_zero() */
extern	void	__mltadd__(Real *,Real *,double,int),
		__add__(Real *,Real *,Real *,int),
		__sub__(Real *,Real *,Real *,int),
                __smlt__(Real *,double,Real *,int),
		__zero__(Real *,int);

#endif


/* MACRO */
/* usual way of computing the inner product */
#define	in_prod(a,b)	_in_prod(a,b,0)

/* Norms */
/* scaled vector norms -- scale == NULL implies unscaled */
#ifndef ANSI_C

extern	double	_v_norm1(), _v_norm2(), _v_norm_inf(),
		m_norm1(), m_norm_inf(), m_norm_frob();

#else
               /* returns sum_i |x[i]/scale[i]| */
extern	double	_v_norm1(VEC *x,VEC *scale),
               /* returns (scaled) Euclidean norm */
                _v_norm2(VEC *x,VEC *scale),
               /* returns max_i |x[i]/scale[i]| */
		_v_norm_inf(VEC *x,VEC *scale);

/* unscaled matrix norms */
extern double m_norm1(MAT *A), m_norm_inf(MAT *A), m_norm_frob(MAT *A);

#endif


/* MACROS */
/* unscaled vector norms */
#define	v_norm1(x)	_v_norm1(x,VNULL)
#define	v_norm2(x)	_v_norm2(x,VNULL)
#define	v_norm_inf(x)	_v_norm_inf(x,VNULL)

/* Basic matrix operations */
#ifndef ANSI_C

extern	MAT *sm_mlt(), *m_mlt(), *mmtr_mlt(), *mtrm_mlt(), *m_add(), *m_sub(),
		*sub_mat(), *m_transp(), *ms_mltadd();

extern   BAND *bd_transp();
extern	MAT *px_rows(), *px_cols(), *swap_rows(), *swap_cols(),
             *_set_row(), *_set_col();
extern	VEC *get_row(), *get_col(), *sub_vec(),
		*mv_mltadd(), *vm_mltadd();

#else

extern	MAT	*sm_mlt(double s,MAT *A,MAT *out), 	/* out <- s.A */
		*m_mlt(MAT *A,MAT *B,MAT *out),	/* out <- A.B */
		*mmtr_mlt(MAT *A,MAT *B,MAT *out),	/* out <- A.B^T */
		*mtrm_mlt(MAT *A,MAT *B,MAT *out),	/* out <- A^T.B */
		*m_add(MAT *A,MAT *B,MAT *out),	/* out <- A + B */
		*m_sub(MAT *A,MAT *B,MAT *out),	/* out <- A - B */
		*sub_mat(MAT *A,u_int,u_int,u_int,u_int,MAT *out),
		*m_transp(MAT *A,MAT *out),		/* out <- A^T */
                /* out <- A + s.B */
		*ms_mltadd(MAT *A,MAT *B,double s,MAT *out);


extern  BAND    *bd_transp(BAND *in, BAND *out);   /* out <- A^T */
extern	MAT	*px_rows(PERM *px,MAT *A,MAT *out),	/* out <- P.A */
		*px_cols(PERM *px,MAT *A,MAT *out),	/* out <- A.P^T */
		*swap_rows(MAT *,int,int,int,int),
		*swap_cols(MAT *,int,int,int,int),
                 /* A[i][j] <- out[j], j >= j0 */
		*_set_col(MAT *A,u_int i,VEC *out,u_int j0),
                 /* A[i][j] <- out[i], i >= i0 */
		*_set_row(MAT *A,u_int j,VEC *out,u_int i0);

extern	VEC	*get_row(MAT *,u_int,VEC *),
		*get_col(MAT *,u_int,VEC *),
		*sub_vec(VEC *,int,int,VEC *),
                   /* out <- x + s.A.y */
		*mv_mltadd(VEC *x,VEC *y,MAT *A,double s,VEC *out),
                  /* out^T <- x^T + s.y^T.A */
		*vm_mltadd(VEC *x,VEC *y,MAT *A,double s,VEC *out);
#endif


/* MACROS */
/* row i of A <- vec */
#define	set_row(mat,row,vec)	_set_row(mat,row,vec,0)
/* col j of A <- vec */
#define	set_col(mat,col,vec)	_set_col(mat,col,vec,0)


/* Basic permutation operations */
#ifndef ANSI_C

extern	PERM *px_mlt(), *px_inv(), *px_transp();
extern	int  px_sign();

#else

extern	PERM	*px_mlt(PERM *px1,PERM *px2,PERM *out),	/* out <- px1.px2 */
		*px_inv(PERM *px,PERM *out),	/* out <- px^{-1} */
                 /* swap px[i] and px[j] */
		*px_transp(PERM *px,u_int i,u_int j);

     /* returns sign(px) = +1 if px product of even # transpositions
                           -1 if ps product of odd  # transpositions */
extern	int	px_sign(PERM *);

#endif


/* Basic integer vector operations */
#ifndef ANSI_C

extern	IVEC	*iv_add(), *iv_sub(), *iv_sort();

#else

extern	IVEC	*iv_add(IVEC *ix,IVEC *iy,IVEC *out),  /* out <- ix + iy */
		*iv_sub(IVEC *ix,IVEC *iy,IVEC *out),  /* out <- ix - iy */
        /* sorts ix & sets order so that sorted ix[i] = old ix[order[i]] */
		*iv_sort(IVEC *ix, PERM *order);

#endif


/* miscellaneous functions */

#ifndef ANSI_C

extern	double	square(), cube(), mrand();
extern	void	smrand(), mrandlist();
extern  void    m_dump(), px_dump(), v_dump(), iv_dump();
extern MAT *band2mat();
extern BAND *mat2band();

#else

double	square(double x), 	/* returns x^2 */
  cube(double x), 		/* returns x^3 */
  mrand(void);                  /* returns random # in [0,1) */

void	smrand(int seed),            /* seeds mrand() */
  mrandlist(Real *x, int len);       /* generates len random numbers */

void    m_dump(FILE *fp,MAT *a), px_dump(FILE *,PERM *px),
        v_dump(FILE *fp,VEC *x), iv_dump(FILE *fp, IVEC *ix);

MAT *band2mat(BAND *bA, MAT *A);
BAND *mat2band(MAT *A, int lb,int ub, BAND *bA);

#endif


/* miscellaneous constants */
#define	VNULL	((VEC *)NULL)
#define	MNULL	((MAT *)NULL)
#define	PNULL	((PERM *)NULL)
#define	IVNULL	((IVEC *)NULL)
#define BDNULL  ((BAND *)NULL)



/* varying number of arguments */

#ifdef ANSI_C
#include <stdarg.h>

/* prototypes */

int v_get_vars(int dim,...);
int iv_get_vars(int dim,...);
int m_get_vars(int m,int n,...);
int px_get_vars(int dim,...);

int v_resize_vars(int new_dim,...);
int iv_resize_vars(int new_dim,...);
int m_resize_vars(int m,int n,...);
int px_resize_vars(int new_dim,...);

int v_free_vars(VEC **,...);
int iv_free_vars(IVEC **,...);
int px_free_vars(PERM **,...);
int m_free_vars(MAT **,...);

#elif VARARGS
/* old varargs is used */

#include  <varargs.h>

/* prototypes */

int v_get_vars();
int iv_get_vars();
int m_get_vars();
int px_get_vars();

int v_resize_vars();
int iv_resize_vars();
int m_resize_vars();
int px_resize_vars();

int v_free_vars();
int iv_free_vars();
int px_free_vars();
int m_free_vars();

#endif

#if defined(__cplusplus)
}
#endif

#endif


