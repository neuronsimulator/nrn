#include <../../nrnconf.h>
/******************************************************************************
 *
 * File: csoda.c
 *
 *  Fortran-77 original (LSODA) obtained from 'netlib@ornl.gov'.
 *  Combined with supporting routines from Linpack and translated into C by
 *  'f2c' (obtained from 'netlib@att.research.com').
 *
 ******************************************************************************/
#define d1mach_ csoda_d1mach
#define daxpy_ csoda_daxpy
#define ddot_ csoda_ddot
#define dgbfa_ csoda_dgbfa
#define dgbsl_ csoda_dgbsl
#define dgefa_ csoda_dgefa
#define dgesl_ csoda_dgesl
#define dscal_ csoda_dscal
#define idamax_ csoda_idamax

#ifndef LINT
static char RCSid[] =
    "csoda.c,v 1.1.1.1 1994/10/12 17:22:19 hines Exp" ;
#endif

/*  -- translated by f2c (version of 4 June 1990  12:16:42).
   You must link the resulting object file with the libraries:
	-lF77 -lI77 -lm -lc   (in that order)
*/

#include "f2c.h"

/* Common Block Declarations */

struct ls0001_1_ {
    doublereal rowns[209], ccmax, el0, h, hmin, hmxi, hu, rc, tn, uround;
    integer illin, init, lyh, lewt, lacor, lsavf, lwm, liwm, mxstep, mxhnil, 
	    nhnil, ntrep, nslast, nyh, iowns[6], icf, ierpj, iersl, jcur, 
	    jstart, kflag, l, meth, miter, maxord, maxcor, msbp, mxncf, n, nq,
	     nst, nfe, nje, nqu;
};
struct ls0001_2_ {
    doublereal rowns[209], ccmax, el0, h, hmin, hmxi, hu, rc, tn, uround;
    integer iownd[14], iowns[6], icf, ierpj, iersl, jcur, jstart, kflag, l, 
	    meth, miter, maxord, maxcor, msbp, mxncf, n, nq, nst, nfe, nje, 
	    nqu;
};
struct ls0001_3_ {
    doublereal conit, crate, el[13], elco[156]	/* was [13][12] */, hold, 
	    rmax, tesco[36]	/* was [3][12] */, ccmax, el0, h, hmin, hmxi, 
	    hu, rc, tn, uround;
    integer iownd[14], ialth, ipup, lmax, meo, nqnyh, nslp, icf, ierpj, iersl,
	     jcur, jstart, kflag, l, meth, miter, maxord, maxcor, msbp, mxncf,
	     n, nq, nst, nfe, nje, nqu;
};
struct ls0001_4_ {
    doublereal rowns[209], rcomm[9];
    integer illin, iduma[10], ntrep, idumb[2], iowns[6], icomm[19];
};

#define ls0001_1 (*(struct ls0001_1_ *) &ls0001_)
#define ls0001_2 (*(struct ls0001_2_ *) &ls0001_)
#define ls0001_3 (*(struct ls0001_3_ *) &ls0001_)
#define ls0001_4 (*(struct ls0001_4_ *) &ls0001_)

union {
    struct {
	doublereal tsw, rowns2[20], pdnorm;
	integer insufr, insufi, ixpr, iowns2[2], jtyp, mused, mxordn, mxords;
    } _1;
    struct {
	doublereal rownd2, rowns2[20], pdnorm;
	integer iownd2[3], iowns2[2], jtyp, mused, mxordn, mxords;
    } _2;
    struct {
	doublereal rownd2, pdest, pdlast, ratio, cm1[12], cm2[5], pdnorm;
	integer iownd2[3], icount, irflag, jtyp, mused, mxordn, mxords;
    } _3;
} lsa001_;

#define lsa001_1 (lsa001_._1)
#define lsa001_2 (lsa001_._2)
#define lsa001_3 (lsa001_._3)

struct eh0001_1_ {
    integer mesflg, lunit;
};

#define eh0001_1 (*(struct eh0001_1_ *) &eh0001_)

/* Initialized data */

struct {
    doublereal fill_1[218];
    integer e_2;
    integer fill_3[10];
    integer e_4;
    doublereal fill_5[14];
    doublereal e_6;
    } ls0001_ = { {0}, 0, {0}, 0, {0}, 0. };

struct {
    integer e_1[2];
    } eh0001_ = { 1, 6 };


/* Table of constant values */

static integer c__1 = 1;
static integer c__60 = 60;
static integer c__103 = 103;
static integer c__0 = 0;
static doublereal c_b136 = 0.;
static integer c__50 = 50;
static integer c__2 = 2;
static integer c__104 = 104;
static integer c__4 = 4;
static integer c__101 = 101;
static integer c__102 = 102;
static integer c__105 = 105;
static integer c__106 = 106;
static integer c__107 = 107;
static integer c__301 = 301;
static integer c__201 = 201;
static integer c__202 = 202;
static integer c__203 = 203;
static integer c__204 = 204;
static integer c__205 = 205;
static integer c__30 = 30;
static integer c__206 = 206;
static integer c__207 = 207;
static integer c__3 = 3;
static integer c__5 = 5;
static integer c__6 = 6;
static integer c__7 = 7;
static integer c__8 = 8;
static integer c__9 = 9;
static integer c__10 = 10;
static integer c__11 = 11;
static integer c__12 = 12;
static integer c__13 = 13;
static integer c__40 = 40;
static integer c__14 = 14;
static integer c__15 = 15;
static integer c__16 = 16;
static integer c__17 = 17;
static integer c__18 = 18;
static integer c__19 = 19;
static integer c__20 = 20;
static integer c__21 = 21;
static integer c__22 = 22;
static integer c__23 = 23;
static integer c__24 = 24;
static integer c__25 = 25;
static integer c__26 = 26;
static integer c__27 = 27;
static integer c__28 = 28;
static integer c__29 = 29;
static integer c__302 = 302;
static integer c__303 = 303;
static integer c__51 = 51;
static integer c__52 = 52;

doublereal d1mach_(idum)
integer *idum;
{
    /* System generated locals */
    doublereal ret_val;

    /* Local variables */
    static doublereal comp, u;

/*-----------------------------------------------------------------------A
UX00030*/
/*THIS ROUTINE COMPUTES THE UNIT ROUNDOFF OF THE MACHINE IN DOUBLE      AU
X00040*/
/*PRECISION.  THIS IS DEFINED AS THE SMALLEST POSITIVE MACHINE NUMBER   AU
X00050*/
/*U SUCH THAT  1.0D0 + U .NE. 1.0D0 (IN DOUBLE PRECISION).              AU
X00060*/
/*-----------------------------------------------------------------------A
UX00070*/
    u = 1.;
L10:
    u *= .5;
    comp = u + 1.;
    if (comp != 1.) {
	goto L10;
    }
    ret_val = u * 2.;
    return ret_val;
/*----------------------- END OF FUNCTION D1MACH ------------------------A
UX00150*/
} /* d1mach_ */

/* Subroutine */ int daxpy_(n, da, dx, incx, dy, incy)
integer *n;
doublereal *da, *dx;
integer *incx;
doublereal *dy;
integer *incy;
{
    /* System generated locals */
    integer i_1;

    /* Local variables */
    static integer i, m, ix, iy, mp1;

/*                                                                      AU
X06830*/
/*    CONSTANT TIMES A VECTOR PLUS A VECTOR.                            AU
X06840*/
/*    USES UNROLLED LOOPS FOR INCREMENTS EQUAL TO ONE.                  AU
X06850*/
/*    JACK DONGARRA, LINPACK, 3/11/78.                                  AU
X06860*/
/*                                                                      AU
X06870*/
/*                                                                      AU
X06900*/
    /* Parameter adjustments */
    --dy;
    --dx;

    /* Function Body */
    if (*n <= 0) {
	return 0;
    }
    if (*da == 0.) {
	return 0;
    }
    if (*incx == 1 && *incy == 1) {
	goto L20;
    }
/*                                                                      AU
X06940*/
/*       CODE FOR UNEQUAL INCREMENTS OR EQUAL INCREMENTS                AU
X06950*/
/*         NOT EQUAL TO 1                                               AU
X06960*/
/*                                                                      AU
X06970*/
    ix = 1;
    iy = 1;
    if (*incx < 0) {
	ix = (-(*n) + 1) * *incx + 1;
    }
    if (*incy < 0) {
	iy = (-(*n) + 1) * *incy + 1;
    }
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
	dy[iy] += *da * dx[ix];
	ix += *incx;
	iy += *incy;
/* L10: */
    }
    return 0;
/*                                                                      AU
X07080*/
/*       CODE FOR BOTH INCREMENTS EQUAL TO 1                            AU
X07090*/
/*                                                                      AU
X07100*/
/*                                                                      AU
X07110*/
/*       CLEAN-UP LOOP                                                  AU
X07120*/
/*                                                                      AU
X07130*/
L20:
    m = *n % 4;
    if (m == 0) {
	goto L40;
    }
    i_1 = m;
    for (i = 1; i <= i_1; ++i) {
	dy[i] += *da * dx[i];
/* L30: */
    }
    if (*n < 4) {
	return 0;
    }
L40:
    mp1 = m + 1;
    i_1 = *n;
    for (i = mp1; i <= i_1; i += 4) {
	dy[i] += *da * dx[i];
	dy[i + 1] += *da * dx[i + 1];
	dy[i + 2] += *da * dx[i + 2];
	dy[i + 3] += *da * dx[i + 3];
/* L50: */
    }
    return 0;
} /* daxpy_ */

doublereal ddot_(n, dx, incx, dy, incy)
integer *n;
doublereal *dx;
integer *incx;
doublereal *dy;
integer *incy;
{
    /* System generated locals */
    integer i_1;
    doublereal ret_val;

    /* Local variables */
    static integer i, m;
    static doublereal dtemp;
    static integer ix, iy, mp1;

/*                                                                      AU
X07710*/
/*    FORMS THE DOT PRODUCT OF TWO VECTORS.                             AU
X07720*/
/*    USES UNROLLED LOOPS FOR INCREMENTS EQUAL TO ONE.                  AU
X07730*/
/*    JACK DONGARRA, LINPACK, 3/11/78.                                  AU
X07740*/
/*                                                                      AU
X07750*/
/*                                                                      AU
X07780*/
    /* Parameter adjustments */
    --dy;
    --dx;

    /* Function Body */
    ret_val = 0.;
    dtemp = 0.;
    if (*n <= 0) {
	return ret_val;
    }
    if (*incx == 1 && *incy == 1) {
	goto L20;
    }
/*                                                                      AU
X07830*/
/*       CODE FOR UNEQUAL INCREMENTS OR EQUAL INCREMENTS                AU
X07840*/
/*         NOT EQUAL TO 1                                               AU
X07850*/
/*                                                                      AU
X07860*/
    ix = 1;
    iy = 1;
    if (*incx < 0) {
	ix = (-(*n) + 1) * *incx + 1;
    }
    if (*incy < 0) {
	iy = (-(*n) + 1) * *incy + 1;
    }
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
	dtemp += dx[ix] * dy[iy];
	ix += *incx;
	iy += *incy;
/* L10: */
    }
    ret_val = dtemp;
    return ret_val;
/*                                                                      AU
X07980*/
/*       CODE FOR BOTH INCREMENTS EQUAL TO 1                            AU
X07990*/
/*                                                                      AU
X08000*/
/*                                                                      AU
X08010*/
/*       CLEAN-UP LOOP                                                  AU
X08020*/
/*                                                                      AU
X08030*/
L20:
    m = *n % 5;
    if (m == 0) {
	goto L40;
    }
    i_1 = m;
    for (i = 1; i <= i_1; ++i) {
	dtemp += dx[i] * dy[i];
/* L30: */
    }
    if (*n < 5) {
	goto L60;
    }
L40:
    mp1 = m + 1;
    i_1 = *n;
    for (i = mp1; i <= i_1; i += 5) {
	dtemp = dtemp + dx[i] * dy[i] + dx[i + 1] * dy[i + 1] + dx[i + 2] * 
		dy[i + 2] + dx[i + 3] * dy[i + 3] + dx[i + 4] * dy[i + 4];
/* L50: */
    }
L60:
    ret_val = dtemp;
    return ret_val;
} /* ddot_ */

/* Subroutine */ int dgbfa_(abd, lda, n, ml, mu, ipvt, info)
doublereal *abd;
integer *lda, *n, *ml, *mu, *ipvt, *info;
{
    /* System generated locals */
    integer abd_dim1, abd_offset, i_1, i_2, i_3, i_4;

    /* Local variables */
    static integer i, j, k, l, m;
    static doublereal t;
    extern /* Subroutine */ int dscal_(), daxpy_();
    static integer i0, j0, j1, lm, mm, ju;
    extern integer idamax_();
    static integer jz, kp1, nm1;

/*                                                                      AU
X03760*/
/*    DGBFA FACTORS A DOUBLE PRECISION BAND MATRIX BY ELIMINATION.      AU
X03770*/
/*                                                                      AU
X03780*/
/*    DGBFA IS USUALLY CALLED BY DGBCO, BUT IT CAN BE CALLED            AU
X03790*/
/*    DIRECTLY WITH A SAVING IN TIME IF  RCOND  IS NOT NEEDED.          AU
X03800*/
/*                                                                      AU
X03810*/
/*    ON ENTRY                                                          AU
X03820*/
/*                                                                      AU
X03830*/
/*       ABD     DOUBLE PRECISION(LDA, N)                               AU
X03840*/
/*               CONTAINS THE MATRIX IN BAND STORAGE.  THE COLUMNS      AU
X03850*/
/*               OF THE MATRIX ARE STORED IN THE COLUMNS OF  ABD  AND   AU
X03860*/
/*               THE DIAGONALS OF THE MATRIX ARE STORED IN ROWS         AU
X03870*/
/*               ML+1 THROUGH 2*ML+MU+1 OF  ABD .                       AU
X03880*/
/*               SEE THE COMMENTS BELOW FOR DETAILS.                    AU
X03890*/
/*                                                                      AU
X03900*/
/*       LDA     INTEGER                                                AU
X03910*/
/*               THE LEADING DIMENSION OF THE ARRAY  ABD .              AU
X03920*/
/*               LDA MUST BE .GE. 2*ML + MU + 1 .                       AU
X03930*/
/*                                                                      AU
X03940*/
/*       N       INTEGER                                                AU
X03950*/
/*               THE ORDER OF THE ORIGINAL MATRIX.                      AU
X03960*/
/*                                                                      AU
X03970*/
/*       ML      INTEGER                                                AU
X03980*/
/*               NUMBER OF DIAGONALS BELOW THE MAIN DIAGONAL.           AU
X03990*/
/*               0 .LE. ML .LT. N .                                     AU
X04000*/
/*                                                                      AU
X04010*/
/*       MU      INTEGER                                                AU
X04020*/
/*               NUMBER OF DIAGONALS ABOVE THE MAIN DIAGONAL.           AU
X04030*/
/*               0 .LE. MU .LT. N .                                     AU
X04040*/
/*               MORE EFFICIENT IF  ML .LE. MU .                        AU
X04050*/
/*    ON RETURN                                                         AU
X04060*/
/*                                                                      AU
X04070*/
/*       ABD     AN UPPER TRIANGULAR MATRIX IN BAND STORAGE AND         AU
X04080*/
/*               THE MULTIPLIERS WHICH WERE USED TO OBTAIN IT.          AU
X04090*/
/*               THE FACTORIZATION CAN BE WRITTEN  A = L*U  WHERE       AU
X04100*/
/*               L  IS A PRODUCT OF PERMUTATION AND UNIT LOWER          AU
X04110*/
/*               TRIANGULAR MATRICES AND  U  IS UPPER TRIANGULAR.       AU
X04120*/
/*                                                                      AU
X04130*/
/*       IPVT    INTEGER(N)                                             AU
X04140*/
/*               AN INTEGER VECTOR OF PIVOT INDICES.                    AU
X04150*/
/*                                                                      AU
X04160*/
/*       INFO    INTEGER                                                AU
X04170*/
/*               = 0  NORMAL VALUE.                                     AU
X04180*/
/*               = K  IF  U(K,K) .EQ. 0.0 .  THIS IS NOT AN ERROR       AU
X04190*/
/*                    CONDITION FOR THIS SUBROUTINE, BUT IT DOES        AU
X04200*/
/*                    INDICATE THAT DGBSL WILL DIVIDE BY ZERO IF        AU
X04210*/
/*                    CALLED.  USE  RCOND  IN DGBCO FOR A RELIABLE      AU
X04220*/
/*                    INDICATION OF SINGULARITY.                        AU
X04230*/
/*                                                                      AU
X04240*/
/*    BAND STORAGE                                                      AU
X04250*/
/*                                                                      AU
X04260*/
/*          IF  A  IS A BAND MATRIX, THE FOLLOWING PROGRAM SEGMENT      AU
X04270*/
/*          WILL SET UP THE INPUT.                                      AU
X04280*/
/*                                                                      AU
X04290*/
/*                  ML = (BAND WIDTH BELOW THE DIAGONAL)                AU
X04300*/
/*                  MU = (BAND WIDTH ABOVE THE DIAGONAL)                AU
X04310*/
/*                  M = ML + MU + 1                                     AU
X04320*/
/*                  DO 20 J = 1, N                                      AU
X04330*/
/*                     I1 = MAX0(1, J-MU)                               AU
X04340*/
/*                     I2 = MIN0(N, J+ML)                               AU
X04350*/
/*                     DO 10 I = I1, I2                                 AU
X04360*/
/*                        K = I - J + M                                 AU
X04370*/
/*                        ABD(K,J) = A(I,J)                             AU
X04380*/
/*               10    CONTINUE                                         AU
X04390*/
/*               20 CONTINUE                                            AU
X04400*/
/*                                                                      AU
X04410*/
/*          THIS USES ROWS  ML+1  THROUGH  2*ML+MU+1  OF  ABD .         AU
X04420*/
/*          IN ADDITION, THE FIRST  ML  ROWS IN  ABD  ARE USED FOR      AU
X04430*/
/*          ELEMENTS GENERATED DURING THE TRIANGULARIZATION.            AU
X04440*/
/*          THE TOTAL NUMBER OF ROWS NEEDED IN  ABD  IS  2*ML+MU+1 .    AU
X04450*/
/*          THE  ML+MU BY ML+MU  UPPER LEFT TRIANGLE AND THE            AU
X04460*/
/*          ML BY ML  LOWER RIGHT TRIANGLE ARE NOT REFERENCED.          AU
X04470*/
/*                                                                      AU
X04480*/
/*    LINPACK. THIS VERSION DATED 08/14/78 .                            AU
X04490*/
/*    CLEVE MOLER, UNIVERSITY OF NEW MEXICO, ARGONNE NATIONAL LAB.      AU
X04500*/
/*                                                                      AU
X04510*/
/*    SUBROUTINES AND FUNCTIONS                                         AU
X04520*/
/*                                                                      AU
X04530*/
/*    BLAS DAXPY,DSCAL,IDAMAX                                           AU
X04540*/
/*    FORTRAN MAX0,MIN0                                                 AU
X04550*/
/*                                                                      AU
X04560*/
/*    INTERNAL VARIABLES                                                AU
X04570*/
/*                                                                      AU
X04580*/
/*                                                                      AU
X04610*/
/*                                                                      AU
X04620*/
    /* Parameter adjustments */
    --ipvt;
    abd_dim1 = *lda;
    abd_offset = abd_dim1 + 1;
    abd -= abd_offset;

    /* Function Body */
    m = *ml + *mu + 1;
    *info = 0;
/*                                                                      AU
X04650*/
/*    ZERO INITIAL FILL-IN COLUMNS                                      AU
X04660*/
/*                                                                      AU
X04670*/
    j0 = *mu + 2;
    j1 = min(*n,m) - 1;
    if (j1 < j0) {
	goto L30;
    }
    i_1 = j1;
    for (jz = j0; jz <= i_1; ++jz) {
	i0 = m + 1 - jz;
	i_2 = *ml;
	for (i = i0; i <= i_2; ++i) {
	    abd[i + jz * abd_dim1] = 0.;
/* L10: */
	}
/* L20: */
    }
L30:
    jz = j1;
    ju = 0;
/*                                                                      AU
X04800*/
/*    GAUSSIAN ELIMINATION WITH PARTIAL PIVOTING                        AU
X04810*/
/*                                                                      AU
X04820*/
    nm1 = *n - 1;
    if (nm1 < 1) {
	goto L130;
    }
    i_1 = nm1;
    for (k = 1; k <= i_1; ++k) {
	kp1 = k + 1;
/*                                                                    
  AUX04870*/
/*       ZERO NEXT FILL-IN COLUMN                                     
  AUX04880*/
/*                                                                    
  AUX04890*/
	++jz;
	if (jz > *n) {
	    goto L50;
	}
	if (*ml < 1) {
	    goto L50;
	}
	i_2 = *ml;
	for (i = 1; i <= i_2; ++i) {
	    abd[i + jz * abd_dim1] = 0.;
/* L40: */
	}
L50:
/*                                                                    
  AUX04970*/
/*       FIND L = PIVOT INDEX                                         
  AUX04980*/
/*                                                                    
  AUX04990*/
/* Computing MIN */
	i_2 = *ml, i_3 = *n - k;
	lm = min(i_2,i_3);
	i_2 = lm + 1;
	l = idamax_(&i_2, &abd[m + k * abd_dim1], &c__1) + m - 1;
	ipvt[k] = l + k - m;
/*                                                                    
  AUX05030*/
/*       ZERO PIVOT IMPLIES THIS COLUMN ALREADY TRIANGULARIZED        
  AUX05040*/
/*                                                                    
  AUX05050*/
	if (abd[l + k * abd_dim1] == 0.) {
	    goto L100;
	}
/*                                                                    
  AUX05070*/
/*          INTERCHANGE IF NECESSARY                                  
  AUX05080*/
/*                                                                    
  AUX05090*/
	if (l == m) {
	    goto L60;
	}
	t = abd[l + k * abd_dim1];
	abd[l + k * abd_dim1] = abd[m + k * abd_dim1];
	abd[m + k * abd_dim1] = t;
L60:
/*                                                                    
  AUX05150*/
/*          COMPUTE MULTIPLIERS                                       
  AUX05160*/
/*                                                                    
  AUX05170*/
	t = -1. / abd[m + k * abd_dim1];
	dscal_(&lm, &t, &abd[m + 1 + k * abd_dim1], &c__1);
/*                                                                    
  AUX05200*/
/*          ROW ELIMINATION WITH COLUMN INDEXING                      
  AUX05210*/
/*                                                                    
  AUX05220*/
/* Computing MIN */
/* Computing MAX */
	i_3 = ju, i_4 = *mu + ipvt[k];
	i_2 = max(i_3,i_4);
	ju = min(i_2,*n);
	mm = m;
	if (ju < kp1) {
	    goto L90;
	}
	i_2 = ju;
	for (j = kp1; j <= i_2; ++j) {
	    --l;
	    --mm;
	    t = abd[l + j * abd_dim1];
	    if (l == mm) {
		goto L70;
	    }
	    abd[l + j * abd_dim1] = abd[mm + j * abd_dim1];
	    abd[mm + j * abd_dim1] = t;
L70:
	    daxpy_(&lm, &t, &abd[m + 1 + k * abd_dim1], &c__1, &abd[mm + 1 + 
		    j * abd_dim1], &c__1);
/* L80: */
	}
L90:
	goto L110;
L100:
	*info = k;
L110:
/* L120: */
	;
    }
L130:
    ipvt[*n] = *n;
    if (abd[m + *n * abd_dim1] == 0.) {
	*info = *n;
    }
    return 0;
} /* dgbfa_ */

/* Subroutine */ int dgbsl_(abd, lda, n, ml, mu, ipvt, b, job)
doublereal *abd;
integer *lda, *n, *ml, *mu, *ipvt;
doublereal *b;
integer *job;
{
    /* System generated locals */
    integer abd_dim1, abd_offset, i_1, i_2, i_3;

    /* Local variables */
    extern doublereal ddot_();
    static integer k, l, m;
    static doublereal t;
    extern /* Subroutine */ int daxpy_();
    static integer kb, la, lb, lm, nm1;

/*                                                                      AU
X05500*/
/*    DGBSL SOLVES THE DOUBLE PRECISION BAND SYSTEM                     AU
X05510*/
/*    A * X = B  OR  TRANS(A) * X = B                                   AU
X05520*/
/*             CALL DGBSL(ABD,LDA,N,ML,MU,IPVT,C(1,J),0)                AU
X06010*/
/*       10 CONTINUE                                                    AU
X06020*/
/*                                                                      AU
X06030*/
/*    LINPACK. THIS VERSION DATED 08/14/78 .                            AU
X06040*/
/*    CLEVE MOLER, UNIVERSITY OF NEW MEXICO, ARGONNE NATIONAL LAB.      AU
X06050*/
/*                                                                      AU
X06060*/
/*    SUBROUTINES AND FUNCTIONS                                         AU
X06070*/
/*                                                                      AU
X06080*/
/*    BLAS DAXPY,DDOT                                                   AU
X06090*/
/*    FORTRAN MIN0                                                      AU
X06100*/
/*                                                                      AU
X06110*/
/*    INTERNAL VARIABLES                                                AU
X06120*/
/*                                                                      AU
X06130*/
/*                                                                      AU
X06160*/
    /* Parameter adjustments */
    --b;
    --ipvt;
    abd_dim1 = *lda;
    abd_offset = abd_dim1 + 1;
    abd -= abd_offset;

    /* Function Body */
    m = *mu + *ml + 1;
    nm1 = *n - 1;
    if (*job != 0) {
	goto L50;
    }
/*                                                                      AU
X06200*/
/*       JOB = 0 , SOLVE  A * X = B                                     AU
X06210*/
/*       FIRST SOLVE L*Y = B                                            AU
X06220*/
/*                                                                      AU
X06230*/
    if (*ml == 0) {
	goto L30;
    }
    if (nm1 < 1) {
	goto L30;
    }
    i_1 = nm1;
    for (k = 1; k <= i_1; ++k) {
/* Computing MIN */
	i_2 = *ml, i_3 = *n - k;
	lm = min(i_2,i_3);
	l = ipvt[k];
	t = b[l];
	if (l == k) {
	    goto L10;
	}
	b[l] = b[k];
	b[k] = t;
L10:
	daxpy_(&lm, &t, &abd[m + 1 + k * abd_dim1], &c__1, &b[k + 1], &c__1);
/* L20: */
    }
L30:
/*                                                                      AU
X06370*/
/*       NOW SOLVE  U*X = Y                                             AU
X06380*/
/*                                                                      AU
X06390*/
    i_1 = *n;
    for (kb = 1; kb <= i_1; ++kb) {
	k = *n + 1 - kb;
	b[k] /= abd[m + k * abd_dim1];
	lm = min(k,m) - 1;
	la = m - lm;
	lb = k - lm;
	t = -b[k];
	daxpy_(&lm, &t, &abd[la + k * abd_dim1], &c__1, &b[lb], &c__1);
/* L40: */
    }
    goto L100;
L50:
/*                                                                      AU
X06510*/
/*       JOB = NONZERO, SOLVE  TRANS(A) * X = B                         AU
X06520*/
/*       FIRST SOLVE  TRANS(U)*Y = B                                    AU
X06530*/
/*                                                                      AU
X06540*/
    i_1 = *n;
    for (k = 1; k <= i_1; ++k) {
	lm = min(k,m) - 1;
	la = m - lm;
	lb = k - lm;
	t = ddot_(&lm, &abd[la + k * abd_dim1], &c__1, &b[lb], &c__1);
	b[k] = (b[k] - t) / abd[m + k * abd_dim1];
/* L60: */
    }
/*                                                                      AU
X06620*/
/*       NOW SOLVE TRANS(L)*X = Y                                       AU
X06630*/
/*                                                                      AU
X06640*/
    if (*ml == 0) {
	goto L90;
    }
    if (nm1 < 1) {
	goto L90;
    }
    i_1 = nm1;
    for (kb = 1; kb <= i_1; ++kb) {
	k = *n - kb;
/* Computing MIN */
	i_2 = *ml, i_3 = *n - k;
	lm = min(i_2,i_3);
	b[k] += ddot_(&lm, &abd[m + 1 + k * abd_dim1], &c__1, &b[k + 1], &
		c__1);
	l = ipvt[k];
	if (l == k) {
	    goto L70;
	}
	t = b[l];
	b[l] = b[k];
	b[k] = t;
L70:
/* L80: */
	;
    }
L90:
L100:
    return 0;
} /* dgbsl_ */

/* Subroutine */ int dgefa_(a, lda, n, ipvt, info)
doublereal *a;
integer *lda, *n, *ipvt, *info;
{
    /* System generated locals */
    integer a_dim1, a_offset, i_1, i_2, i_3;

    /* Local variables */
    static integer j, k, l;
    static doublereal t;
    extern /* Subroutine */ int dscal_(), daxpy_();
    extern integer idamax_();
    static integer kp1, nm1;

/*                                                                      AU
X01560*/
/*    DGEFA FACTORS A DOUBLE PRECISION MATRIX BY GAUSSIAN ELIMINATION.  AU
X01570*/
/*                                                                      AU
X01580*/
/*    DGEFA IS USUALLY CALLED BY DGECO, BUT IT CAN BE CALLED            AU
X01590*/
/*    DIRECTLY WITH A SAVING IN TIME IF  RCOND  IS NOT NEEDED.          AU
X01600*/
/*    (TIME FOR DGECO) = (1 + 9/N)*(TIME FOR DGEFA) .                   AU
X01610*/
/*                                                                      AU
X01620*/
/*    ON ENTRY                                                          AU
X01630*/
/*                                                                      AU
X01640*/
/*       A       DOUBLE PRECISION(LDA, N)                               AU
X01650*/
/*               THE MATRIX TO BE FACTORED.                             AU
X01660*/
/*                                                                      AU
X01670*/
/*       LDA     INTEGER                                                AU
X01680*/
/*               THE LEADING DIMENSION OF THE ARRAY  A .                AU
X01690*/
/*                                                                      AU
X01700*/
/*       N       INTEGER                                                AU
X01710*/
/*               THE ORDER OF THE MATRIX  A .                           AU
X01720*/
/*                                                                      AU
X01730*/
/*    ON RETURN                                                         AU
X01740*/
/*                                                                      AU
X01750*/
/*       A       AN UPPER TRIANGULAR MATRIX AND THE MULTIPLIERS         AU
X01760*/
/*               WHICH WERE USED TO OBTAIN IT.                          AU
X01770*/
/*               THE FACTORIZATION CAN BE WRITTEN  A = L*U  WHERE       AU
X01780*/
/*               L  IS A PRODUCT OF PERMUTATION AND UNIT LOWER          AU
X01790*/
/*               TRIANGULAR MATRICES AND  U  IS UPPER TRIANGULAR.       AU
X01800*/
/*                                                                      AU
X01810*/
/*       IPVT    INTEGER(N)                                             AU
X01820*/
/*               AN INTEGER VECTOR OF PIVOT INDICES.                    AU
X01830*/
/*                                                                      AU
X01840*/
/*       INFO    INTEGER                                                AU
X01850*/
/*               = 0  NORMAL VALUE.                                     AU
X01860*/
/*               = K  IF  U(K,K) .EQ. 0.0 .  THIS IS NOT AN ERROR       AU
X01870*/
/*                    CONDITION FOR THIS SUBROUTINE, BUT IT DOES        AU
X01880*/
/*                    INDICATE THAT DGESL OR DGEDI WILL DIVIDE BY ZERO  AU
X01890*/
/*                    IF CALLED.  USE  RCOND  IN DGECO FOR A RELIABLE   AU
X01900*/
/*                    INDICATION OF SINGULARITY.                        AU
X01910*/
/*                                                                      AU
X01920*/
/*    LINPACK. THIS VERSION DATED 08/14/78 .                            AU
X01930*/
/*    CLEVE MOLER, UNIVERSITY OF NEW MEXICO, ARGONNE NATIONAL LAB.      AU
X01940*/
/*                                                                      AU
X01950*/
/*    SUBROUTINES AND FUNCTIONS                                         AU
X01960*/
/*                                                                      AU
X01970*/
/*    BLAS DAXPY,DSCAL,IDAMAX                                           AU
X01980*/
/*                                                                      AU
X01990*/
/*    INTERNAL VARIABLES                                                AU
X02000*/
/*                                                                      AU
X02010*/
/*                                                                      AU
X02040*/
/*                                                                      AU
X02050*/
/*    GAUSSIAN ELIMINATION WITH PARTIAL PIVOTING                        AU
X02060*/
/*                                                                      AU
X02070*/
    /* Parameter adjustments */
    --ipvt;
    a_dim1 = *lda;
    a_offset = a_dim1 + 1;
    a -= a_offset;

    /* Function Body */
    *info = 0;
    nm1 = *n - 1;
    if (nm1 < 1) {
	goto L70;
    }
    i_1 = nm1;
    for (k = 1; k <= i_1; ++k) {
	kp1 = k + 1;
/*                                                                    
  AUX02130*/
/*       FIND L = PIVOT INDEX                                         
  AUX02140*/
/*                                                                    
  AUX02150*/
	i_2 = *n - k + 1;
	l = idamax_(&i_2, &a[k + k * a_dim1], &c__1) + k - 1;
	ipvt[k] = l;
/*                                                                    
  AUX02180*/
/*       ZERO PIVOT IMPLIES THIS COLUMN ALREADY TRIANGULARIZED        
  AUX02190*/
/*                                                                    
  AUX02200*/
	if (a[l + k * a_dim1] == 0.) {
	    goto L40;
	}
/*                                                                    
  AUX02220*/
/*          INTERCHANGE IF NECESSARY                                  
  AUX02230*/
/*                                                                    
  AUX02240*/
	if (l == k) {
	    goto L10;
	}
	t = a[l + k * a_dim1];
	a[l + k * a_dim1] = a[k + k * a_dim1];
	a[k + k * a_dim1] = t;
L10:
/*                                                                    
  AUX02300*/
/*          COMPUTE MULTIPLIERS                                       
  AUX02310*/
/*                                                                    
  AUX02320*/
	t = -1. / a[k + k * a_dim1];
	i_2 = *n - k;
	dscal_(&i_2, &t, &a[k + 1 + k * a_dim1], &c__1);
/*                                                                    
  AUX02350*/
/*          ROW ELIMINATION WITH COLUMN INDEXING                      
  AUX02360*/
/*                                                                    
  AUX02370*/
	i_2 = *n;
	for (j = kp1; j <= i_2; ++j) {
	    t = a[l + j * a_dim1];
	    if (l == k) {
		goto L20;
	    }
	    a[l + j * a_dim1] = a[k + j * a_dim1];
	    a[k + j * a_dim1] = t;
L20:
	    i_3 = *n - k;
	    daxpy_(&i_3, &t, &a[k + 1 + k * a_dim1], &c__1, &a[k + 1 + j * 
		    a_dim1], &c__1);
/* L30: */
	}
	goto L50;
L40:
	*info = k;
L50:
/* L60: */
	;
    }
L70:
    ipvt[*n] = *n;
    if (a[*n + *n * a_dim1] == 0.) {
	*info = *n;
    }
    return 0;
} /* dgefa_ */

/* Subroutine */ int dgesl_(a, lda, n, ipvt, b, job)
doublereal *a;
integer *lda, *n, *ipvt;
doublereal *b;
integer *job;
{
    /* System generated locals */
    integer a_dim1, a_offset, i_1, i_2;

    /* Local variables */
    extern doublereal ddot_();
    static integer k, l;
    static doublereal t;
    extern /* Subroutine */ int daxpy_();
    static integer kb, nm1;

/*                                                                      AU
X02590*/
/*    DGESL SOLVES THE DOUBLE PRECISION SYSTEM                          AU
X02600*/
/*    A * X = B  OR  TRANS(A) * X = B                                   AU
X02610*/
/*    SUBROUTINES AND FUNCTIONS                                         AU
X03100*/
/*                                                                      AU
X03110*/
/*    BLAS DAXPY,DDOT                                                   AU
X03120*/
/*                                                                      AU
X03130*/
/*    INTERNAL VARIABLES                                                AU
X03140*/
/*                                                                      AU
X03150*/
/*                                                                      AU
X03180*/
    /* Parameter adjustments */
    --b;
    --ipvt;
    a_dim1 = *lda;
    a_offset = a_dim1 + 1;
    a -= a_offset;

    /* Function Body */
    nm1 = *n - 1;
    if (*job != 0) {
	goto L50;
    }
/*                                                                      AU
X03210*/
/*       JOB = 0 , SOLVE  A * X = B                                     AU
X03220*/
/*       FIRST SOLVE  L*Y = B                                           AU
X03230*/
/*                                                                      AU
X03240*/
    if (nm1 < 1) {
	goto L30;
    }
    i_1 = nm1;
    for (k = 1; k <= i_1; ++k) {
	l = ipvt[k];
	t = b[l];
	if (l == k) {
	    goto L10;
	}
	b[l] = b[k];
	b[k] = t;
L10:
	i_2 = *n - k;
	daxpy_(&i_2, &t, &a[k + 1 + k * a_dim1], &c__1, &b[k + 1], &c__1);
/* L20: */
    }
L30:
/*                                                                      AU
X03360*/
/*       NOW SOLVE  U*X = Y                                             AU
X03370*/
/*                                                                      AU
X03380*/
    i_1 = *n;
    for (kb = 1; kb <= i_1; ++kb) {
	k = *n + 1 - kb;
	b[k] /= a[k + k * a_dim1];
	t = -b[k];
	i_2 = k - 1;
	daxpy_(&i_2, &t, &a[k * a_dim1 + 1], &c__1, &b[1], &c__1);
/* L40: */
    }
    goto L100;
L50:
/*                                                                      AU
X03470*/
/*       JOB = NONZERO, SOLVE  TRANS(A) * X = B                         AU
X03480*/
/*       FIRST SOLVE  TRANS(U)*Y = B                                    AU
X03490*/
/*                                                                      AU
X03500*/
    i_1 = *n;
    for (k = 1; k <= i_1; ++k) {
	i_2 = k - 1;
	t = ddot_(&i_2, &a[k * a_dim1 + 1], &c__1, &b[1], &c__1);
	b[k] = (b[k] - t) / a[k + k * a_dim1];
/* L60: */
    }
/*                                                                      AU
X03550*/
/*       NOW SOLVE TRANS(L)*X = Y                                       AU
X03560*/
/*                                                                      AU
X03570*/
    if (nm1 < 1) {
	goto L90;
    }
    i_1 = nm1;
    for (kb = 1; kb <= i_1; ++kb) {
	k = *n - kb;
	i_2 = *n - k;
	b[k] += ddot_(&i_2, &a[k + 1 + k * a_dim1], &c__1, &b[k + 1], &c__1);
	l = ipvt[k];
	if (l == k) {
	    goto L70;
	}
	t = b[l];
	b[l] = b[k];
	b[k] = t;
L70:
/* L80: */
	;
    }
L90:
L100:
    return 0;
} /* dgesl_ */

/* Subroutine */ int dscal_(n, da, dx, incx)
integer *n;
doublereal *da, *dx;
integer *incx;
{
    /* System generated locals */
    integer i_1, i_2;

    /* Local variables */
    static integer i, m, nincx, mp1;

/*                                                                      AU
X07300*/
/*    SCALES A VECTOR BY A CONSTANT.                                    AU
X07310*/
/*    USES UNROLLED LOOPS FOR INCREMENT EQUAL TO ONE.                   AU
X07320*/
/*    JACK DONGARRA, LINPACK, 3/11/78.                                  AU
X07330*/
/*                                                                      AU
X07340*/
/*                                                                      AU
X07370*/
    /* Parameter adjustments */
    --dx;

    /* Function Body */
    if (*n <= 0) {
	return 0;
    }
    if (*incx == 1) {
	goto L20;
    }
/*                                                                      AU
X07400*/
/*       CODE FOR INCREMENT NOT EQUAL TO 1                              AU
X07410*/
/*                                                                      AU
X07420*/
    nincx = *n * *incx;
    i_1 = nincx;
    i_2 = *incx;
    for (i = 1; i_2 < 0 ? i >= i_1 : i <= i_1; i += i_2) {
	dx[i] = *da * dx[i];
/* L10: */
    }
    return 0;
/*                                                                      AU
X07480*/
/*       CODE FOR INCREMENT EQUAL TO 1                                  AU
X07490*/
/*                                                                      AU
X07500*/
/*                                                                      AU
X07510*/
/*       CLEAN-UP LOOP                                                  AU
X07520*/
/*                                                                      AU
X07530*/
L20:
    m = *n % 5;
    if (m == 0) {
	goto L40;
    }
    i_2 = m;
    for (i = 1; i <= i_2; ++i) {
	dx[i] = *da * dx[i];
/* L30: */
    }
    if (*n < 5) {
	return 0;
    }
L40:
    mp1 = m + 1;
    i_2 = *n;
    for (i = mp1; i <= i_2; i += 5) {
	dx[i] = *da * dx[i];
	dx[i + 1] = *da * dx[i + 1];
	dx[i + 2] = *da * dx[i + 2];
	dx[i + 3] = *da * dx[i + 3];
	dx[i + 4] = *da * dx[i + 4];
/* L50: */
    }
    return 0;
} /* dscal_ */

integer idamax_(n, dx, incx)
integer *n;
doublereal *dx;
integer *incx;
{
    /* System generated locals */
    integer ret_val, i_1;
    doublereal d_1;

    /* Local variables */
    static doublereal dmax_;
    static integer i, ix;

/*                                                                      AU
X08190*/
/*    FINDS THE INDEX OF ELEMENT HAVING MAX. ABSOLUTE VALUE.            AU
X08200*/
/*    JACK DONGARRA, LINPACK, 3/11/78.                                  AU
X08210*/
/*                                                                      AU
X08220*/
/*                                                                      AU
X08250*/
    /* Parameter adjustments */
    --dx;

    /* Function Body */
    ret_val = 0;
    if (*n < 1) {
	return ret_val;
    }
    ret_val = 1;
    if (*n == 1) {
	return ret_val;
    }
    if (*incx == 1) {
	goto L20;
    }
/*                                                                      AU
X08310*/
/*       CODE FOR INCREMENT NOT EQUAL TO 1                              AU
X08320*/
/*                                                                      AU
X08330*/
    ix = 1;
    dmax_ = abs(dx[1]);
    ix += *incx;
    i_1 = *n;
    for (i = 2; i <= i_1; ++i) {
	if ((d_1 = dx[ix], abs(d_1)) <= dmax_) {
	    goto L5;
	}
	ret_val = i;
	dmax_ = (d_1 = dx[ix], abs(d_1));
L5:
	ix += *incx;
/* L10: */
    }
    return ret_val;
/*                                                                      AU
X08440*/
/*       CODE FOR INCREMENT EQUAL TO 1                                  AU
X08450*/
/*                                                                      AU
X08460*/
L20:
    dmax_ = abs(dx[1]);
    i_1 = *n;
    for (i = 2; i <= i_1; ++i) {
	if ((d_1 = dx[i], abs(d_1)) <= dmax_) {
	    goto L30;
	}
	ret_val = i;
	dmax_ = (d_1 = dx[i], abs(d_1));
L30:
	;
    }
    return ret_val;
} /* idamax_ */

/* Subroutine */ int lsoda_(f, neq, y, t, tout, itol, rtol, atol, itask, 
	istate, iopt, rwork, lrw, iwork, liw, jac, jt)
/* Subroutine */ int (*f) ();
integer *neq;
doublereal *y, *t, *tout;
integer *itol;
doublereal *rtol, *atol;
integer *itask, *istate, *iopt;
doublereal *rwork;
integer *lrw, *iwork, *liw;
/* Subroutine */ int (*jac) ();
integer *jt;
{
    /* Initialized data */

    static integer mord[2] = { 12,5 };
    static integer mxstp0 = 500;
    static integer mxhnl0 = 10;

    /* System generated locals */
    integer i_1;
    doublereal d_1, d_2;

    /* Builtin functions */
    double sqrt(), d_sign();

    /* Local variables */
    extern /* Subroutine */ int prja_();
    static doublereal hmax;
    static logical ihit;
    static doublereal ewti, size;
    static integer len1c, len1n, len1s, i, iflag;
    static doublereal atoli;
    static integer leniw, lenwm;
    extern /* Subroutine */ int stoda_();
    static integer imxer;
    static doublereal tcrit;
    static integer lenrw;
    static doublereal h0;
    static integer i1, i2;
    static doublereal rtoli, tdist, tolsf;
    extern doublereal d1mach_();
    static doublereal tnext;
    extern /* Subroutine */ int ewset_(), intdy_();
    static doublereal w0;
    extern /* Subroutine */ int solsy_();
    static integer ml;
    static doublereal rh;
    static integer mu;
    static doublereal tp;
    static integer leniwc, lenrwc, lf0, lenrwn, lenrws;
    extern doublereal vmnorm_();
    extern /* Subroutine */ int xerrwv_();
    static doublereal big;
    static integer kgo;
    static doublereal ayi, hmx, tol, sum;
    static integer len1, len2;

/* -----------------------------------------------------------------------
 */
/* this is the march 30, 1987 version of */
/* lsoda.. livermore solver for ordinary differential equations, with */
/*         automatic method switching for stiff and nonstiff problems. */
/* jac    = name of subroutine for jacobian matrix. */
/*          use a dummy name.  see also paragraph e below. */
/* jt     = jacobian type indicator.  set jt = 2. */
/*          see also paragraph e below. */
/* note that the main program must declare arrays y, rwork, iwork, */
/* and possibly atol. */

/* c. the output from the first call (or any call) is.. */
/*      y = array of computed values of y(t) vector. */
/*      t = corresponding value of independent variable (normally tout). 
*/
/* istate = 2  if lsoda was successful, negative otherwise. */
/*          -1 means excess work done on this call (perhaps wrong jt). */
/*          -2 means excess accuracy requested (tolerances too small). */
/*          -3 means illegal input detected (see printed message). */
/*          -4 means repeated error test failures (check all inputs). */
/*          -5 means repeated convergence failures (perhaps bad jacobian 
*/
/*             supplied or wrong choice of jt or tolerances). */
/*          -6 means error weight became zero during problem. (solution */

/*             component i vanished, and atol or atol(i) = 0.) */
/*          -7 means work space insufficient to finish (see messages). */

/* d. to continue the integration after a successful return, simply */
/* reset tout and call lsoda again.  no other parameters need be reset. */


/* e. note.. if and when lsoda regards the problem as stiff, and */
/* switches methods accordingly, it must make use of the neq by neq */
/* jacobian matrix, j = df/dy.  for the sake of simplicity, the */
/* inputs to lsoda recommended in paragraph b above cause lsoda to */
/* treat j as a full matrix, and to approximate it internally by */
/* difference quotients.  alternatively, j can be treated as a band */
/* matrix (with great potential reduction in the size of the rwork */
/* array).  also, in either the full or banded case, the user can supply 
*/
/* j in closed form, with a routine whose name is passed as the jac */
/* argument.  these alternatives are described in the paragraphs on */
/* rwork, jac, and jt in the full description of the call sequence below. 
*/

/* -----------------------------------------------------------------------
 */
/* example problem. */

/* the following is a simple example problem, with the coding */
/* needed for its solution by lsoda.  the problem is from chemical */
/* kinetics, and consists of the following three rate equations.. */
/*     dy1/dt = -.04*y1 + 1.e4*y2*y3 */
/*     dy2/dt = .04*y1 - 1.e4*y2*y3 - 3.e7*y2**2 */
/*     dy3/dt = 3.e7*y2**2 */
/* on the interval from t = 0.0 to t = 4.e10, with initial conditions */
/* y1 = 1.0, y2 = y3 = 0.  the problem is stiff. */

/* the following coding solves this problem with lsoda, */
/* printing results at t = .4, 4., ..., 4.e10.  it uses */
/* itol = 2 and atol much smaller for y2 than y1 or y3 because */
/* y2 has much smaller values. */
/* at the end of the run, statistical quantities of interest are */
/* printed (see optional outputs in the full description below). */

/*     external fex */
/*     double precision atol, rtol, rwork, t, tout, y */
/*     dimension y(3), atol(3), rwork(70), iwork(23) */
/*     neq = 3 */
/*     y(1) = 1.0d0 */
/*     y(2) = 0.0d0 */
/*     y(3) = 0.0d0 */
/*     t = 0.0d0 */
/*     tout = 0.4d0 */
/*     itol = 2 */
/*     rtol = 1.0d-4 */
/*     atol(1) = 1.0d-6 */
/*     atol(2) = 1.0d-10 */
/*     atol(3) = 1.0d-6 */
/*     itask = 1 */
/*     istate = 1 */
/*     iopt = 0 */
/*     lrw = 70 */
/*     liw = 23 */
/*     jt = 2 */
/*     do 40 iout = 1,12 */
/*       call lsoda(fex,neq,y,t,tout,itol,rtol,atol,itask,istate, */
/*    1     iopt,rwork,lrw,iwork,liw,jdum,jt) */
/*       write(6,20)t,y(1),y(2),y(3) */
/* 20    format(7h at t =,e12.4,6h   y =,3e14.6) */
/*       if (istate .lt. 0) go to 80 */
/* 40    tout = tout*10.0d0 */
/*     write(6,60)iwork(11),iwork(12),iwork(13),iwork(19),rwork(15) */
/* 60  format(/12h no. steps =,i4,11h  no. f-s =,i4,11h  no. j-s =,i4/ */
/*    1   19h method last used =,i2,25h   last switch was at t =,e12.4) */

/*     stop */
/* 80  write(6,90)istate */
/* 90  format(///22h error halt.. istate =,i3) */
/*     stop */
/*     end */

/*     subroutine fex (neq, t, y, ydot) */
/*     double precision t, y, ydot */
/*     dimension y(3), ydot(3) */
/*     ydot(1) = -.04d0*y(1) + 1.0d4*y(2)*y(3) */
/*     ydot(3) = 3.0d7*y(2)*y(2) */
/*     ydot(2) = -ydot(1) - ydot(3) */
/*     return */
/*     end */

/* the output of this program (on a cdc-7600 in single precision) */
/* is as follows.. */

/*   at t =  4.0000e-01   y =  9.851712e-01  3.386380e-05  1.479493e-02 */

/*   at t =  4.0000e+00   y =  9.055333e-01  2.240655e-05  9.444430e-02 */

/*   at t =  4.0000e+01   y =  7.158403e-01  9.186334e-06  2.841505e-01 */

/*   at t =  4.0000e+02   y =  4.505250e-01  3.222964e-06  5.494717e-01 */

/*   at t =  4.0000e+03   y =  1.831975e-01  8.941774e-07  8.168016e-01 */

/*   at t =  4.0000e+04   y =  3.898730e-02  1.621940e-07  9.610125e-01 */

/*   at t =  4.0000e+05   y =  4.936363e-03  1.984221e-08  9.950636e-01 */

/*   at t =  4.0000e+06   y =  5.161831e-04  2.065786e-09  9.994838e-01 */

/*   at t =  4.0000e+07   y =  5.179817e-05  2.072032e-10  9.999482e-01 */

/*   at t =  4.0000e+08   y =  5.283401e-06  2.113371e-11  9.999947e-01 */

/*   at t =  4.0000e+09   y =  4.659031e-07  1.863613e-12  9.999995e-01 */

/*   at t =  4.0000e+10   y =  1.404280e-08  5.617126e-14  1.000000e+00 */


/*   no. steps = 361  no. f-s = 693  no. j-s =  64 */
/*   method last used = 2   last switch was at t =  6.0092e-03 */
/* -----------------------------------------------------------------------
 */
/* full description of user interface to lsoda. */

/* the user interface to lsoda consists of the following parts. */

/* i.   the call sequence to subroutine lsoda, which is a driver */
/*      routine for the solver.  this includes descriptions of both */
/*      the call sequence arguments and of user-supplied routines. */
/*      following these descriptions is a description of */
/*      optional inputs available through the call sequence, and then */
/*      a description of optional outputs (in the work arrays). */

/* ii.  descriptions of other routines in the lsoda package that may be */

/*      (optionally) called by the user.  these provide the ability to */
/*      alter error message handling, save and restore the internal */
/*      common, and obtain specified derivatives of the solution y(t). */

/* iii. descriptions of common blocks to be declared in overlay */
/*      or similar environments, or to be saved when doing an interrupt */

/*      of the problem and continued solution later. */

/* iv.  description of a subroutine in the lsoda package, */
/*      which the user may replace with his own version, if desired. */
/*      this relates to the measurement of errors. */

/* -----------------------------------------------------------------------
 */
/* part i.  call sequence. */

/* the call sequence parameters used for input only are */
/*     f, neq, tout, itol, rtol, atol, itask, iopt, lrw, liw, jac, jt, */
/* and those used for both input and output are */
/*     y, t, istate. */
/* the work arrays rwork and iwork are also used for conditional and */
/* optional inputs and optional outputs.  (the term output here refers */
/* to the return from subroutine lsoda to the user-s calling program.) */

/* the legality of input parameters will be thoroughly checked on the */
/* initial call for the problem, but not checked thereafter unless a */
/* change in input parameters is flagged by istate = 3 on input. */

/* the descriptions of the call arguments are as follows. */

/* f      = the name of the user-supplied subroutine defining the */
/*          ode system.  the system must be put in the first-order */
/*          form dy/dt = f(t,y), where f is a vector-valued function */
/*          of the scalar t and the vector y.  subroutine f is to */
/*          compute the function f.  it is to have the form */
/*               subroutine f (neq, t, y, ydot) */
/*               dimension y(1), ydot(1) */
/*          where neq, t, and y are input, and the array ydot = f(t,y) */
/*          is output.  y and ydot are arrays of length neq. */
/*          (in the dimension statement above, 1 is a dummy */
/*          dimension.. it can be replaced by any value.) */
/*          subroutine f should not alter y(1),...,y(neq). */
/*          f must be declared external in the calling program. */

/*          subroutine f may access user-defined quantities in */
/*          neq(2),... and/or in y(neq(1)+1),... if neq is an array */
/*          (dimensioned in f) and/or y has length exceeding neq(1). */
/*          see the descriptions of neq and y below. */

/*          if quantities computed in the f routine are needed */
/*          externally to lsoda, an extra call to f should be made */
/*          for this purpose, for consistent and accurate results. */
/*          if only the derivative dy/dt is needed, use intdy instead. */

/* neq    = the size of the ode system (number of first order */
/*          ordinary differential equations).  used only for input. */
/*          neq may be decreased, but not increased, during the problem. 
*/
/*          if neq is decreased (with istate = 3 on input), the */
/*          remaining components of y should be left undisturbed, if */
/*          these are to be accessed in f and/or jac. */

/*          normally, neq is a scalar, and it is generally referred to */
/*          as a scalar in this user interface description.  however, */
/*          neq may be an array, with neq(1) set to the system size. */
/*          (the lsoda package accesses only neq(1).)  in either case, */
/*          this parameter is passed as the neq argument in all calls */
/*          to f and jac.  hence, if it is an array, locations */
/*          neq(2),... may be used to store other integer data and pass */

/*          it to f and/or jac.  subroutines f and/or jac must include */
/*          neq in a dimension statement in that case. */

/* y      = a real array for the vector of dependent variables, of */
/*          length neq or more.  used for both input and output on the */
/*          first call (istate = 1), and only for output on other calls. 
*/
/*          on the first call, y must contain the vector of initial */
/*          values.  on output, y contains the computed solution vector, 
*/
/*          evaluated at t.  if desired, the y array may be used */
/*          for other purposes between calls to the solver. */

/*          this array is passed as the y argument in all calls to */
/*          f and jac.  hence its length may exceed neq, and locations */
/*          y(neq+1),... may be used to store other real data and */
/*          pass it to f and/or jac.  (the lsoda package accesses only */
/*          y(1),...,y(neq).) */

/* t      = the independent variable.  on input, t is used only on the */
/*          first call, as the initial point of the integration. */
/*          on output, after each call, t is the value at which a */
/*          computed solution y is evaluated (usually the same as tout). 
*/
/*          on an error return, t is the farthest point reached. */

/* tout   = the next value of t at which a computed solution is desired. 
*/
/*          used only for input. */

/*          when starting the problem (istate = 1), tout may be equal */
/*          to t for one call, then should .ne. t for the next call. */
/*          for the initial t, an input value of tout .ne. t is used */
/*          in order to determine the direction of the integration */
/*          (i.e. the algebraic sign of the step sizes) and the rough */
/*          scale of the problem.  integration in either direction */
/*          (forward or backward in t) is permitted. */

/*          if itask = 2 or 5 (one-step modes), tout is ignored after */
/*          the first call (i.e. the first call with tout .ne. t). */
/*          otherwise, tout is required on every call. */

/*          if itask = 1, 3, or 4, the values of tout need not be */
/*          monotone, but a value of tout which backs up is limited */
/*          to the current internal t interval, whose endpoints are */
/*          tcur - hu and tcur (see optional outputs, below, for */
/*          tcur and hu). */

/* itol   = an indicator for the type of error control.  see */
/*          description below under atol.  used only for input. */

/* rtol   = a relative error tolerance parameter, either a scalar or */
/*          an array of length neq.  see description below under atol. */
/*          input only. */

/* atol   = an absolute error tolerance parameter, either a scalar or */
/*          an array of length neq.  input only. */

/*             the input parameters itol, rtol, and atol determine */
/*          the error control performed by the solver.  the solver will */

/*          control the vector e = (e(i)) of estimated local errors */
/*          in y, according to an inequality of the form */
/*                      max-norm of ( e(i)/ewt(i) )   .le.   1, */
/*          where ewt = (ewt(i)) is a vector of positive error weights. */

/*          the values of rtol and atol should all be non-negative. */
/*          the following table gives the types (scalar/array) of */
/*          rtol and atol, and the corresponding form of ewt(i). */

/*             itol    rtol       atol          ewt(i) */
/*              1     scalar     scalar     rtol*abs(y(i)) + atol */
/*              2     scalar     array      rtol*abs(y(i)) + atol(i) */
/*              3     array      scalar     rtol(i)*abs(y(i)) + atol */
/*              4     array      array      rtol(i)*abs(y(i)) + atol(i) */


/*          when either of these parameters is a scalar, it need not */
/*          be dimensioned in the user-s calling program. */

/*          if none of the above choices (with itol, rtol, and atol */
/*          fixed throughout the problem) is suitable, more general */
/*          error controls can be obtained by substituting a */
/*          user-supplied routine for the setting of ewt. */
/*          see part iv below. */

/*          if global errors are to be estimated by making a repeated */
/*          run on the same problem with smaller tolerances, then all */
/*          components of rtol and atol (i.e. of ewt) should be scaled */
/*          down uniformly. */

/* itask  = an index specifying the task to be performed. */
/*          input only.  itask has the following values and meanings. */
/*          1  means normal computation of output values of y(t) at */
/*             t = tout (by overshooting and interpolating). */
/*          2  means take one step only and return. */
/*          3  means stop at the first internal mesh point at or */
/*             beyond t = tout and return. */
/*          4  means normal computation of output values of y(t) at */
/*             t = tout but without overshooting t = tcrit. */
/*             tcrit must be input as rwork(1).  tcrit may be equal to */
/*             or beyond tout, but not behind it in the direction of */
/*             integration.  this option is useful if the problem */
/*             has a singularity at or beyond t = tcrit. */
/*          5  means take one step, without passing tcrit, and return. */
/*             tcrit must be input as rwork(1). */

/*          note..  if itask = 4 or 5 and the solver reaches tcrit */
/*          (within roundoff), it will return t = tcrit (exactly) to */
/*          indicate this (unless itask = 4 and tout comes before tcrit, 
*/
/*          in which case answers at t = tout are returned first). */

/* istate = an index used for input and output to specify the */
/*          the state of the calculation. */

/*          on input, the values of istate are as follows. */
/*          1  means this is the first call for the problem */
/*             (initializations will be done).  see note below. */
/*          2  means this is not the first call, and the calculation */
/*             is to continue normally, with no change in any input */
/*             parameters except possibly tout and itask. */
/*             (if itol, rtol, and/or atol are changed between calls */
/*             with istate = 2, the new values will be used but not */
/*             tested for legality.) */
/*          3  means this is not the first call, and the */
/*             calculation is to continue normally, but with */
/*             a change in input parameters other than */
/*             tout and itask.  changes are allowed in */
/*             neq, itol, rtol, atol, iopt, lrw, liw, jt, ml, mu, */
/*             and any optional inputs except h0, mxordn, and mxords. */
/*             (see iwork description for ml and mu.) */
/*          note..  a preliminary call with tout = t is not counted */
/*          as a first call here, as no initialization or checking of */
/*          input is done.  (such a call is sometimes useful for the */
/*          purpose of outputting the initial conditions.) */
/*          thus the first call for which tout .ne. t requires */
/*          istate = 1 on input. */

/*          on output, istate has the following values and meanings. */
/*           1  means nothing was done, as tout was equal to t with */
/*              istate = 1 on input.  (however, an internal counter was */

/*              set to detect and prevent repeated calls of this type.) */

/*           2  means the integration was performed successfully. */
/*          -1  means an excessive amount of work (more than mxstep */
/*              steps) was done on this call, before completing the */
/*              requested task, but the integration was otherwise */
/*              successful as far as t.  (mxstep is an optional input */
/*              and is normally 500.)  to continue, the user may */
/*              simply reset istate to a value .gt. 1 and call again */
/*              (the excess work step counter will be reset to 0). */
/*              in addition, the user may increase mxstep to avoid */
/*              this error return (see below on optional inputs). */
/*          -2  means too much accuracy was requested for the precision */

/*              of the machine being used.  this was detected before */
/*              completing the requested task, but the integration */
/*              was successful as far as t.  to continue, the tolerance */

/*              parameters must be reset, and istate must be set */
/*              to 3.  the optional output tolsf may be used for this */
/*              purpose.  (note.. if this condition is detected before */
/*              taking any steps, then an illegal input return */
/*              (istate = -3) occurs instead.) */
/*          -3  means illegal input was detected, before taking any */
/*              integration steps.  see written message for details. */
/*              note..  if the solver detects an infinite loop of calls */

/*              to the solver with illegal input, it will cause */
/*              the run to stop. */
/*          -4  means there were repeated error test failures on */
/*              one attempted step, before completing the requested */
/*              task, but the integration was successful as far as t. */
/*              the problem may have a singularity, or the input */
/*              may be inappropriate. */
/*          -5  means there were repeated convergence test failures on */
/*              one attempted step, before completing the requested */
/*              task, but the integration was successful as far as t. */
/*              this may be caused by an inaccurate jacobian matrix, */
/*              if one is being used. */
/*          -6  means ewt(i) became zero for some i during the */
/*              integration.  pure relative error control (atol(i)=0.0) */

/*              was requested on a variable which has now vanished. */
/*              the integration was successful as far as t. */
/*          -7  means the length of rwork and/or iwork was too small to */

/*              proceed, but the integration was successful as far as t. 
*/
/*              this happens when lsoda chooses to switch methods */
/*              but lrw and/or liw is too small for the new method. */

/*          note..  since the normal output value of istate is 2, */
/*          it does not need to be reset for normal continuation. */
/*          also, since a negative input value of istate will be */
/*          regarded as illegal, a negative output value requires the */
/*          user to change it, and possibly other inputs, before */
/*          calling the solver again. */

/* iopt   = an integer flag to specify whether or not any optional */
/*          inputs are being used on this call.  input only. */
/*          the optional inputs are listed separately below. */
/*          iopt = 0 means no optional inputs are being used. */
/*                   default values will be used in all cases. */
/*          iopt = 1 means one or more optional inputs are being used. */

/* rwork  = a real array (double precision) for work space, and (in the */

/*          first 20 words) for conditional and optional inputs and */
/*          optional outputs. */
/*          as lsoda switches automatically between stiff and nonstiff */
/*          methods, the required length of rwork can change during the */

/*          problem.  thus the rwork array passed to lsoda can either */
/*          have a static (fixed) length large enough for both methods, */

/*          or have a dynamic (changing) length altered by the calling */
/*          program in response to output from lsoda. */

/*                       --- fixed length case --- */
/*          if the rwork length is to be fixed, it should be at least */
/*               max (lrn, lrs), */
/*          where lrn and lrs are the rwork lengths required when the */
/*          current method is nonstiff or stiff, respectively. */

/*          the separate rwork length requirements lrn and lrs are */
/*          as follows.. */
/*          if neq is constant and the maximum method orders have */
/*          their default values, then */
/*             lrn = 20 + 16*neq, */
/*             lrs = 22 + 9*neq + neq**2           if jt = 1 or 2, */
/*             lrs = 22 + 10*neq + (2*ml+mu)*neq   if jt = 4 or 5. */
/*          under any other conditions, lrn and lrs are given by.. */
/*             lrn = 20 + nyh*(mxordn+1) + 3*neq, */
/*             lrs = 20 + nyh*(mxords+1) + 3*neq + lmat, */
/*          where */
/*             nyh    = the initial value of neq, */
/*             mxordn = 12, unless a smaller value is given as an */
/*                      optional input, */
/*             mxords = 5, unless a smaller value is given as an */
/*                      optional input, */
/*             lmat   = length of matrix work space.. */
/*             lmat   = neq**2 + 2              if jt = 1 or 2, */
/*             lmat   = (2*ml + mu + 1)*neq + 2 if jt = 4 or 5. */

/*                       --- dynamic length case --- */
/*          if the length of rwork is to be dynamic, then it should */
/*          be at least lrn or lrs, as defined above, depending on the */
/*          current method.  initially, it must be at least lrn (since */
/*          lsoda starts with the nonstiff method).  on any return */
/*          from lsoda, the optional output mcur indicates the current */
/*          method.  if mcur differs from the value it had on the */
/*          previous return, or if there has only been one call to */
/*          lsoda and mcur is now 2, then lsoda has switched */
/*          methods during the last call, and the length of rwork */
/*          should be reset (to lrn if mcur = 1, or to lrs if */
/*          mcur = 2).  (an increase in the rwork length is required */
/*          if lsoda returned istate = -7, but not otherwise.) */
/*          after resetting the length, call lsoda with istate = 3 */
/*          to signal that change. */

/* lrw    = the length of the array rwork, as declared by the user. */
/*          (this will be checked by the solver.) */

/* iwork  = an integer array for work space. */
/*          as lsoda switches automatically between stiff and nonstiff */
/*          methods, the required length of iwork can change during */
/*          problem, between */
/*             lis = 20 + neq   and   lin = 20, */
/*          respectively.  thus the iwork array passed to lsoda can */
/*          either have a fixed length of at least 20 + neq, or have a */
/*          dynamic length of at least lin or lis, depending on the */
/*          current method.  the comments on dynamic length under */
/*          rwork above apply here.  initially, this length need */
/*          only be at least lin = 20. */

/*          the first few words of iwork are used for conditional and */
/*          optional inputs and optional outputs. */

/*          the following 2 words in iwork are conditional inputs.. */
/*            iwork(1) = ml     these are the lower and upper */
/*            iwork(2) = mu     half-bandwidths, respectively, of the */
/*                       banded jacobian, excluding the main diagonal. */
/*                       the band is defined by the matrix locations */
/*                       (i,j) with i-ml .le. j .le. i+mu.  ml and mu */
/*                       must satisfy  0 .le.  ml,mu  .le. neq-1. */
/*                       these are required if jt is 4 or 5, and */
/*                       ignored otherwise.  ml and mu may in fact be */
/*                       the band parameters for a matrix to which */
/*                       df/dy is only approximately equal. */

/* liw    = the length of the array iwork, as declared by the user. */
/*          (this will be checked by the solver.) */

/* note.. the base addresses of the work arrays must not be */
/* altered between calls to lsoda for the same problem. */
/* the contents of the work arrays must not be altered */
/* between calls, except possibly for the conditional and */
/* optional inputs, and except for the last 3*neq words of rwork. */
/* the latter space is used for internal scratch space, and so is */
/* available for use by the user outside lsoda between calls, if */
/* desired (but not for use by f or jac). */

/* jac    = the name of the user-supplied routine to compute the */
/*          jacobian matrix, df/dy, if jt = 1 or 4.  the jac routine */
/*          is optional, but if the problem is expected to be stiff much 
*/
/*          of the time, you are encouraged to supply jac, for the sake */

/*          of efficiency.  (alternatively, set jt = 2 or 5 to have */
/*          lsoda compute df/dy internally by difference quotients.) */
/*          if and when lsoda uses df/dy, if treats this neq by neq */
/*          matrix either as full (jt = 1 or 2), or as banded (jt = */
/*          4 or 5) with half-bandwidths ml and mu (discussed under */
/*          iwork above).  in either case, if jt = 1 or 4, the jac */
/*          routine must compute df/dy as a function of the scalar t */
/*          and the vector y.  it is to have the form */
/*               subroutine jac (neq, t, y, ml, mu, pd, nrowpd) */
/*               dimension y(1), pd(nrowpd,1) */
/*          where neq, t, y, ml, mu, and nrowpd are input and the array */

/*          pd is to be loaded with partial derivatives (elements of */
/*          the jacobian matrix) on output.  pd must be given a first */
/*          dimension of nrowpd.  t and y have the same meaning as in */
/*          subroutine f.  (in the dimension statement above, 1 is a */
/*          dummy dimension.. it can be replaced by any value.) */
/*               in the full matrix case (jt = 1), ml and mu are */
/*          ignored, and the jacobian is to be loaded into pd in */
/*          columnwise manner, with df(i)/dy(j) loaded into pd(i,j). */
/*               in the band matrix case (jt = 4), the elements */
/*          within the band are to be loaded into pd in columnwise */
/*          manner, with diagonal lines of df/dy loaded into the rows */
/*          of pd.  thus df(i)/dy(j) is to be loaded into pd(i-j+mu+1,j). 
*/
/*          ml and mu are the half-bandwidth parameters (see iwork). */
/*          the locations in pd in the two triangular areas which */
/*          correspond to nonexistent matrix elements can be ignored */
/*          or loaded arbitrarily, as they are overwritten by lsoda. */
/*               jac need not provide df/dy exactly.  a crude */
/*          approximation (possibly with a smaller bandwidth) will do. */
/*               in either case, pd is preset to zero by the solver, */
/*          so that only the nonzero elements need be loaded by jac. */
/*          each call to jac is preceded by a call to f with the same */
/*          arguments neq, t, and y.  thus to gain some efficiency, */
/*          intermediate quantities shared by both calculations may be */
/*          saved in a user common block by f and not recomputed by jac, 
*/
/*          if desired.  also, jac may alter the y array, if desired. */
/*          jac must be declared external in the calling program. */
/*               subroutine jac may access user-defined quantities in */
/*          neq(2),... and/or in y(neq(1)+1),... if neq is an array */
/*          (dimensioned in jac) and/or y has length exceeding neq(1). */
/*          see the descriptions of neq and y above. */

/* jt     = jacobian type indicator.  used only for input. */
/*          jt specifies how the jacobian matrix df/dy will be */
/*          treated, if and when lsoda requires this matrix. */
/*          jt has the following values and meanings.. */
/*           1 means a user-supplied full (neq by neq) jacobian. */
/*           2 means an internally generated (difference quotient) full */

/*             jacobian (using neq extra calls to f per df/dy value). */
/*           4 means a user-supplied banded jacobian. */
/*           5 means an internally generated banded jacobian (using */
/*             ml+mu+1 extra calls to f per df/dy evaluation). */
/*          if jt = 1 or 4, the user must supply a subroutine jac */
/*          (the name is arbitrary) as described above under jac. */
/*          if jt = 2 or 5, a dummy argument can be used. */
/* -----------------------------------------------------------------------
 */
/* optional inputs. */

/* the following is a list of the optional inputs provided for in the */
/* call sequence.  (see also part ii.)  for each such input variable, */
/* this table lists its name as used in this documentation, its */
/* location in the call sequence, its meaning, and the default value. */
/* the use of any of these inputs requires iopt = 1, and in that */
/* case all of these inputs are examined.  a value of zero for any */
/* of these optional inputs will cause the default value to be used. */
/* thus to use a subset of the optional inputs, simply preload */
/* locations 5 to 10 in rwork and iwork to 0.0 and 0 respectively, and */
/* then set those of interest to nonzero values. */

/* name    location      meaning and default value */

/* h0      rwork(5)  the step size to be attempted on the first step. */
/*                   the default value is determined by the solver. */

/* hmax    rwork(6)  the maximum absolute step size allowed. */
/*                   the default value is infinite. */

/* hmin    rwork(7)  the minimum absolute step size allowed. */
/*                   the default value is 0.  (this lower bound is not */
/*                   enforced on the final step before reaching tcrit */
/*                   when itask = 4 or 5.) */

/* ixpr    iwork(5)  flag to generate extra printing at method switches. 
*/
/*                   ixpr = 0 means no extra printing (the default). */
/*                   ixpr = 1 means print data on each switch. */
/*                   t, h, and nst will be printed on the same logical */
/*                   unit as used for error messages. */

/* mxstep  iwork(6)  maximum number of (internally defined) steps */
/*                   allowed during one call to the solver. */
/*                   the default value is 500. */

/* mxhnil  iwork(7)  maximum number of messages printed (per problem) */
/*                   warning that t + h = t on a step (h = step size). */
/*                   this must be positive to result in a non-default */
/*                   value.  the default value is 10. */

/* mxordn  iwork(8)  the maximum order to be allowed for the nonstiff */
/*                   (adams) method.  the default value is 12. */
/*                   if mxordn exceeds the default value, it will */
/*                   be reduced to the default value. */
/*                   mxordn is held constant during the problem. */

/* mxords  iwork(9)  the maximum order to be allowed for the stiff */
/*                   (bdf) method.  the default value is 5. */
/*                   if mxords exceeds the default value, it will */
/*                   be reduced to the default value. */
/*                   mxords is held constant during the problem. */
/* -----------------------------------------------------------------------
 */
/* optional outputs. */

/* as optional additional output from lsoda, the variables listed */
/* below are quantities related to the performance of lsoda */
/* which are available to the user.  these are communicated by way of */
/* the work arrays, but also have internal mnemonic names as shown. */
/* except where stated otherwise, all of these outputs are defined */
/* on any successful return from lsoda, and on any return with */
/* istate = -1, -2, -4, -5, or -6.  on an illegal input return */
/* (istate = -3), they will be unchanged from their existing values */
/* (if any), except possibly for tolsf, lenrw, and leniw. */
/* on any error return, outputs relevant to the error will be defined, */
/* as noted below. */

/* name    location      meaning */

/* hu      rwork(11) the step size in t last used (successfully). */

/* hcur    rwork(12) the step size to be attempted on the next step. */

/* tcur    rwork(13) the current value of the independent variable */
/*                   which the solver has actually reached, i.e. the */
/*                   current internal mesh point in t.  on output, tcur */

/*                   will always be at least as far as the argument */
/*                   t, but may be farther (if interpolation was done). */


/* tolsf   rwork(14) a tolerance scale factor, greater than 1.0, */
/*                   computed when a request for too much accuracy was */
/*                   detected (istate = -3 if detected at the start of */
/*                   the problem, istate = -2 otherwise).  if itol is */
/*                   left unaltered but rtol and atol are uniformly */
/*                   scaled up by a factor of tolsf for the next call, */
/*                   then the solver is deemed likely to succeed. */
/*                   (the user may also ignore tolsf and alter the */
/*                   tolerance parameters in any other way appropriate.) 
*/

/* tsw     rwork(15) the value of t at the time of the last method */
/*                   switch, if any. */

/* nst     iwork(11) the number of steps taken for the problem so far. */

/* nfe     iwork(12) the number of f evaluations for the problem so far. 
*/

/* nje     iwork(13) the number of jacobian evaluations (and of matrix */
/*                   lu decompositions) for the problem so far. */

/* nqu     iwork(14) the method order last used (successfully). */

/* nqcur   iwork(15) the order to be attempted on the next step. */

/* imxer   iwork(16) the index of the component of largest magnitude in */

/*                   the weighted local error vector ( e(i)/ewt(i) ), */
/*                   on an error return with istate = -4 or -5. */

/* lenrw   iwork(17) the length of rwork actually required, assuming */
/*                   that the length of rwork is to be fixed for the */
/*                   rest of the problem, and that switching may occur. */

/*                   this is defined on normal returns and on an illegal 
*/
/*                   input return for insufficient storage. */

/* leniw   iwork(18) the length of iwork actually required, assuming */
/*                   that the length of iwork is to be fixed for the */
/*                   rest of the problem, and that switching may occur. */

/*                   this is defined on normal returns and on an illegal 
*/
/*                   input return for insufficient storage. */

/* mused   iwork(19) the method indicator for the last successful step.. 
*/
/*                   1 means adams (nonstiff), 2 means bdf (stiff). */

/* mcur    iwork(20) the current method indicator.. */
/*                   1 means adams (nonstiff), 2 means bdf (stiff). */
/*                   this is the method to be attempted */
/*                   on the next step.  thus it differs from mused */
/*                   only if a method switch has just been made. */

/* the following two arrays are segments of the rwork array which */
/* may also be of interest to the user as optional outputs. */
/* for each array, the table below gives its internal name, */
/* its base address in rwork, and its description. */

/* name    base address      description */

/* yh      21             the nordsieck history array, of size nyh by */
/*                        (nqcur + 1), where nyh is the initial value */
/*                        of neq.  for j = 0,1,...,nqcur, column j+1 */
/*                        of yh contains hcur**j/factorial(j) times */
/*                        the j-th derivative of the interpolating */
/*                        polynomial currently representing the solution, 
*/
/*                        evaluated at t = tcur. */

/* acor     lacor         array of size neq used for the accumulated */
/*         (from common   corrections on each step, scaled on output */
/*           as noted)    to represent the estimated local error in y */
/*                        on the last step.  this is the vector e in */
/*                        the description of the error control.  it is */
/*                        defined only on a successful return from lsoda. 
*/
/*                        the base address lacor is obtained by */
/*                        including in the user-s program the */
/*                        following 3 lines.. */
/*                           double precision rls */
/*                           common /ls0001/ rls(218), ils(39) */
/*                           lacor = ils(5) */

/* -----------------------------------------------------------------------
 */
/* part ii.  other routines callable. */

/* the following are optional calls which the user may make to */
/* gain additional capabilities in conjunction with lsoda. */
/* (the routines xsetun and xsetf are designed to conform to the */
/* slatec error handling package.) */

/*     form of call                  function */
/*   call xsetun(lun)          set the logical unit number, lun, for */
/*                             output of messages from lsoda, if */
/*                             the default is not desired. */
/*                             the default value of lun is 6. */

/*   call xsetf(mflag)         set a flag to control the printing of */
/*                             messages by lsoda. */
/*                             mflag = 0 means do not print. (danger.. */
/*                             this risks losing valuable information.) */

/*                             mflag = 1 means print (the default). */

/*                             either of the above calls may be made at */

/*                             any time and will take effect immediately. 
*/

/*   call srcma(rsav,isav,job) saves and restores the contents of */
/*                             the internal common blocks used by */
/*                             lsoda (see part iii below). */
/*                             rsav must be a real array of length 240 */
/*                             or more, and isav must be an integer */
/*                             array of length 50 or more. */
/*                             job=1 means save common into rsav/isav. */
/*                             job=2 means restore common from rsav/isav. 
*/
/*                                srcma is useful if one is */
/*                             interrupting a run and restarting */
/*                             later, or alternating between two or */
/*                             more problems solved with lsoda. */

/*   call intdy(,,,,,)         provide derivatives of y, of various */
/*        (see below)          orders, at a specified point t, if */
/*                             desired.  it may be called only after */
/*                             a successful return from lsoda. */

/* the detailed instructions for using intdy are as follows. */
/* the form of the call is.. */

/*   call intdy (t, k, rwork(21), nyh, dky, iflag) */

/* the input parameters are.. */

/* t         = value of independent variable where answers are desired */
/*             (normally the same as the t last returned by lsoda). */
/*             for valid results, t must lie between tcur - hu and tcur. 
*/
/*             (see optional outputs for tcur and hu.) */
/* k         = integer order of the derivative desired.  k must satisfy */

/*             0 .le. k .le. nqcur, where nqcur is the current order */
/*             (see optional outputs).  the capability corresponding */
/*             to k = 0, i.e. computing y(t), is already provided */
/*             by lsoda directly.  since nqcur .ge. 1, the first */
/*             derivative dy/dt is always available with intdy. */
/* rwork(21) = the base address of the history array yh. */
/* nyh       = column length of yh, equal to the initial value of neq. */

/* the output parameters are.. */

/* dky       = a real array of length neq containing the computed value */

/*             of the k-th derivative of y(t). */
/* iflag     = integer flag, returned as 0 if k and t were legal, */
/*             -1 if k was illegal, and -2 if t was illegal. */
/*             on an error return, a message is also written. */
/* -----------------------------------------------------------------------
 */
/* part iii.  common blocks. */

/* if lsoda is to be used in an overlay situation, the user */
/* must declare, in the primary overlay, the variables in.. */
/*   (1) the call sequence to lsoda, */
/*   (2) the three internal common blocks */
/*         /ls0001/  of length  257  (218 double precision words */
/*                         followed by 39 integer words), */
/*         /lsa001/  of length  31    (22 double precision words */
/*                         followed by  9 integer words), */
/*         /eh0001/  of length  2 (integer words). */

/* if lsoda is used on a system in which the contents of internal */
/* common blocks are not preserved between calls, the user should */
/* declare the above common blocks in his main program to insure */
/* that their contents are preserved. */

/* if the solution of a given problem by lsoda is to be interrupted */
/* and then later continued, such as when restarting an interrupted run */

/* or alternating between two or more problems, the user should save, */
/* following the return from the last lsoda call prior to the */
/* interruption, the contents of the call sequence variables and the */
/* internal common blocks, and later restore these values before the */
/* next lsoda call for that problem.  to save and restore the common */
/* blocks, use subroutine srcma (see part ii above). */

/* -----------------------------------------------------------------------
 */
/* part iv.  optionally replaceable solver routines. */

/* below is a description of a routine in the lsoda package which */
/* relates to the measurement of errors, and can be */
/* replaced by a user-supplied version, if desired.  however, since such 
*/
/* a replacement may have a major impact on performance, it should be */
/* done only when absolutely necessary, and only with great caution. */
/* (note.. the means by which the package version of a routine is */
/* superseded by the user-s version may be system-dependent.) */

/* (a) ewset. */
/* the following subroutine is called just before each internal */
/* integration step, and sets the array of error weights, ewt, as */
/* described under itol/rtol/atol above.. */
/*     subroutine ewset (neq, itol, rtol, atol, ycur, ewt) */
/* where neq, itol, rtol, and atol are as in the lsoda call sequence, */
/* ycur contains the current dependent variable vector, and */
/* ewt is the array of weights set by ewset. */

/* if the user supplies this subroutine, it must return in ewt(i) */
/* (i = 1,...,neq) a positive quantity suitable for comparing errors */
/* in y(i) to.  the ewt array returned by ewset is passed to the */
/* vmnorm routine, and also used by lsoda in the computation */
/* of the optional output imxer, and the increments for difference */
/* quotient jacobians. */

/* in the user-supplied version of ewset, it may be desirable to use */
/* the current values of derivatives of y.  derivatives up to order nq */
/* are available from the history array yh, described above under */
/* optional outputs.  in ewset, yh is identical to the ycur array, */
/* extended to nq + 1 columns with a column length of nyh and scale */
/* factors of h**j/factorial(j).  on the first call for the problem, */
/* given by nst = 0, nq is 1 and h is temporarily set to 1.0. */
/* the quantities nq, nyh, h, and nst can be obtained by including */
/* in ewset the statements.. */
/*     double precision h, rls */
/*     common /ls0001/ rls(218),ils(39) */
/*     nq = ils(35) */
/*     nyh = ils(14) */
/*     nst = ils(36) */
/*     h = rls(212) */
/* thus, for example, the current value of dy/dt can be obtained as */
/* ycur(nyh+i)/h  (i=1,...,neq)  (and the division by h is */
/* unnecessary when nst = 0). */
/* -----------------------------------------------------------------------
 */
/* -----------------------------------------------------------------------
 */
/* other routines in the lsoda package. */

/* in addition to subroutine lsoda, the lsoda package includes the */
/* following subroutines and function routines.. */
/*  intdy    computes an interpolated value of the y vector at t = tout. 
*/
/*  stoda    is the core integrator, which does one step of the */
/*           integration and the associated error control. */
/*  cfode    sets all method coefficients and test constants. */
/*  prja     computes and preprocesses the jacobian matrix j = df/dy */
/*           and the newton iteration matrix p = i - h*l0*j. */
/*  solsy    manages solution of linear system in chord iteration. */
/*  ewset    sets the error weight vector ewt before each step. */
/*  vmnorm   computes the weighted max-norm of a vector. */
/*  fnorm    computes the norm of a full matrix consistent with the */
/*           weighted max-norm on vectors. */
/*  bnorm    computes the norm of a band matrix consistent with the */
/*           weighted max-norm on vectors. */
/*  srcma    is a user-callable routine to save and restore */
/*           the contents of the internal common blocks. */
/*  dgefa and dgesl   are routines from linpack for solving full */
/*           systems of linear algebraic equations. */
/*  dgbfa and dgbsl   are routines from linpack for solving banded */
/*           linear systems. */
/*  daxpy, dscal, idamax, and ddot   are basic linear algebra modules */
/*           (blas) used by the above linpack routines. */
/*  d1mach   computes the unit roundoff in a machine-independent manner. 
*/
/*  xerrwv, xsetun, and xsetf   handle the printing of all error */
/*           messages and warnings.  xerrwv is machine-dependent. */
/* note..  vmnorm, fnorm, bnorm, idamax, ddot, and d1mach are function */
/* routines.  all the others are subroutines. */

/* the intrinsic and external routines used by lsoda are.. */
/* dabs, dmax1, dmin1, dfloat, max0, min0, mod, dsign, dsqrt, and write. 
*/

/* a block data subprogram is also included with the package, */
/* for loading some of the variables in internal common. */

/* -----------------------------------------------------------------------
 */
/* the following card is for optimized compilation on lll compilers. */
/* lll. optimize */
/* -----------------------------------------------------------------------
 */
/* -----------------------------------------------------------------------
 */
/* the following two internal common blocks contain */
/* (a) variables which are local to any subroutine but whose values must 
*/
/*     be preserved between calls to the routine (own variables), and */
/* (b) variables which are communicated between subroutines. */
/* the structure of each block is as follows..  all real variables are */
/* listed first, followed by all integers.  within each type, the */
/* variables are grouped with those local to subroutine lsoda first, */
/* then those local to subroutine stoda, and finally those used */
/* for communication.  the block ls0001 is declared in subroutines */
/* lsoda, intdy, stoda, prja, and solsy.  the block lsa001 is declared */
/* in subroutines lsoda, stoda, and prja.  groups of variables are */
/* replaced by dummy arrays in the common declarations in routines */
/* where those variables are not used. */
/* -----------------------------------------------------------------------
 */

    /* Parameter adjustments */
    --iwork;
    --rwork;
    --atol;
    --rtol;
    --y;
    --neq;

    /* Function Body */
/* -----------------------------------------------------------------------
 */
/* block a. */
/* this code block is executed on every call. */
/* it tests istate and itask for legality and branches appropriately. */
/* if istate .gt. 1 but the flag init shows that initialization has */
/* not yet been done, an error return occurs. */
/* if istate = 1 and tout = t, jump to block g and return immediately. */
/* -----------------------------------------------------------------------
 */
    if (*istate < 1 || *istate > 3) {
	goto L601;
    }
    if (*itask < 1 || *itask > 5) {
	goto L602;
    }
    if (*istate == 1) {
	goto L10;
    }
    if (ls0001_1.init == 0) {
	goto L603;
    }
    if (*istate == 2) {
	goto L200;
    }
    goto L20;
L10:
    ls0001_1.init = 0;
    if (*tout == *t) {
	goto L430;
    }
L20:
    ls0001_1.ntrep = 0;
/* -----------------------------------------------------------------------
 */
/* block b. */
/* the next code block is executed for the initial call (istate = 1), */
/* or for a continuation call with parameter changes (istate = 3). */
/* it contains checking of all inputs and various initializations. */

/* first check legality of the non-optional inputs neq, itol, iopt, */
/* jt, ml, and mu. */
/* -----------------------------------------------------------------------
 */
    if (neq[1] <= 0) {
	goto L604;
    }
    if (*istate == 1) {
	goto L25;
    }
    if (neq[1] > ls0001_1.n) {
	goto L605;
    }
L25:
    ls0001_1.n = neq[1];
    if (*itol < 1 || *itol > 4) {
	goto L606;
    }
    if (*iopt < 0 || *iopt > 1) {
	goto L607;
    }
    if (*jt == 3 || *jt < 1 || *jt > 5) {
	goto L608;
    }
    lsa001_1.jtyp = *jt;
    if (*jt <= 2) {
	goto L30;
    }
    ml = iwork[1];
    mu = iwork[2];
    if (ml < 0 || ml >= ls0001_1.n) {
	goto L609;
    }
    if (mu < 0 || mu >= ls0001_1.n) {
	goto L610;
    }
L30:
/* next process and check the optional inputs. -------------------------- 
*/
    if (*iopt == 1) {
	goto L40;
    }
    lsa001_1.ixpr = 0;
    ls0001_1.mxstep = mxstp0;
    ls0001_1.mxhnil = mxhnl0;
    ls0001_1.hmxi = 0.;
    ls0001_1.hmin = 0.;
    if (*istate != 1) {
	goto L60;
    }
    h0 = 0.;
    lsa001_1.mxordn = mord[0];
    lsa001_1.mxords = mord[1];
    goto L60;
L40:
    lsa001_1.ixpr = iwork[5];
    if (lsa001_1.ixpr < 0 || lsa001_1.ixpr > 1) {
	goto L611;
    }
    ls0001_1.mxstep = iwork[6];
    if (ls0001_1.mxstep < 0) {
	goto L612;
    }
    if (ls0001_1.mxstep == 0) {
	ls0001_1.mxstep = mxstp0;
    }
    ls0001_1.mxhnil = iwork[7];
    if (ls0001_1.mxhnil < 0) {
	goto L613;
    }
    if (ls0001_1.mxhnil == 0) {
	ls0001_1.mxhnil = mxhnl0;
    }
    if (*istate != 1) {
	goto L50;
    }
    h0 = rwork[5];
    lsa001_1.mxordn = iwork[8];
    if (lsa001_1.mxordn < 0) {
	goto L628;
    }
    if (lsa001_1.mxordn == 0) {
	lsa001_1.mxordn = 100;
    }
    lsa001_1.mxordn = min(lsa001_1.mxordn,mord[0]);
    lsa001_1.mxords = iwork[9];
    if (lsa001_1.mxords < 0) {
	goto L629;
    }
    if (lsa001_1.mxords == 0) {
	lsa001_1.mxords = 100;
    }
    lsa001_1.mxords = min(lsa001_1.mxords,mord[1]);
    if ((*tout - *t) * h0 < 0.) {
	goto L614;
    }
L50:
    hmax = rwork[6];
    if (hmax < 0.) {
	goto L615;
    }
    ls0001_1.hmxi = 0.;
    if (hmax > 0.) {
	ls0001_1.hmxi = 1. / hmax;
    }
    ls0001_1.hmin = rwork[7];
    if (ls0001_1.hmin < 0.) {
	goto L616;
    }
/* -----------------------------------------------------------------------
 */
/* set work array pointers and check lengths lrw and liw. */
/* if istate = 1, meth is initialized to 1 here to facilitate the */
/* checking of work space lengths. */
/* pointers to segments of rwork and iwork are named by prefixing l to */
/* the name of the segment.  e.g., the segment yh starts at rwork(lyh). */

/* segments of rwork (in order) are denoted  yh, wm, ewt, savf, acor. */
/* if the lengths provided are insufficient for the current method, */
/* an error return occurs.  this is treated as illegal input on the */
/* first call, but as a problem interruption with istate = -7 on a */
/* continuation call.  if the lengths are sufficient for the current */
/* method but not for both methods, a warning message is sent. */
/* -----------------------------------------------------------------------
 */
L60:
    if (*istate == 1) {
	ls0001_1.meth = 1;
    }
    if (*istate == 1) {
	ls0001_1.nyh = ls0001_1.n;
    }
    ls0001_1.lyh = 21;
    len1n = (lsa001_1.mxordn + 1) * ls0001_1.nyh + 20;
    len1s = (lsa001_1.mxords + 1) * ls0001_1.nyh + 20;
    ls0001_1.lwm = len1s + 1;
    if (*jt <= 2) {
	lenwm = ls0001_1.n * ls0001_1.n + 2;
    }
    if (*jt >= 4) {
	lenwm = ((ml << 1) + mu + 1) * ls0001_1.n + 2;
    }
    len1s += lenwm;
    len1c = len1n;
    if (ls0001_1.meth == 2) {
	len1c = len1s;
    }
    len1 = max(len1n,len1s);
    len2 = ls0001_1.n * 3;
    lenrw = len1 + len2;
    lenrwn = len1n + len2;
    lenrws = len1s + len2;
    lenrwc = len1c + len2;
    iwork[17] = lenrw;
    ls0001_1.liwm = 1;
    leniw = ls0001_1.n + 20;
    leniwc = 20;
    if (ls0001_1.meth == 2) {
	leniwc = leniw;
    }
    iwork[18] = leniw;
    if (*istate == 1 && *lrw < lenrwc) {
	goto L617;
    }
    if (*istate == 1 && *liw < leniwc) {
	goto L618;
    }
    if (*istate == 3 && *lrw < lenrwc) {
	goto L550;
    }
    if (*istate == 3 && *liw < leniwc) {
	goto L555;
    }
    ls0001_1.lewt = len1 + 1;
    lsa001_1.insufr = 0;
    if (*lrw >= lenrw) {
	goto L65;
    }
    lsa001_1.insufr = 2;
    ls0001_1.lewt = len1c + 1;
    xerrwv_("lsoda--  warning.. rwork length is sufficient for now, but  ", &
	    c__60, &c__103, &c__0, &c__0, &c__0, &c__0, &c__0, &c_b136, &
	    c_b136, 60L);
    xerrwv_("      may not be later.  integration will proceed anyway.   ", &
	    c__60, &c__103, &c__0, &c__0, &c__0, &c__0, &c__0, &c_b136, &
	    c_b136, 60L);
    xerrwv_("      length needed is lenrw = i1, while lrw = i2.", &c__50, &
	    c__103, &c__0, &c__2, &lenrw, lrw, &c__0, &c_b136, &c_b136, 50L);
L65:
    ls0001_1.lsavf = ls0001_1.lewt + ls0001_1.n;
    ls0001_1.lacor = ls0001_1.lsavf + ls0001_1.n;
    lsa001_1.insufi = 0;
    if (*liw >= leniw) {
	goto L70;
    }
    lsa001_1.insufi = 2;
    xerrwv_("lsoda--  warning.. iwork length is sufficient for now, but  ", &
	    c__60, &c__104, &c__0, &c__0, &c__0, &c__0, &c__0, &c_b136, &
	    c_b136, 60L);
    xerrwv_("      may not be later.  integration will proceed anyway.   ", &
	    c__60, &c__104, &c__0, &c__0, &c__0, &c__0, &c__0, &c_b136, &
	    c_b136, 60L);
    xerrwv_("      length needed is leniw = i1, while liw = i2.", &c__50, &
	    c__104, &c__0, &c__2, &leniw, liw, &c__0, &c_b136, &c_b136, 50L);
L70:
/* check rtol and atol for legality. ------------------------------------ 
*/
    rtoli = rtol[1];
    atoli = atol[1];
    i_1 = ls0001_1.n;
    for (i = 1; i <= i_1; ++i) {
	if (*itol >= 3) {
	    rtoli = rtol[i];
	}
	if (*itol == 2 || *itol == 4) {
	    atoli = atol[i];
	}
	if (rtoli < 0.) {
	    goto L619;
	}
	if (atoli < 0.) {
	    goto L620;
	}
/* L75: */
    }
    if (*istate == 1) {
	goto L100;
    }
/* if istate = 3, set flag to signal parameter changes to stoda. -------- 
*/
    ls0001_1.jstart = -1;
    if (ls0001_1.n == ls0001_1.nyh) {
	goto L200;
    }
/* neq was reduced.  zero part of yh to avoid undefined references. ----- 
*/
    i1 = ls0001_1.lyh + ls0001_1.l * ls0001_1.nyh;
    i2 = ls0001_1.lyh + (ls0001_1.maxord + 1) * ls0001_1.nyh - 1;
    if (i1 > i2) {
	goto L200;
    }
    i_1 = i2;
    for (i = i1; i <= i_1; ++i) {
/* L95: */
	rwork[i] = 0.;
    }
    goto L200;
/* -----------------------------------------------------------------------
 */
/* block c. */
/* the next block is for the initial call only (istate = 1). */
/* it contains all remaining initializations, the initial call to f, */
/* and the calculation of the initial step size. */
/* the error weights in ewt are inverted after being loaded. */
/* -----------------------------------------------------------------------
 */
L100:
    ls0001_1.uround = d1mach_(&c__4);
    ls0001_1.tn = *t;
    lsa001_1.tsw = *t;
    ls0001_1.maxord = lsa001_1.mxordn;
    if (*itask != 4 && *itask != 5) {
	goto L110;
    }
    tcrit = rwork[1];
    if ((tcrit - *tout) * (*tout - *t) < 0.) {
	goto L625;
    }
    if (h0 != 0. && (*t + h0 - tcrit) * h0 > 0.) {
	h0 = tcrit - *t;
    }
L110:
    ls0001_1.jstart = 0;
    ls0001_1.nhnil = 0;
    ls0001_1.nst = 0;
    ls0001_1.nje = 0;
    ls0001_1.nslast = 0;
    ls0001_1.hu = 0.;
    ls0001_1.nqu = 0;
    lsa001_1.mused = 0;
    ls0001_1.miter = 0;
    ls0001_1.ccmax = .3;
    ls0001_1.maxcor = 3;
    ls0001_1.msbp = 20;
    ls0001_1.mxncf = 10;
/* initial call to f.  (lf0 points to yh(*,2).) ------------------------- 
*/
    lf0 = ls0001_1.lyh + ls0001_1.nyh;
    (*f)(&neq[1], t, &y[1], &rwork[lf0]);
    ls0001_1.nfe = 1;
/* load the initial value vector in yh. --------------------------------- 
*/
    i_1 = ls0001_1.n;
    for (i = 1; i <= i_1; ++i) {
/* L115: */
	rwork[i + ls0001_1.lyh - 1] = y[i];
    }
/* load and invert the ewt array.  (h is temporarily set to 1.0.) ------- 
*/
    ls0001_1.nq = 1;
    ls0001_1.h = 1.;
    ewset_(&ls0001_1.n, itol, &rtol[1], &atol[1], &rwork[ls0001_1.lyh], &
	    rwork[ls0001_1.lewt]);
    i_1 = ls0001_1.n;
    for (i = 1; i <= i_1; ++i) {
	if (rwork[i + ls0001_1.lewt - 1] <= 0.) {
	    goto L621;
	}
/* L120: */
	rwork[i + ls0001_1.lewt - 1] = 1. / rwork[i + ls0001_1.lewt - 1];
    }
/* -----------------------------------------------------------------------
 */
/* the coding below computes the step size, h0, to be attempted on the */
/* first step, unless the user has supplied a value for this. */
/* first check that tout - t differs significantly from zero. */
/* a scalar tolerance quantity tol is computed, as max(rtol(i)) */
/* if this is positive, or max(atol(i)/abs(y(i))) otherwise, adjusted */
/* so as to be between 100*uround and 1.0e-3. */
/* then the computed value h0 is given by.. */

/*   h0**(-2)  =  1./(tol * w0**2)  +  tol * (norm(f))**2 */

/* where   w0     = max ( abs(t), abs(tout) ), */
/*         f      = the initial value of the vector f(t,y), and */
/*         norm() = the weighted vector norm used throughout, given by */
/*                  the vmnorm function routine, and weighted by the */
/*                  tolerances initially loaded into the ewt array. */
/* the sign of h0 is inferred from the initial values of tout and t. */
/* abs(h0) is made .le. abs(tout-t) in any case. */
/* -----------------------------------------------------------------------
 */
    if (h0 != 0.) {
	goto L180;
    }
    tdist = (d_1 = *tout - *t, abs(d_1));
/* Computing MAX */
    d_1 = abs(*t), d_2 = abs(*tout);
    w0 = max(d_1,d_2);
    if (tdist < ls0001_1.uround * 2. * w0) {
	goto L622;
    }
    tol = rtol[1];
    if (*itol <= 2) {
	goto L140;
    }
    i_1 = ls0001_1.n;
    for (i = 1; i <= i_1; ++i) {
/* L130: */
/* Computing MAX */
	d_1 = tol, d_2 = rtol[i];
	tol = max(d_1,d_2);
    }
L140:
    if (tol > 0.) {
	goto L160;
    }
    atoli = atol[1];
    i_1 = ls0001_1.n;
    for (i = 1; i <= i_1; ++i) {
	if (*itol == 2 || *itol == 4) {
	    atoli = atol[i];
	}
	ayi = (d_1 = y[i], abs(d_1));
	if (ayi != 0.) {
/* Computing MAX */
	    d_1 = tol, d_2 = atoli / ayi;
	    tol = max(d_1,d_2);
	}
/* L150: */
    }
L160:
/* Computing MAX */
    d_1 = tol, d_2 = ls0001_1.uround * 100.;
    tol = max(d_1,d_2);
    tol = min(tol,.001);
    sum = vmnorm_(&ls0001_1.n, &rwork[lf0], &rwork[ls0001_1.lewt]);
/* Computing 2nd power */
    d_1 = sum;
    sum = 1. / (tol * w0 * w0) + tol * (d_1 * d_1);
    h0 = 1. / sqrt(sum);
    h0 = min(h0,tdist);
    d_1 = *tout - *t;
    h0 = d_sign(&h0, &d_1);
/* adjust h0 if necessary to meet hmax bound. --------------------------- 
*/
L180:
    rh = abs(h0) * ls0001_1.hmxi;
    if (rh > 1.) {
	h0 /= rh;
    }
/* load h with h0 and scale yh(*,2) by h0. ------------------------------ 
*/
    ls0001_1.h = h0;
    i_1 = ls0001_1.n;
    for (i = 1; i <= i_1; ++i) {
/* L190: */
	rwork[i + lf0 - 1] = h0 * rwork[i + lf0 - 1];
    }
    goto L270;
/* -----------------------------------------------------------------------
 */
/* block d. */
/* the next code block is for continuation calls only (istate = 2 or 3) */

/* and is to check stop conditions before taking a step. */
/* -----------------------------------------------------------------------
 */
L200:
    ls0001_1.nslast = ls0001_1.nst;
    switch ((int)*itask) {
	case 1:  goto L210;
	case 2:  goto L250;
	case 3:  goto L220;
	case 4:  goto L230;
	case 5:  goto L240;
    }
L210:
    if ((ls0001_1.tn - *tout) * ls0001_1.h < 0.) {
	goto L250;
    }
    intdy_(tout, &c__0, &rwork[ls0001_1.lyh], &ls0001_1.nyh, &y[1], &iflag);
    if (iflag != 0) {
	goto L627;
    }
    *t = *tout;
    goto L420;
L220:
    tp = ls0001_1.tn - ls0001_1.hu * (ls0001_1.uround * 100. + 1.);
    if ((tp - *tout) * ls0001_1.h > 0.) {
	goto L623;
    }
    if ((ls0001_1.tn - *tout) * ls0001_1.h < 0.) {
	goto L250;
    }
    *t = ls0001_1.tn;
    goto L400;
L230:
    tcrit = rwork[1];
    if ((ls0001_1.tn - tcrit) * ls0001_1.h > 0.) {
	goto L624;
    }
    if ((tcrit - *tout) * ls0001_1.h < 0.) {
	goto L625;
    }
    if ((ls0001_1.tn - *tout) * ls0001_1.h < 0.) {
	goto L245;
    }
    intdy_(tout, &c__0, &rwork[ls0001_1.lyh], &ls0001_1.nyh, &y[1], &iflag);
    if (iflag != 0) {
	goto L627;
    }
    *t = *tout;
    goto L420;
L240:
    tcrit = rwork[1];
    if ((ls0001_1.tn - tcrit) * ls0001_1.h > 0.) {
	goto L624;
    }
L245:
    hmx = abs(ls0001_1.tn) + abs(ls0001_1.h);
    ihit = (d_1 = ls0001_1.tn - tcrit, abs(d_1)) <= ls0001_1.uround * 100. * 
	    hmx;
    if (ihit) {
	*t = tcrit;
    }
    if (ihit) {
	goto L400;
    }
    tnext = ls0001_1.tn + ls0001_1.h * (ls0001_1.uround * 4. + 1.);
    if ((tnext - tcrit) * ls0001_1.h <= 0.) {
	goto L250;
    }
    ls0001_1.h = (tcrit - ls0001_1.tn) * (1. - ls0001_1.uround * 4.);
    if (*istate == 2) {
	ls0001_1.jstart = -2;
    }
/* -----------------------------------------------------------------------
 */
/* block e. */
/* the next block is normally executed for all calls and contains */
/* the call to the one-step core integrator stoda. */

/* this is a looping point for the integration steps. */

/* first check for too many steps being taken, update ewt (if not at */
/* start of problem), check for too much accuracy being requested, and */
/* check for h below the roundoff level in t. */
/* -----------------------------------------------------------------------
 */
L250:
    if (ls0001_1.meth == lsa001_1.mused) {
	goto L255;
    }
    if (lsa001_1.insufr == 1) {
	goto L550;
    }
    if (lsa001_1.insufi == 1) {
	goto L555;
    }
L255:
    if (ls0001_1.nst - ls0001_1.nslast >= ls0001_1.mxstep) {
	goto L500;
    }
    ewset_(&ls0001_1.n, itol, &rtol[1], &atol[1], &rwork[ls0001_1.lyh], &
	    rwork[ls0001_1.lewt]);
    i_1 = ls0001_1.n;
    for (i = 1; i <= i_1; ++i) {
	if (rwork[i + ls0001_1.lewt - 1] <= 0.) {
	    goto L510;
	}
/* L260: */
	rwork[i + ls0001_1.lewt - 1] = 1. / rwork[i + ls0001_1.lewt - 1];
    }
L270:
    tolsf = ls0001_1.uround * vmnorm_(&ls0001_1.n, &rwork[ls0001_1.lyh], &
	    rwork[ls0001_1.lewt]);
    if (tolsf <= .01) {
	goto L280;
    }
    tolsf *= 200.;
    if (ls0001_1.nst == 0) {
	goto L626;
    }
    goto L520;
L280:
    if (ls0001_1.tn + ls0001_1.h != ls0001_1.tn) {
	goto L290;
    }
    ++ls0001_1.nhnil;
    if (ls0001_1.nhnil > ls0001_1.mxhnil) {
	goto L290;
    }
    xerrwv_("lsoda--  warning..internal t (=r1) and h (=r2) are", &c__50, &
	    c__101, &c__0, &c__0, &c__0, &c__0, &c__0, &c_b136, &c_b136, 50L);

    xerrwv_("      such that in the machine, t + h = t on the next step  ", &
	    c__60, &c__101, &c__0, &c__0, &c__0, &c__0, &c__0, &c_b136, &
	    c_b136, 60L);
    xerrwv_("      (h = step size). solver will continue anyway", &c__50, &
	    c__101, &c__0, &c__0, &c__0, &c__0, &c__2, &ls0001_1.tn, &
	    ls0001_1.h, 50L);
    if (ls0001_1.nhnil < ls0001_1.mxhnil) {
	goto L290;
    }
    xerrwv_("lsoda--  above warning has been issued i1 times.  ", &c__50, &
	    c__102, &c__0, &c__0, &c__0, &c__0, &c__0, &c_b136, &c_b136, 50L);

    xerrwv_("      it will not be issued again for this problem", &c__50, &
	    c__102, &c__0, &c__1, &ls0001_1.mxhnil, &c__0, &c__0, &c_b136, &
	    c_b136, 50L);
L290:
/* -----------------------------------------------------------------------
 */
/*     call stoda(neq,y,yh,nyh,yh,ewt,savf,acor,wm,iwm,f,jac,prja,solsy) 
*/
/* -----------------------------------------------------------------------
 */
    stoda_(&neq[1], &y[1], &rwork[ls0001_1.lyh], &ls0001_1.nyh, &rwork[
	    ls0001_1.lyh], &rwork[ls0001_1.lewt], &rwork[ls0001_1.lsavf], &
	    rwork[ls0001_1.lacor], &rwork[ls0001_1.lwm], &iwork[ls0001_1.liwm]
	    , f, jac, prja_, solsy_);
    kgo = 1 - ls0001_1.kflag;
    switch ((int)kgo) {
	case 1:  goto L300;
	case 2:  goto L530;
	case 3:  goto L540;
    }
/* -----------------------------------------------------------------------
 */
/* block f. */
/* the following block handles the case of a successful return from the */

/* core integrator (kflag = 0). */
/* if a method switch was just made, record tsw, reset maxord, */
/* set jstart to -1 to signal stoda to complete the switch, */
/* and do extra printing of data if ixpr = 1. */
/* then, in any case, check for stop conditions. */
/* -----------------------------------------------------------------------
 */
L300:
    ls0001_1.init = 1;
    if (ls0001_1.meth == lsa001_1.mused) {
	goto L310;
    }
    lsa001_1.tsw = ls0001_1.tn;
    ls0001_1.maxord = lsa001_1.mxordn;
    if (ls0001_1.meth == 2) {
	ls0001_1.maxord = lsa001_1.mxords;
    }
    if (ls0001_1.meth == 2) {
	rwork[ls0001_1.lwm] = sqrt(ls0001_1.uround);
    }
    lsa001_1.insufr = min(lsa001_1.insufr,1);
    lsa001_1.insufi = min(lsa001_1.insufi,1);
    ls0001_1.jstart = -1;
    if (lsa001_1.ixpr == 0) {
	goto L310;
    }
    if (ls0001_1.meth == 2) {
	xerrwv_("lsoda-- a switch to the bdf (stiff) method has occurred     "
		, &c__60, &c__105, &c__0, &c__0, &c__0, &c__0, &c__0, &c_b136,
		 &c_b136, 60L);
    }
    if (ls0001_1.meth == 1) {
	xerrwv_("lsoda-- a switch to the adams (nonstiff) method has occurred"
		, &c__60, &c__106, &c__0, &c__0, &c__0, &c__0, &c__0, &c_b136,
		 &c_b136, 60L);
    }
    xerrwv_("     at t = r1,  tentative step size h = r2,  step nst = i1 ", &
	    c__60, &c__107, &c__0, &c__1, &ls0001_1.nst, &c__0, &c__2, &
	    ls0001_1.tn, &ls0001_1.h, 60L);
L310:
    switch ((int)*itask) {
	case 1:  goto L320;
	case 2:  goto L400;
	case 3:  goto L330;
	case 4:  goto L340;
	case 5:  goto L350;
    }
/* itask = 1.  if tout has been reached, interpolate. ------------------- 
*/
L320:
    if ((ls0001_1.tn - *tout) * ls0001_1.h < 0.) {
	goto L250;
    }
    intdy_(tout, &c__0, &rwork[ls0001_1.lyh], &ls0001_1.nyh, &y[1], &iflag);
    *t = *tout;
    goto L420;
/* itask = 3.  jump to exit if tout was reached. ------------------------ 
*/
L330:
    if ((ls0001_1.tn - *tout) * ls0001_1.h >= 0.) {
	goto L400;
    }
    goto L250;
/* itask = 4.  see if tout or tcrit was reached.  adjust h if necessary. 
*/
L340:
    if ((ls0001_1.tn - *tout) * ls0001_1.h < 0.) {
	goto L345;
    }
    intdy_(tout, &c__0, &rwork[ls0001_1.lyh], &ls0001_1.nyh, &y[1], &iflag);
    *t = *tout;
    goto L420;
L345:
    hmx = abs(ls0001_1.tn) + abs(ls0001_1.h);
    ihit = (d_1 = ls0001_1.tn - tcrit, abs(d_1)) <= ls0001_1.uround * 100. * 
	    hmx;
    if (ihit) {
	goto L400;
    }
    tnext = ls0001_1.tn + ls0001_1.h * (ls0001_1.uround * 4. + 1.);
    if ((tnext - tcrit) * ls0001_1.h <= 0.) {
	goto L250;
    }
    ls0001_1.h = (tcrit - ls0001_1.tn) * (1. - ls0001_1.uround * 4.);
    ls0001_1.jstart = -2;
    goto L250;
/* itask = 5.  see if tcrit was reached and jump to exit. --------------- 
*/
L350:
    hmx = abs(ls0001_1.tn) + abs(ls0001_1.h);
    ihit = (d_1 = ls0001_1.tn - tcrit, abs(d_1)) <= ls0001_1.uround * 100. * 
	    hmx;
/* -----------------------------------------------------------------------
 */
/* block g. */
/* the following block handles all successful returns from lsoda. */
/* if itask .ne. 1, y is loaded from yh and t is set accordingly. */
/* istate is set to 2, the illegal input counter is zeroed, and the */
/* optional outputs are loaded into the work arrays before returning. */
/* if istate = 1 and tout = t, there is a return with no action taken, */
/* except that if this has happened repeatedly, the run is terminated. */
/* -----------------------------------------------------------------------
 */
L400:
    i_1 = ls0001_1.n;
    for (i = 1; i <= i_1; ++i) {
/* L410: */
	y[i] = rwork[i + ls0001_1.lyh - 1];
    }
    *t = ls0001_1.tn;
    if (*itask != 4 && *itask != 5) {
	goto L420;
    }
    if (ihit) {
	*t = tcrit;
    }
L420:
    *istate = 2;
    ls0001_1.illin = 0;
    rwork[11] = ls0001_1.hu;
    rwork[12] = ls0001_1.h;
    rwork[13] = ls0001_1.tn;
    rwork[15] = lsa001_1.tsw;
    iwork[11] = ls0001_1.nst;
    iwork[12] = ls0001_1.nfe;
    iwork[13] = ls0001_1.nje;
    iwork[14] = ls0001_1.nqu;
    iwork[15] = ls0001_1.nq;
    iwork[19] = lsa001_1.mused;
    iwork[20] = ls0001_1.meth;
    return 0;

L430:
    ++ls0001_1.ntrep;
    if (ls0001_1.ntrep < 5) {
	return 0;
    }
    xerrwv_("lsoda--  repeated calls with istate = 1 and tout = t (=r1)  ", &
	    c__60, &c__301, &c__0, &c__0, &c__0, &c__0, &c__1, t, &c_b136, 
	    60L);
    goto L800;
/* -----------------------------------------------------------------------
 */
/* block h. */
/* the following block handles all unsuccessful returns other than */
/* those for illegal input.  first the error message routine is called. */

/* if there was an error test or convergence test failure, imxer is set. 
*/
/* then y is loaded from yh, t is set to tn, and the illegal input */
/* counter illin is set to 0.  the optional outputs are loaded into */
/* the work arrays before returning. */
/* -----------------------------------------------------------------------
 */
/* the maximum number of steps was taken before reaching tout. ---------- 
*/
L500:
    xerrwv_("lsoda--  at current t (=r1), mxstep (=i1) steps   ", &c__50, &
	    c__201, &c__0, &c__0, &c__0, &c__0, &c__0, &c_b136, &c_b136, 50L);

    xerrwv_("      taken on this call before reaching tout     ", &c__50, &
	    c__201, &c__0, &c__1, &ls0001_1.mxstep, &c__0, &c__1, &
	    ls0001_1.tn, &c_b136, 50L);
    *istate = -1;
    goto L580;
/* ewt(i) .le. 0.0 for some i (not at start of problem). ---------------- 
*/
L510:
    ewti = rwork[ls0001_1.lewt + i - 1];
    xerrwv_("lsoda--  at t (=r1), ewt(i1) has become r2 .le. 0.", &c__50, &
	    c__202, &c__0, &c__1, &i, &c__0, &c__2, &ls0001_1.tn, &ewti, 50L);

    *istate = -6;
    goto L580;
/* too much accuracy requested for machine precision. ------------------- 
*/
L520:
    xerrwv_("lsoda--  at t (=r1), too much accuracy requested  ", &c__50, &
	    c__203, &c__0, &c__0, &c__0, &c__0, &c__0, &c_b136, &c_b136, 50L);

    xerrwv_("      for precision of machine..  see tolsf (=r2) ", &c__50, &
	    c__203, &c__0, &c__0, &c__0, &c__0, &c__2, &ls0001_1.tn, &tolsf, 
	    50L);
    rwork[14] = tolsf;
    *istate = -2;
    goto L580;
/* kflag = -1.  error test failed repeatedly or with abs(h) = hmin. ----- 
*/
L530:
    xerrwv_("lsoda--  at t(=r1) and step size h(=r2), the error", &c__50, &
	    c__204, &c__0, &c__0, &c__0, &c__0, &c__0, &c_b136, &c_b136, 50L);

    xerrwv_("      test failed repeatedly or with abs(h) = hmin", &c__50, &
	    c__204, &c__0, &c__0, &c__0, &c__0, &c__2, &ls0001_1.tn, &
	    ls0001_1.h, 50L);
    *istate = -4;
    goto L560;
/* kflag = -2.  convergence failed repeatedly or with abs(h) = hmin. ---- 
*/
L540:
    xerrwv_("lsoda--  at t (=r1) and step size h (=r2), the    ", &c__50, &
	    c__205, &c__0, &c__0, &c__0, &c__0, &c__0, &c_b136, &c_b136, 50L);

    xerrwv_("      corrector convergence failed repeatedly     ", &c__50, &
	    c__205, &c__0, &c__0, &c__0, &c__0, &c__0, &c_b136, &c_b136, 50L);

    xerrwv_("      or with abs(h) = hmin   ", &c__30, &c__205, &c__0, &c__0, &
	    c__0, &c__0, &c__2, &ls0001_1.tn, &ls0001_1.h, 30L);
    *istate = -5;
    goto L560;
/* rwork length too small to proceed. ----------------------------------- 
*/
L550:
    xerrwv_("lsoda--  at current t(=r1), rwork length too small", &c__50, &
	    c__206, &c__0, &c__0, &c__0, &c__0, &c__0, &c_b136, &c_b136, 50L);

    xerrwv_("      to proceed.  the integration was otherwise successful.", &
	    c__60, &c__206, &c__0, &c__0, &c__0, &c__0, &c__1, &ls0001_1.tn, &
	    c_b136, 60L);
    *istate = -7;
    goto L580;
/* iwork length too small to proceed. ----------------------------------- 
*/
L555:
    xerrwv_("lsoda--  at current t(=r1), iwork length too small", &c__50, &
	    c__207, &c__0, &c__0, &c__0, &c__0, &c__0, &c_b136, &c_b136, 50L);

    xerrwv_("      to proceed.  the integration was otherwise successful.", &
	    c__60, &c__207, &c__0, &c__0, &c__0, &c__0, &c__1, &ls0001_1.tn, &
	    c_b136, 60L);
    *istate = -7;
    goto L580;
/* compute imxer if relevant. ------------------------------------------- 
*/
L560:
    big = 0.;
    imxer = 1;
    i_1 = ls0001_1.n;
    for (i = 1; i <= i_1; ++i) {
	size = (d_1 = rwork[i + ls0001_1.lacor - 1] * rwork[i + ls0001_1.lewt 
		- 1], abs(d_1));
	if (big >= size) {
	    goto L570;
	}
	big = size;
	imxer = i;
L570:
	;
    }
    iwork[16] = imxer;
/* set y vector, t, illin, and optional outputs. ------------------------ 
*/
L580:
    i_1 = ls0001_1.n;
    for (i = 1; i <= i_1; ++i) {
/* L590: */
	y[i] = rwork[i + ls0001_1.lyh - 1];
    }
    *t = ls0001_1.tn;
    ls0001_1.illin = 0;
    rwork[11] = ls0001_1.hu;
    rwork[12] = ls0001_1.h;
    rwork[13] = ls0001_1.tn;
    rwork[15] = lsa001_1.tsw;
    iwork[11] = ls0001_1.nst;
    iwork[12] = ls0001_1.nfe;
    iwork[13] = ls0001_1.nje;
    iwork[14] = ls0001_1.nqu;
    iwork[15] = ls0001_1.nq;
    iwork[19] = lsa001_1.mused;
    iwork[20] = ls0001_1.meth;
    return 0;
/* -----------------------------------------------------------------------
 */
/* block i. */
/* the following block handles all error returns due to illegal input */
/* (istate = -3), as detected before calling the core integrator. */
/* first the error message routine is called.  then if there have been */
/* 5 consecutive such returns just before this call to the solver, */
/* the run is halted. */
/* -----------------------------------------------------------------------
 */
L601:
    xerrwv_("lsoda--  istate (=i1) illegal ", &c__30, &c__1, &c__0, &c__1, 
	    istate, &c__0, &c__0, &c_b136, &c_b136, 30L);
    goto L700;
L602:
    xerrwv_("lsoda--  itask (=i1) illegal  ", &c__30, &c__2, &c__0, &c__1, 
	    itask, &c__0, &c__0, &c_b136, &c_b136, 30L);
    goto L700;
L603:
    xerrwv_("lsoda--  istate .gt. 1 but lsoda not initialized  ", &c__50, &
	    c__3, &c__0, &c__0, &c__0, &c__0, &c__0, &c_b136, &c_b136, 50L);
    goto L700;
L604:
    xerrwv_("lsoda--  neq (=i1) .lt. 1     ", &c__30, &c__4, &c__0, &c__1, &
	    neq[1], &c__0, &c__0, &c_b136, &c_b136, 30L);
    goto L700;
L605:
    xerrwv_("lsoda--  istate = 3 and neq increased (i1 to i2)  ", &c__50, &
	    c__5, &c__0, &c__2, &ls0001_1.n, &neq[1], &c__0, &c_b136, &c_b136,
	     50L);
    goto L700;
L606:
    xerrwv_("lsoda--  itol (=i1) illegal   ", &c__30, &c__6, &c__0, &c__1, 
	    itol, &c__0, &c__0, &c_b136, &c_b136, 30L);
    goto L700;
L607:
    xerrwv_("lsoda--  iopt (=i1) illegal   ", &c__30, &c__7, &c__0, &c__1, 
	    iopt, &c__0, &c__0, &c_b136, &c_b136, 30L);
    goto L700;
L608:
    xerrwv_("lsoda--  jt (=i1) illegal     ", &c__30, &c__8, &c__0, &c__1, jt,
	     &c__0, &c__0, &c_b136, &c_b136, 30L);
    goto L700;
L609:
    xerrwv_("lsoda--  ml (=i1) illegal.. .lt.0 or .ge.neq (=i2)", &c__50, &
	    c__9, &c__0, &c__2, &ml, &neq[1], &c__0, &c_b136, &c_b136, 50L);
    goto L700;
L610:
    xerrwv_("lsoda--  mu (=i1) illegal.. .lt.0 or .ge.neq (=i2)", &c__50, &
	    c__10, &c__0, &c__2, &mu, &neq[1], &c__0, &c_b136, &c_b136, 50L);
    goto L700;
L611:
    xerrwv_("lsoda--  ixpr (=i1) illegal   ", &c__30, &c__11, &c__0, &c__1, &
	    lsa001_1.ixpr, &c__0, &c__0, &c_b136, &c_b136, 30L);
    goto L700;
L612:
    xerrwv_("lsoda--  mxstep (=i1) .lt. 0  ", &c__30, &c__12, &c__0, &c__1, &
	    ls0001_1.mxstep, &c__0, &c__0, &c_b136, &c_b136, 30L);
    goto L700;
L613:
    xerrwv_("lsoda--  mxhnil (=i1) .lt. 0  ", &c__30, &c__13, &c__0, &c__1, &
	    ls0001_1.mxhnil, &c__0, &c__0, &c_b136, &c_b136, 30L);
    goto L700;
L614:
    xerrwv_("lsoda--  tout (=r1) behind t (=r2)      ", &c__40, &c__14, &c__0,
	     &c__0, &c__0, &c__0, &c__2, tout, t, 40L);
    xerrwv_("      integration direction is given by h0 (=r1)  ", &c__50, &
	    c__14, &c__0, &c__0, &c__0, &c__0, &c__1, &h0, &c_b136, 50L);
    goto L700;
L615:
    xerrwv_("lsoda--  hmax (=r1) .lt. 0.0  ", &c__30, &c__15, &c__0, &c__0, &
	    c__0, &c__0, &c__1, &hmax, &c_b136, 30L);
    goto L700;
L616:
    xerrwv_("lsoda--  hmin (=r1) .lt. 0.0  ", &c__30, &c__16, &c__0, &c__0, &
	    c__0, &c__0, &c__1, &ls0001_1.hmin, &c_b136, 30L);
    goto L700;
L617:
    xerrwv_("lsoda--  rwork length needed, lenrw (=i1), exceeds lrw (=i2)", &
	    c__60, &c__17, &c__0, &c__2, &lenrw, lrw, &c__0, &c_b136, &c_b136,
	     60L);
    goto L700;
L618:
    xerrwv_("lsoda--  iwork length needed, leniw (=i1), exceeds liw (=i2)", &
	    c__60, &c__18, &c__0, &c__2, &leniw, liw, &c__0, &c_b136, &c_b136,
	     60L);
    goto L700;
L619:
    xerrwv_("lsoda--  rtol(i1) is r1 .lt. 0.0        ", &c__40, &c__19, &c__0,
	     &c__1, &i, &c__0, &c__1, &rtoli, &c_b136, 40L);
    goto L700;
L620:
    xerrwv_("lsoda--  atol(i1) is r1 .lt. 0.0        ", &c__40, &c__20, &c__0,
	     &c__1, &i, &c__0, &c__1, &atoli, &c_b136, 40L);
    goto L700;
L621:
    ewti = rwork[ls0001_1.lewt + i - 1];
    xerrwv_("lsoda--  ewt(i1) is r1 .le. 0.0         ", &c__40, &c__21, &c__0,
	     &c__1, &i, &c__0, &c__1, &ewti, &c_b136, 40L);
    goto L700;
L622:
    xerrwv_("lsoda--  tout (=r1) too close to t(=r2) to start integration", &
	    c__60, &c__22, &c__0, &c__0, &c__0, &c__0, &c__2, tout, t, 60L);
    goto L700;
L623:
    xerrwv_("lsoda--  itask = i1 and tout (=r1) behind tcur - hu (= r2)  ", &
	    c__60, &c__23, &c__0, &c__1, itask, &c__0, &c__2, tout, &tp, 60L);

    goto L700;
L624:
    xerrwv_("lsoda--  itask = 4 or 5 and tcrit (=r1) behind tcur (=r2)   ", &
	    c__60, &c__24, &c__0, &c__0, &c__0, &c__0, &c__2, &tcrit, &
	    ls0001_1.tn, 60L);
    goto L700;
L625:
    xerrwv_("lsoda--  itask = 4 or 5 and tcrit (=r1) behind tout (=r2)   ", &
	    c__60, &c__25, &c__0, &c__0, &c__0, &c__0, &c__2, &tcrit, tout, 
	    60L);
    goto L700;
L626:
    xerrwv_("lsoda--  at start of problem, too much accuracy   ", &c__50, &
	    c__26, &c__0, &c__0, &c__0, &c__0, &c__0, &c_b136, &c_b136, 50L);
    xerrwv_("      requested for precision of machine..  see tolsf (=r1) ", &
	    c__60, &c__26, &c__0, &c__0, &c__0, &c__0, &c__1, &tolsf, &c_b136,
	     60L);
    rwork[14] = tolsf;
    goto L700;
L627:
    xerrwv_("lsoda--  trouble from intdy. itask = i1, tout = r1", &c__50, &
	    c__27, &c__0, &c__1, itask, &c__0, &c__1, tout, &c_b136, 50L);
    goto L700;
L628:
    xerrwv_("lsoda--  mxordn (=i1) .lt. 0  ", &c__30, &c__28, &c__0, &c__1, &
	    lsa001_1.mxordn, &c__0, &c__0, &c_b136, &c_b136, 30L);
    goto L700;
L629:
    xerrwv_("lsoda--  mxords (=i1) .lt. 0  ", &c__30, &c__29, &c__0, &c__1, &
	    lsa001_1.mxords, &c__0, &c__0, &c_b136, &c_b136, 30L);

L700:
    if (ls0001_1.illin == 5) {
	goto L710;
    }
    ++ls0001_1.illin;
    *istate = -3;
    return 0;
L710:
    xerrwv_("lsoda--  repeated occurrences of illegal input    ", &c__50, &
	    c__302, &c__0, &c__0, &c__0, &c__0, &c__0, &c_b136, &c_b136, 50L);


L800:
    xerrwv_("lsoda--  run aborted.. apparent infinite loop     ", &c__50, &
	    c__303, &c__2, &c__0, &c__0, &c__0, &c__0, &c_b136, &c_b136, 50L);

    return 0;
/* ----------------------- end of subroutine lsoda -----------------------
 */
} /* lsoda_ */

/* Subroutine */ int prja_(neq, y, yh, nyh, ewt, ftem, savf, wm, iwm, f, jac)
integer *neq;
doublereal *y, *yh;
integer *nyh;
doublereal *ewt, *ftem, *savf, *wm;
integer *iwm;
/* Subroutine */ int (*f) (), (*jac) ();
{
    /* System generated locals */
    integer yh_dim1, yh_offset, i_1, i_2, i_3, i_4;
    doublereal d_1, d_2;

    /* Local variables */
    static integer lenp;
    static doublereal srur;
    extern /* Subroutine */ int dgbfa_(), dgefa_();
    static integer i, j, mband;
    static doublereal r;
    extern doublereal bnorm_(), fnorm_();
    static integer i1, i2, j1;
    static doublereal r0;
    static integer ii, jj, meband, ml, mu;
    static doublereal yi, yj, hl0;
    static integer ml3, np1;
    extern doublereal vmnorm_();
    static doublereal fac;
    static integer mba, ier;
    static doublereal con, yjj;
    static integer meb1;

/* lll. optimize */
/* -----------------------------------------------------------------------
 */
/* prja is called by stoda to compute and process the matrix */
/* p = i - h*el(1)*j , where j is an approximation to the jacobian. */
/* here j is computed by the user-supplied routine jac if */
/* miter = 1 or 4 or by finite differencing if miter = 2 or 5. */
/* j, scaled by -h*el(1), is stored in wm.  then the norm of j (the */
/* matrix norm consistent with the weighted max-norm on vectors given */
/* by vmnorm) is computed, and j is overwritten by p.  p is then */
/* subjected to lu decomposition in preparation for later solution */
/* of linear systems with p as coefficient matrix. this is done */
/* by dgefa if miter = 1 or 2, and by dgbfa if miter = 4 or 5. */

/* in addition to variables described previously, communication */
/* with prja uses the following.. */
/* y     = array containing predicted values on entry. */
/* ftem  = work array of length n (acor in stoda). */
/* savf  = array containing f evaluated at predicted y. */
/* wm    = real work space for matrices.  on output it contains the */
/*         lu decomposition of p. */
/*         storage of matrix elements starts at wm(3). */
/*         wm also contains the following matrix-related data.. */
/*         wm(1) = sqrt(uround), used in numerical jacobian increments. */

/* iwm   = integer work space containing pivot information, starting at */

/*         iwm(21).   iwm also contains the band parameters */
/*         ml = iwm(1) and mu = iwm(2) if miter is 4 or 5. */
/* el0   = el(1) (input). */
/* pdnorm= norm of jacobian matrix. (output). */
/* ierpj = output error flag,  = 0 if no trouble, .gt. 0 if */
/*         p matrix found to be singular. */
/* jcur  = output flag = 1 to indicate that the jacobian matrix */
/*         (or approximation) is now current. */
/* this routine also uses the common variables el0, h, tn, uround, */
/* miter, n, nfe, and nje. */
/* -----------------------------------------------------------------------
 */
    /* Parameter adjustments */
    --iwm;
    --wm;
    --savf;
    --ftem;
    --ewt;
    yh_dim1 = *nyh;
    yh_offset = yh_dim1 + 1;
    yh -= yh_offset;
    --y;
    --neq;

    /* Function Body */
    ++ls0001_2.nje;
    ls0001_2.ierpj = 0;
    ls0001_2.jcur = 1;
    hl0 = ls0001_2.h * ls0001_2.el0;
    switch ((int)ls0001_2.miter) {
	case 1:  goto L100;
	case 2:  goto L200;
	case 3:  goto L300;
	case 4:  goto L400;
	case 5:  goto L500;
    }
/* if miter = 1, call jac and multiply by scalar. ----------------------- 
*/
L100:
    lenp = ls0001_2.n * ls0001_2.n;
    i_1 = lenp;
    for (i = 1; i <= i_1; ++i) {
/* L110: */
	wm[i + 2] = 0.;
    }
    (*jac)(&neq[1], &ls0001_2.tn, &y[1], &c__0, &c__0, &wm[3], &ls0001_2.n);
    con = -hl0;
    i_1 = lenp;
    for (i = 1; i <= i_1; ++i) {
/* L120: */
	wm[i + 2] *= con;
    }
    goto L240;
/* if miter = 2, make n calls to f to approximate j. -------------------- 
*/
L200:
    fac = vmnorm_(&ls0001_2.n, &savf[1], &ewt[1]);
    r0 = abs(ls0001_2.h) * 1e3 * ls0001_2.uround * (doublereal) ls0001_2.n * 
	    fac;
    if (r0 == 0.) {
	r0 = 1.;
    }
    srur = wm[1];
    j1 = 2;
    i_1 = ls0001_2.n;
    for (j = 1; j <= i_1; ++j) {
	yj = y[j];
/* Computing MAX */
	d_1 = srur * abs(yj), d_2 = r0 / ewt[j];
	r = max(d_1,d_2);
	y[j] += r;
	fac = -hl0 / r;
	(*f)(&neq[1], &ls0001_2.tn, &y[1], &ftem[1]);
	i_2 = ls0001_2.n;
	for (i = 1; i <= i_2; ++i) {
/* L220: */
	    wm[i + j1] = (ftem[i] - savf[i]) * fac;
	}
	y[j] = yj;
	j1 += ls0001_2.n;
/* L230: */
    }
    ls0001_2.nfe += ls0001_2.n;
L240:
/* compute norm of jacobian. -------------------------------------------- 
*/
    lsa001_2.pdnorm = fnorm_(&ls0001_2.n, &wm[3], &ewt[1]) / abs(hl0);
/* add identity matrix. ------------------------------------------------- 
*/
    np1 = ls0001_2.n + 1;
    j = 3;
    i_1 = ls0001_2.n;
    for (i = 1; i <= i_1; ++i) {
	wm[j] += 1.;
/* L250: */
	j += np1;
    }
/* do lu decomposition on p. -------------------------------------------- 
*/
    dgefa_(&wm[3], &ls0001_2.n, &ls0001_2.n, &iwm[21], &ier);
    if (ier != 0) {
	ls0001_2.ierpj = 1;
    }
    return 0;
/* dummy block only, since miter is never 3 in this routine. ------------ 
*/
L300:
    return 0;
/* if miter = 4, call jac and multiply by scalar. ----------------------- 
*/
L400:
    ml = iwm[1];
    mu = iwm[2];
    ml3 = ml + 3;
    mband = ml + mu + 1;
    meband = mband + ml;
    lenp = meband * ls0001_2.n;
    i_1 = lenp;
    for (i = 1; i <= i_1; ++i) {
/* L410: */
	wm[i + 2] = 0.;
    }
    (*jac)(&neq[1], &ls0001_2.tn, &y[1], &ml, &mu, &wm[ml3], &meband);
    con = -hl0;
    i_1 = lenp;
    for (i = 1; i <= i_1; ++i) {
/* L420: */
	wm[i + 2] *= con;
    }
    goto L570;
/* if miter = 5, make mband calls to f to approximate j. ---------------- 
*/
L500:
    ml = iwm[1];
    mu = iwm[2];
    mband = ml + mu + 1;
    mba = min(mband,ls0001_2.n);
    meband = mband + ml;
    meb1 = meband - 1;
    srur = wm[1];
    fac = vmnorm_(&ls0001_2.n, &savf[1], &ewt[1]);
    r0 = abs(ls0001_2.h) * 1e3 * ls0001_2.uround * (doublereal) ls0001_2.n * 
	    fac;
    if (r0 == 0.) {
	r0 = 1.;
    }
    i_1 = mba;
    for (j = 1; j <= i_1; ++j) {
	i_2 = ls0001_2.n;
	i_3 = mband;
	for (i = j; i_3 < 0 ? i >= i_2 : i <= i_2; i += i_3) {
	    yi = y[i];
/* Computing MAX */
	    d_1 = srur * abs(yi), d_2 = r0 / ewt[i];
	    r = max(d_1,d_2);
/* L530: */
	    y[i] += r;
	}
	(*f)(&neq[1], &ls0001_2.tn, &y[1], &ftem[1]);
	i_3 = ls0001_2.n;
	i_2 = mband;
	for (jj = j; i_2 < 0 ? jj >= i_3 : jj <= i_3; jj += i_2) {
	    y[jj] = yh[jj + yh_dim1];
	    yjj = y[jj];
/* Computing MAX */
	    d_1 = srur * abs(yjj), d_2 = r0 / ewt[jj];
	    r = max(d_1,d_2);
	    fac = -hl0 / r;
/* Computing MAX */
	    i_4 = jj - mu;
	    i1 = max(i_4,1);
/* Computing MIN */
	    i_4 = jj + ml;
	    i2 = min(i_4,ls0001_2.n);
	    ii = jj * meb1 - ml + 2;
	    i_4 = i2;
	    for (i = i1; i <= i_4; ++i) {
/* L540: */
		wm[ii + i] = (ftem[i] - savf[i]) * fac;
	    }
/* L550: */
	}
/* L560: */
    }
    ls0001_2.nfe += mba;
L570:
/* compute norm of jacobian. -------------------------------------------- 
*/
    lsa001_2.pdnorm = bnorm_(&ls0001_2.n, &wm[3], &meband, &ml, &mu, &ewt[1]) 
	    / abs(hl0);
/* add identity matrix. ------------------------------------------------- 
*/
    ii = mband + 2;
    i_1 = ls0001_2.n;
    for (i = 1; i <= i_1; ++i) {
	wm[ii] += 1.;
/* L580: */
	ii += meband;
    }
/* do lu decomposition of p. -------------------------------------------- 
*/
    dgbfa_(&wm[3], &meband, &ls0001_2.n, &ml, &mu, &iwm[21], &ier);
    if (ier != 0) {
	ls0001_2.ierpj = 1;
    }
    return 0;
/* ----------------------- end of subroutine prja ------------------------
 */
} /* prja_ */

/* Subroutine */ int solsy_(wm, iwm, x, tem)
doublereal *wm;
integer *iwm;
doublereal *x, *tem;
{
    /* System generated locals */
    integer i_1;

    /* Local variables */
    static integer i;
    static doublereal r;
    extern /* Subroutine */ int dgbsl_(), dgesl_();
    static doublereal di;
    static integer meband, ml, mu;
    static doublereal hl0, phl0;

/* lll. optimize */
/* -----------------------------------------------------------------------
 */
/* this routine manages the solution of the linear system arising from */
/* a chord iteration.  it is called if miter .ne. 0. */
/* if miter is 1 or 2, it calls dgesl to accomplish this. */
/* if miter = 3 it updates the coefficient h*el0 in the diagonal */
/* matrix, and then computes the solution. */
/* if miter is 4 or 5, it calls dgbsl. */
/* communication with solsy uses the following variables.. */
/* wm    = real work space containing the inverse diagonal matrix if */
/*         miter = 3 and the lu decomposition of the matrix otherwise. */
/*         storage of matrix elements starts at wm(3). */
/*         wm also contains the following matrix-related data.. */
/*         wm(1) = sqrt(uround) (not used here), */
/*         wm(2) = hl0, the previous value of h*el0, used if miter = 3. */

/* iwm   = integer work space containing pivot information, starting at */

/*         iwm(21), if miter is 1, 2, 4, or 5.  iwm also contains band */
/*         parameters ml = iwm(1) and mu = iwm(2) if miter is 4 or 5. */
/* x     = the right-hand side vector on input, and the solution vector */

/*         on output, of length n. */
/* tem   = vector of work space of length n, not used in this version. */
/* iersl = output flag (in common).  iersl = 0 if no trouble occurred. */
/*         iersl = 1 if a singular matrix arose with miter = 3. */
/* this routine also uses the common variables el0, h, miter, and n. */
/* -----------------------------------------------------------------------
 */
    /* Parameter adjustments */
    --tem;
    --x;
    --iwm;
    --wm;

    /* Function Body */
    ls0001_2.iersl = 0;
    switch ((int)ls0001_2.miter) {
	case 1:  goto L100;
	case 2:  goto L100;
	case 3:  goto L300;
	case 4:  goto L400;
	case 5:  goto L400;
    }
L100:
    dgesl_(&wm[3], &ls0001_2.n, &ls0001_2.n, &iwm[21], &x[1], &c__0);
    return 0;

L300:
    phl0 = wm[2];
    hl0 = ls0001_2.h * ls0001_2.el0;
    wm[2] = hl0;
    if (hl0 == phl0) {
	goto L330;
    }
    r = hl0 / phl0;
    i_1 = ls0001_2.n;
    for (i = 1; i <= i_1; ++i) {
	di = 1. - r * (1. - 1. / wm[i + 2]);
	if (abs(di) == 0.) {
	    goto L390;
	}
/* L320: */
	wm[i + 2] = 1. / di;
    }
L330:
    i_1 = ls0001_2.n;
    for (i = 1; i <= i_1; ++i) {
/* L340: */
	x[i] = wm[i + 2] * x[i];
    }
    return 0;
L390:
    ls0001_2.iersl = 1;
    return 0;

L400:
    ml = iwm[1];
    mu = iwm[2];
    meband = (ml << 1) + mu + 1;
    dgbsl_(&wm[3], &meband, &ls0001_2.n, &ml, &mu, &iwm[21], &x[1], &c__0);
    return 0;
/* ----------------------- end of subroutine solsy -----------------------
 */
} /* solsy_ */

/* Subroutine */ int stoda_(neq, y, yh, nyh, yh1, ewt, savf, acor, wm, iwm, f,
	 jac, pjac, slvs)
integer *neq;
doublereal *y, *yh;
integer *nyh;
doublereal *yh1, *ewt, *savf, *acor, *wm;
integer *iwm;
/* Subroutine */ int (*f) (), (*jac) (), (*pjac) (), (*slvs) ();
{
    /* Initialized data */

    static doublereal sm1[12] = { .5,.575,.55,.45,.35,.25,.2,.15,.1,.075,.05,
	    .025 };

    /* System generated locals */
    integer yh_dim1, yh_offset, i_1, i_2;
    doublereal d_1, d_2, d_3;

    /* Builtin functions */
    double pow_dd();

    /* Local variables */
    static doublereal dcon, delp;
    static integer lm1p1, lm2p1;
    static doublereal exdn, rhdn;
    static integer iret;
    static doublereal told, rate, rhsm;
    static integer newq;
    static doublereal exsm, rhup, exup, rh1it;
    static integer i, j, m;
    extern /* Subroutine */ int cfode_();
    static doublereal r, alpha;
    static integer iredo, i1;
    static doublereal pnorm;
    static integer jb;
    static doublereal rh, rm, dm1, dm2;
    static integer lm1, lm2;
    extern doublereal vmnorm_();
    static doublereal rh1, rh2, del, ddn;
    static integer ncf;
    static doublereal pdh, dsm, dup, exm1, exm2;
    static integer nqm1, nqm2;

/* lll. optimize */
    /* Parameter adjustments */
    --iwm;
    --wm;
    --acor;
    --savf;
    --ewt;
    --yh1;
    yh_dim1 = *nyh;
    yh_offset = yh_dim1 + 1;
    yh -= yh_offset;
    --y;
    --neq;

    /* Function Body */
/* -----------------------------------------------------------------------
 */
/*          meth = 1 means adams method (nonstiff) */
/*          meth = 2 means bdf method (stiff) */
/*          meth may be reset by stoda. */
/* miter  = corrector iteration method. */
/*          miter = 0 means functional iteration. */
/*          miter = jt .gt. 0 means a chord iteration corresponding */
/*          to jacobian type jt.  (the lsoda argument jt is */
/*          communicated here as jtyp, but is not used in stoda */
/*          except to load miter following a method switch.) */
/*          miter may be reset by stoda. */
/* n      = the number of first-order differential equations. */
/* -----------------------------------------------------------------------
 */
    ls0001_3.kflag = 0;
    told = ls0001_3.tn;
    ncf = 0;
    ls0001_3.ierpj = 0;
    ls0001_3.iersl = 0;
    ls0001_3.jcur = 0;
    ls0001_3.icf = 0;
    delp = 0.;
    if (ls0001_3.jstart > 0) {
	goto L200;
    }
    if (ls0001_3.jstart == -1) {
	goto L100;
    }
    if (ls0001_3.jstart == -2) {
	goto L160;
    }
/* -----------------------------------------------------------------------
 */
/* on the first call, the order is set to 1, and other variables are */
/* initialized.  rmax is the maximum ratio by which h can be increased */
/* in a single step.  it is initially 1.e4 to compensate for the small */
/* initial h, but then is normally equal to 10.  if a failure */
/* occurs (in corrector convergence or error test), rmax is set at 2 */
/* for the next increase. */
/* cfode is called to get the needed coefficients for both methods. */
/* -----------------------------------------------------------------------
 */
    ls0001_3.lmax = ls0001_3.maxord + 1;
    ls0001_3.nq = 1;
    ls0001_3.l = 2;
    ls0001_3.ialth = 2;
    ls0001_3.rmax = 1e4;
    ls0001_3.rc = 0.;
    ls0001_3.el0 = 1.;
    ls0001_3.crate = .7;
    ls0001_3.hold = ls0001_3.h;
    ls0001_3.nslp = 0;
    ls0001_3.ipup = ls0001_3.miter;
    iret = 3;
/* initialize switching parameters.  meth = 1 is assumed initially. ----- 
*/
    lsa001_3.icount = 20;
    lsa001_3.irflag = 0;
    lsa001_3.pdest = 0.;
    lsa001_3.pdlast = 0.;
    lsa001_3.ratio = 5.;
    cfode_(&c__2, ls0001_3.elco, ls0001_3.tesco);
    for (i = 1; i <= 5; ++i) {
/* L10: */
	lsa001_3.cm2[i - 1] = ls0001_3.tesco[i * 3 - 2] * ls0001_3.elco[i + 1 
		+ i * 13 - 14];
    }
    cfode_(&c__1, ls0001_3.elco, ls0001_3.tesco);
    for (i = 1; i <= 12; ++i) {
/* L20: */
	lsa001_3.cm1[i - 1] = ls0001_3.tesco[i * 3 - 2] * ls0001_3.elco[i + 1 
		+ i * 13 - 14];
    }
    goto L150;
/* -----------------------------------------------------------------------
 */
/* the following block handles preliminaries needed when jstart = -1. */
/* ipup is set to miter to force a matrix update. */
/* if an order increase is about to be considered (ialth = 1), */
/* ialth is reset to 2 to postpone consideration one more step. */
/* if the caller has changed meth, cfode is called to reset */
/* the coefficients of the method. */
/* if h is to be changed, yh must be rescaled. */
/* if h or meth is being changed, ialth is reset to l = nq + 1 */
/* to prevent further changes in h for that many steps. */
/* -----------------------------------------------------------------------
 */
L100:
    ls0001_3.ipup = ls0001_3.miter;
    ls0001_3.lmax = ls0001_3.maxord + 1;
    if (ls0001_3.ialth == 1) {
	ls0001_3.ialth = 2;
    }
    if (ls0001_3.meth == lsa001_3.mused) {
	goto L160;
    }
    cfode_(&ls0001_3.meth, ls0001_3.elco, ls0001_3.tesco);
    ls0001_3.ialth = ls0001_3.l;
    iret = 1;
/* -----------------------------------------------------------------------
 */
/* the el vector and related constants are reset */
/* whenever the order nq is changed, or at the start of the problem. */
/* -----------------------------------------------------------------------
 */
L150:
    i_1 = ls0001_3.l;
    for (i = 1; i <= i_1; ++i) {
/* L155: */
	ls0001_3.el[i - 1] = ls0001_3.elco[i + ls0001_3.nq * 13 - 14];
    }
    ls0001_3.nqnyh = ls0001_3.nq * *nyh;
    ls0001_3.rc = ls0001_3.rc * ls0001_3.el[0] / ls0001_3.el0;
    ls0001_3.el0 = ls0001_3.el[0];
    ls0001_3.conit = .5 / (doublereal) (ls0001_3.nq + 2);
    switch ((int)iret) {
	case 1:  goto L160;
	case 2:  goto L170;
	case 3:  goto L200;
    }
/* -----------------------------------------------------------------------
 */
/* if h is being changed, the h ratio rh is checked against */
/* rmax, hmin, and hmxi, and the yh array rescaled.  ialth is set to */
/* l = nq + 1 to prevent a change of h for that many steps, unless */
/* forced by a convergence or error test failure. */
/* -----------------------------------------------------------------------
 */
L160:
    if (ls0001_3.h == ls0001_3.hold) {
	goto L200;
    }
    rh = ls0001_3.h / ls0001_3.hold;
    ls0001_3.h = ls0001_3.hold;
    iredo = 3;
    goto L175;
L170:
/* Computing MAX */
    d_1 = rh, d_2 = ls0001_3.hmin / abs(ls0001_3.h);
    rh = max(d_1,d_2);
L175:
    rh = min(rh,ls0001_3.rmax);
/* Computing MAX */
    d_1 = 1., d_2 = abs(ls0001_3.h) * ls0001_3.hmxi * rh;
    rh /= max(d_1,d_2);
/* -----------------------------------------------------------------------
 */
/* if meth = 1, also restrict the new step size by the stability region. 
*/
/* if this reduces h, set irflag to 1 so that if there are roundoff */
/* problems later, we can assume that is the cause of the trouble. */
/* -----------------------------------------------------------------------
 */
    if (ls0001_3.meth == 2) {
	goto L178;
    }
    lsa001_3.irflag = 0;
/* Computing MAX */
    d_1 = abs(ls0001_3.h) * lsa001_3.pdlast;
    pdh = max(d_1,1e-6);
    if (rh * pdh * 1.00001 < sm1[ls0001_3.nq - 1]) {
	goto L178;
    }
    rh = sm1[ls0001_3.nq - 1] / pdh;
    lsa001_3.irflag = 1;
L178:
    r = 1.;
    i_1 = ls0001_3.l;
    for (j = 2; j <= i_1; ++j) {
	r *= rh;
	i_2 = ls0001_3.n;
	for (i = 1; i <= i_2; ++i) {
/* L180: */
	    yh[i + j * yh_dim1] *= r;
	}
    }
    ls0001_3.h *= rh;
    ls0001_3.rc *= rh;
    ls0001_3.ialth = ls0001_3.l;
    if (iredo == 0) {
	goto L690;
    }
/* -----------------------------------------------------------------------
 */
/* this section computes the predicted values by effectively */
/* multiplying the yh array by the pascal triangle matrix. */
/* rc is the ratio of new to old values of the coefficient  h*el(1). */
/* when rc differs from 1 by more than ccmax, ipup is set to miter */
/* to force pjac to be called, if a jacobian is involved. */
/* in any case, pjac is called at least every msbp steps. */
/* -----------------------------------------------------------------------
 */
L200:
    if ((d_1 = ls0001_3.rc - 1., abs(d_1)) > ls0001_3.ccmax) {
	ls0001_3.ipup = ls0001_3.miter;
    }
    if (ls0001_3.nst >= ls0001_3.nslp + ls0001_3.msbp) {
	ls0001_3.ipup = ls0001_3.miter;
    }
    ls0001_3.tn += ls0001_3.h;
    i1 = ls0001_3.nqnyh + 1;
    i_2 = ls0001_3.nq;
    for (jb = 1; jb <= i_2; ++jb) {
	i1 -= *nyh;
/* dir$ ivdep */
	i_1 = ls0001_3.nqnyh;
	for (i = i1; i <= i_1; ++i) {
/* L210: */
	    yh1[i] += yh1[i + *nyh];
	}
/* L215: */
    }
    pnorm = vmnorm_(&ls0001_3.n, &yh1[1], &ewt[1]);
/* -----------------------------------------------------------------------
 */
/* up to maxcor corrector iterations are taken.  a convergence test is */
/* made on the r.m.s. norm of each correction, weighted by the error */
/* weight vector ewt.  the sum of the corrections is accumulated in the */

/* vector acor(i).  the yh array is not altered in the corrector loop. */
/* -----------------------------------------------------------------------
 */
L220:
    m = 0;
    rate = 0.;
    del = 0.;
    i_2 = ls0001_3.n;
    for (i = 1; i <= i_2; ++i) {
/* L230: */
	y[i] = yh[i + yh_dim1];
    }
    (*f)(&neq[1], &ls0001_3.tn, &y[1], &savf[1]);
    ++ls0001_3.nfe;
    if (ls0001_3.ipup <= 0) {
	goto L250;
    }
/* -----------------------------------------------------------------------
 */
/* if indicated, the matrix p = i - h*el(1)*j is reevaluated and */
/* preprocessed before starting the corrector iteration.  ipup is set */
/* to 0 as an indicator that this has been done. */
/* -----------------------------------------------------------------------
 */
    (*pjac)(&neq[1], &y[1], &yh[yh_offset], nyh, &ewt[1], &acor[1], &savf[1], 
	    &wm[1], &iwm[1], f, jac);
    ls0001_3.ipup = 0;
    ls0001_3.rc = 1.;
    ls0001_3.nslp = ls0001_3.nst;
    ls0001_3.crate = .7;
    if (ls0001_3.ierpj != 0) {
	goto L430;
    }
L250:
    i_2 = ls0001_3.n;
    for (i = 1; i <= i_2; ++i) {
/* L260: */
	acor[i] = 0.;
    }
L270:
    if (ls0001_3.miter != 0) {
	goto L350;
    }
/* -----------------------------------------------------------------------
 */
/* in the case of functional iteration, update y directly from */
/* the result of the last function evaluation. */
/* -----------------------------------------------------------------------
 */
    i_2 = ls0001_3.n;
    for (i = 1; i <= i_2; ++i) {
	savf[i] = ls0001_3.h * savf[i] - yh[i + (yh_dim1 << 1)];
/* L290: */
	y[i] = savf[i] - acor[i];
    }
    del = vmnorm_(&ls0001_3.n, &y[1], &ewt[1]);
    i_2 = ls0001_3.n;
    for (i = 1; i <= i_2; ++i) {
	y[i] = yh[i + yh_dim1] + ls0001_3.el[0] * savf[i];
/* L300: */
	acor[i] = savf[i];
    }
    goto L400;
/* -----------------------------------------------------------------------
 */
/* in the case of the chord method, compute the corrector error, */
/* and solve the linear system with that as right-hand side and */
/* p as coefficient matrix. */
/* -----------------------------------------------------------------------
 */
L350:
    i_2 = ls0001_3.n;
    for (i = 1; i <= i_2; ++i) {
/* L360: */
	y[i] = ls0001_3.h * savf[i] - (yh[i + (yh_dim1 << 1)] + acor[i]);
    }
    (*slvs)(&wm[1], &iwm[1], &y[1], &savf[1]);
    if (ls0001_3.iersl < 0) {
	goto L430;
    }
    if (ls0001_3.iersl > 0) {
	goto L410;
    }
    del = vmnorm_(&ls0001_3.n, &y[1], &ewt[1]);
    i_2 = ls0001_3.n;
    for (i = 1; i <= i_2; ++i) {
	acor[i] += y[i];
/* L380: */
	y[i] = yh[i + yh_dim1] + ls0001_3.el[0] * acor[i];
    }
/* -----------------------------------------------------------------------
 */
/* test for convergence.  if m.gt.0, an estimate of the convergence */
/* rate constant is stored in crate, and this is used in the test. */

/* we first check for a change of iterates that is the size of */
/* roundoff error.  if this occurs, the iteration has converged, and a */
/* new rate estimate is not formed. */
/* in all other cases, force at least two iterations to estimate a */
/* local lipschitz constant estimate for adams methods. */
/* on convergence, form pdest = local maximum lipschitz constant */
/* estimate.  pdlast is the most recent nonzero estimate. */
/* -----------------------------------------------------------------------
 */
L400:
    if (del <= pnorm * 100. * ls0001_3.uround) {
	goto L450;
    }
    if (m == 0 && ls0001_3.meth == 1) {
	goto L405;
    }
    if (m == 0) {
	goto L402;
    }
    rm = 1024.;
    if (del <= delp * 1024.) {
	rm = del / delp;
    }
    rate = max(rate,rm);
/* Computing MAX */
    d_1 = ls0001_3.crate * .2;
    ls0001_3.crate = max(d_1,rm);
L402:
/* Computing MIN */
    d_1 = 1., d_2 = ls0001_3.crate * 1.5;
    dcon = del * min(d_1,d_2) / (ls0001_3.tesco[ls0001_3.nq * 3 - 2] * 
	    ls0001_3.conit);
    if (dcon > 1.) {
	goto L405;
    }
/* Computing MAX */
    d_2 = lsa001_3.pdest, d_3 = rate / (d_1 = ls0001_3.h * ls0001_3.el[0], 
	    abs(d_1));
    lsa001_3.pdest = max(d_2,d_3);
    if (lsa001_3.pdest != 0.) {
	lsa001_3.pdlast = lsa001_3.pdest;
    }
    goto L450;
L405:
    ++m;
    if (m == ls0001_3.maxcor) {
	goto L410;
    }
    if (m >= 2 && del > delp * 2.) {
	goto L410;
    }
    delp = del;
    (*f)(&neq[1], &ls0001_3.tn, &y[1], &savf[1]);
    ++ls0001_3.nfe;
    goto L270;
/* -----------------------------------------------------------------------
 */
/* the corrector iteration failed to converge. */
/* if miter .ne. 0 and the jacobian is out of date, pjac is called for */
/* the next try.  otherwise the yh array is retracted to its values */
/* before prediction, and h is reduced, if possible.  if h cannot be */
/* reduced or mxncf failures have occurred, exit with kflag = -2. */
/* -----------------------------------------------------------------------
 */
L410:
    if (ls0001_3.miter == 0 || ls0001_3.jcur == 1) {
	goto L430;
    }
    ls0001_3.icf = 1;
    ls0001_3.ipup = ls0001_3.miter;
    goto L220;
L430:
    ls0001_3.icf = 2;
    ++ncf;
    ls0001_3.rmax = 2.;
    ls0001_3.tn = told;
    i1 = ls0001_3.nqnyh + 1;
    i_2 = ls0001_3.nq;
    for (jb = 1; jb <= i_2; ++jb) {
	i1 -= *nyh;
/* dir$ ivdep */
	i_1 = ls0001_3.nqnyh;
	for (i = i1; i <= i_1; ++i) {
/* L440: */
	    yh1[i] -= yh1[i + *nyh];
	}
/* L445: */
    }
    if (ls0001_3.ierpj < 0 || ls0001_3.iersl < 0) {
	goto L680;
    }
    if (abs(ls0001_3.h) <= ls0001_3.hmin * 1.00001) {
	goto L670;
    }
    if (ncf == ls0001_3.mxncf) {
	goto L670;
    }
    rh = .25;
    ls0001_3.ipup = ls0001_3.miter;
    iredo = 1;
    goto L170;
/* -----------------------------------------------------------------------
 */
/* the corrector has converged.  jcur is set to 0 */
/* to signal that the jacobian involved may need updating later. */
/* the local error test is made and control passes to statement 500 */
/* if it fails. */
/* -----------------------------------------------------------------------
 */
L450:
    ls0001_3.jcur = 0;
    if (m == 0) {
	dsm = del / ls0001_3.tesco[ls0001_3.nq * 3 - 2];
    }
    if (m > 0) {
	dsm = vmnorm_(&ls0001_3.n, &acor[1], &ewt[1]) / ls0001_3.tesco[
		ls0001_3.nq * 3 - 2];
    }
    if (dsm > 1.) {
	goto L500;
    }
/* -----------------------------------------------------------------------
 */
/* after a successful step, update the yh array. */
/* decrease icount by 1, and if it is -1, consider switching methods. */
/* if a method switch is made, reset various parameters, */
/* rescale the yh array, and exit.  if there is no switch, */
/* consider changing h if ialth = 1.  otherwise decrease ialth by 1. */
/* if ialth is then 1 and nq .lt. maxord, then acor is saved for */
/* use in a possible order increase on the next step. */
/* if a change in h is considered, an increase or decrease in order */
/* by one is considered also.  a change in h is made only if it is by a */

/* factor of at least 1.1.  if not, ialth is set to 3 to prevent */
/* testing for that many steps. */
/* -----------------------------------------------------------------------
 */
    ls0001_3.kflag = 0;
    iredo = 0;
    ++ls0001_3.nst;
    ls0001_3.hu = ls0001_3.h;
    ls0001_3.nqu = ls0001_3.nq;
    lsa001_3.mused = ls0001_3.meth;
    i_2 = ls0001_3.l;
    for (j = 1; j <= i_2; ++j) {
	i_1 = ls0001_3.n;
	for (i = 1; i <= i_1; ++i) {
/* L460: */
	    yh[i + j * yh_dim1] += ls0001_3.el[j - 1] * acor[i];
	}
    }
    --lsa001_3.icount;
    if (lsa001_3.icount >= 0) {
	goto L488;
    }
    if (ls0001_3.meth == 2) {
	goto L480;
    }
/* -----------------------------------------------------------------------
 */
/* we are currently using an adams method.  consider switching to bdf. */
/* if the current order is greater than 5, assume the problem is */
/* not stiff, and skip this section. */
/* if the lipschitz constant and error estimate are not polluted */
/* by roundoff, go to 470 and perform the usual test. */
/* otherwise, switch to the bdf methods if the last step was */
/* restricted to insure stability (irflag = 1), and stay with adams */
/* method if not.  when switching to bdf with polluted error estimates, */

/* in the absence of other information, double the step size. */

/* when the estimates are ok, we make the usual test by computing */
/* the step size we could have (ideally) used on this step, */
/* with the current (adams) method, and also that for the bdf. */
/* if nq .gt. mxords, we consider changing to order mxords on switching. 
*/
/* compare the two step sizes to decide whether to switch. */
/* the step size advantage must be at least ratio = 5 to switch. */
/* -----------------------------------------------------------------------
 */
    if (ls0001_3.nq > 5) {
	goto L488;
    }
    if (dsm > pnorm * 100. * ls0001_3.uround && lsa001_3.pdest != 0.) {
	goto L470;
    }
    if (lsa001_3.irflag == 0) {
	goto L488;
    }
    rh2 = 2.;
    nqm2 = min(ls0001_3.nq,lsa001_3.mxords);
    goto L478;
L470:
    exsm = 1. / (doublereal) ls0001_3.l;
    rh1 = 1. / (pow_dd(&dsm, &exsm) * 1.2 + 1.2e-6);
    rh1it = rh1 * 2.;
    pdh = lsa001_3.pdlast * abs(ls0001_3.h);
    if (pdh * rh1 > 1e-5) {
	rh1it = sm1[ls0001_3.nq - 1] / pdh;
    }
    rh1 = min(rh1,rh1it);
    if (ls0001_3.nq <= lsa001_3.mxords) {
	goto L474;
    }
    nqm2 = lsa001_3.mxords;
    lm2 = lsa001_3.mxords + 1;
    exm2 = 1. / (doublereal) lm2;
    lm2p1 = lm2 + 1;
    dm2 = vmnorm_(&ls0001_3.n, &yh[lm2p1 * yh_dim1 + 1], &ewt[1]) / 
	    lsa001_3.cm2[lsa001_3.mxords - 1];
    rh2 = 1. / (pow_dd(&dm2, &exm2) * 1.2 + 1.2e-6);
    goto L476;
L474:
    dm2 = dsm * (lsa001_3.cm1[ls0001_3.nq - 1] / lsa001_3.cm2[ls0001_3.nq - 1]
	    );
    rh2 = 1. / (pow_dd(&dm2, &exsm) * 1.2 + 1.2e-6);
    nqm2 = ls0001_3.nq;
L476:
    if (rh2 < lsa001_3.ratio * rh1) {
	goto L488;
    }
/* the switch test passed.  reset relevant quantities for bdf. ---------- 
*/
L478:
    rh = rh2;
    lsa001_3.icount = 20;
    ls0001_3.meth = 2;
    ls0001_3.miter = lsa001_3.jtyp;
    lsa001_3.pdlast = 0.;
    ls0001_3.nq = nqm2;
    ls0001_3.l = ls0001_3.nq + 1;
    goto L170;
/* -----------------------------------------------------------------------
 */
/* we are currently using a bdf method.  consider switching to adams. */
/* compute the step size we could have (ideally) used on this step, */
/* with the current (bdf) method, and also that for the adams. */
/* if nq .gt. mxordn, we consider changing to order mxordn on switching. 
*/
/* compare the two step sizes to decide whether to switch. */
/* the step size advantage must be at least 5/ratio = 1 to switch. */
/* if the step size for adams would be so small as to cause */
/* roundoff pollution, we stay with bdf. */
/* -----------------------------------------------------------------------
 */
L480:
    exsm = 1. / (doublereal) ls0001_3.l;
    if (lsa001_3.mxordn >= ls0001_3.nq) {
	goto L484;
    }
    nqm1 = lsa001_3.mxordn;
    lm1 = lsa001_3.mxordn + 1;
    exm1 = 1. / (doublereal) lm1;
    lm1p1 = lm1 + 1;
    dm1 = vmnorm_(&ls0001_3.n, &yh[lm1p1 * yh_dim1 + 1], &ewt[1]) / 
	    lsa001_3.cm1[lsa001_3.mxordn - 1];
    rh1 = 1. / (pow_dd(&dm1, &exm1) * 1.2 + 1.2e-6);
    goto L486;
L484:
    dm1 = dsm * (lsa001_3.cm2[ls0001_3.nq - 1] / lsa001_3.cm1[ls0001_3.nq - 1]
	    );
    rh1 = 1. / (pow_dd(&dm1, &exsm) * 1.2 + 1.2e-6);
    nqm1 = ls0001_3.nq;
    exm1 = exsm;
L486:
    rh1it = rh1 * 2.;
    pdh = lsa001_3.pdnorm * abs(ls0001_3.h);
    if (pdh * rh1 > 1e-5) {
	rh1it = sm1[nqm1 - 1] / pdh;
    }
    rh1 = min(rh1,rh1it);
    rh2 = 1. / (pow_dd(&dsm, &exsm) * 1.2 + 1.2e-6);
    if (rh1 * lsa001_3.ratio < rh2 * 5.) {
	goto L488;
    }
    alpha = max(.001,rh1);
    dm1 = pow_dd(&alpha, &exm1) * dm1;
    if (dm1 <= ls0001_3.uround * 1e3 * pnorm) {
	goto L488;
    }
/* the switch test passed.  reset relevant quantities for adams. -------- 
*/
    rh = rh1;
    lsa001_3.icount = 20;
    ls0001_3.meth = 1;
    ls0001_3.miter = 0;
    lsa001_3.pdlast = 0.;
    ls0001_3.nq = nqm1;
    ls0001_3.l = ls0001_3.nq + 1;
    goto L170;

/* no method switch is being made.  do the usual step/order selection. -- 
*/
L488:
    --ls0001_3.ialth;
    if (ls0001_3.ialth == 0) {
	goto L520;
    }
    if (ls0001_3.ialth > 1) {
	goto L700;
    }
    if (ls0001_3.l == ls0001_3.lmax) {
	goto L700;
    }
    i_1 = ls0001_3.n;
    for (i = 1; i <= i_1; ++i) {
/* L490: */
	yh[i + ls0001_3.lmax * yh_dim1] = acor[i];
    }
    goto L700;
/* -----------------------------------------------------------------------
 */
/* the error test failed.  kflag keeps track of multiple failures. */
/* restore tn and the yh array to their previous values, and prepare */
/* to try the step again.  compute the optimum step size for this or */
/* one lower order.  after 2 or more failures, h is forced to decrease */
/* by a factor of 0.2 or less. */
/* -----------------------------------------------------------------------
 */
L500:
    --ls0001_3.kflag;
    ls0001_3.tn = told;
    i1 = ls0001_3.nqnyh + 1;
    i_1 = ls0001_3.nq;
    for (jb = 1; jb <= i_1; ++jb) {
	i1 -= *nyh;
/* dir$ ivdep */
	i_2 = ls0001_3.nqnyh;
	for (i = i1; i <= i_2; ++i) {
/* L510: */
	    yh1[i] -= yh1[i + *nyh];
	}
/* L515: */
    }
    ls0001_3.rmax = 2.;
    if (abs(ls0001_3.h) <= ls0001_3.hmin * 1.00001) {
	goto L660;
    }
    if (ls0001_3.kflag <= -3) {
	goto L640;
    }
    iredo = 2;
    rhup = 0.;
    goto L540;
/* -----------------------------------------------------------------------
 */
/* regardless of the success or failure of the step, factors */
/* rhdn, rhsm, and rhup are computed, by which h could be multiplied */
/* at order nq - 1, order nq, or order nq + 1, respectively. */
/* in the case of failure, rhup = 0.0 to avoid an order increase. */
/* the largest of these is determined and the new order chosen */
/* accordingly.  if the order is to be increased, we compute one */
/* additional scaled derivative. */
/* -----------------------------------------------------------------------
 */
L520:
    rhup = 0.;
    if (ls0001_3.l == ls0001_3.lmax) {
	goto L540;
    }
    i_1 = ls0001_3.n;
    for (i = 1; i <= i_1; ++i) {
/* L530: */
	savf[i] = acor[i] - yh[i + ls0001_3.lmax * yh_dim1];
    }
    dup = vmnorm_(&ls0001_3.n, &savf[1], &ewt[1]) / ls0001_3.tesco[
	    ls0001_3.nq * 3 - 1];
    exup = 1. / (doublereal) (ls0001_3.l + 1);
    rhup = 1. / (pow_dd(&dup, &exup) * 1.4 + 1.4e-6);
L540:
    exsm = 1. / (doublereal) ls0001_3.l;
    rhsm = 1. / (pow_dd(&dsm, &exsm) * 1.2 + 1.2e-6);
    rhdn = 0.;
    if (ls0001_3.nq == 1) {
	goto L550;
    }
    ddn = vmnorm_(&ls0001_3.n, &yh[ls0001_3.l * yh_dim1 + 1], &ewt[1]) / 
	    ls0001_3.tesco[ls0001_3.nq * 3 - 3];
    exdn = 1. / (doublereal) ls0001_3.nq;
    rhdn = 1. / (pow_dd(&ddn, &exdn) * 1.3 + 1.3e-6);
/* if meth = 1, limit rh according to the stability region also. -------- 
*/
L550:
    if (ls0001_3.meth == 2) {
	goto L560;
    }
/* Computing MAX */
    d_1 = abs(ls0001_3.h) * lsa001_3.pdlast;
    pdh = max(d_1,1e-6);
    if (ls0001_3.l < ls0001_3.lmax) {
/* Computing MIN */
	d_1 = rhup, d_2 = sm1[ls0001_3.l - 1] / pdh;
	rhup = min(d_1,d_2);
    }
/* Computing MIN */
    d_1 = rhsm, d_2 = sm1[ls0001_3.nq - 1] / pdh;
    rhsm = min(d_1,d_2);
    if (ls0001_3.nq > 1) {
/* Computing MIN */
	d_1 = rhdn, d_2 = sm1[ls0001_3.nq - 2] / pdh;
	rhdn = min(d_1,d_2);
    }
    lsa001_3.pdest = 0.;
L560:
    if (rhsm >= rhup) {
	goto L570;
    }
    if (rhup > rhdn) {
	goto L590;
    }
    goto L580;
L570:
    if (rhsm < rhdn) {
	goto L580;
    }
    newq = ls0001_3.nq;
    rh = rhsm;
    goto L620;
L580:
    newq = ls0001_3.nq - 1;
    rh = rhdn;
    if (ls0001_3.kflag < 0 && rh > 1.) {
	rh = 1.;
    }
    goto L620;
L590:
    newq = ls0001_3.l;
    rh = rhup;
    if (rh < 1.1) {
	goto L610;
    }
    r = ls0001_3.el[ls0001_3.l - 1] / (doublereal) ls0001_3.l;
    i_1 = ls0001_3.n;
    for (i = 1; i <= i_1; ++i) {
/* L600: */
	yh[i + (newq + 1) * yh_dim1] = acor[i] * r;
    }
    goto L630;
L610:
    ls0001_3.ialth = 3;
    goto L700;
/* if meth = 1 and h is restricted by stability, bypass 10 percent test. 
*/
L620:
    if (ls0001_3.meth == 2) {
	goto L622;
    }
    if (rh * pdh * 1.00001 >= sm1[newq - 1]) {
	goto L625;
    }
L622:
    if (ls0001_3.kflag == 0 && rh < 1.1) {
	goto L610;
    }
L625:
    if (ls0001_3.kflag <= -2) {
	rh = min(rh,.2);
    }
/* -----------------------------------------------------------------------
 */
/* if there is a change of order, reset nq, l, and the coefficients. */
/* in any case h is reset according to rh and the yh array is rescaled. */

/* then exit from 690 if the step was ok, or redo the step otherwise. */
/* -----------------------------------------------------------------------
 */
    if (newq == ls0001_3.nq) {
	goto L170;
    }
L630:
    ls0001_3.nq = newq;
    ls0001_3.l = ls0001_3.nq + 1;
    iret = 2;
    goto L150;
/* -----------------------------------------------------------------------
 */
/* control reaches this section if 3 or more failures have occured. */
/* if 10 failures have occurred, exit with kflag = -1. */
/* it is assumed that the derivatives that have accumulated in the */
/* yh array have errors of the wrong order.  hence the first */
/* derivative is recomputed, and the order is set to 1.  then */
/* h is reduced by a factor of 10, and the step is retried, */
/* until it succeeds or h reaches hmin. */
/* -----------------------------------------------------------------------
 */
L640:
    if (ls0001_3.kflag == -10) {
	goto L660;
    }
    rh = .1;
/* Computing MAX */
    d_1 = ls0001_3.hmin / abs(ls0001_3.h);
    rh = max(d_1,rh);
    ls0001_3.h *= rh;
    i_1 = ls0001_3.n;
    for (i = 1; i <= i_1; ++i) {
/* L645: */
	y[i] = yh[i + yh_dim1];
    }
    (*f)(&neq[1], &ls0001_3.tn, &y[1], &savf[1]);
    ++ls0001_3.nfe;
    i_1 = ls0001_3.n;
    for (i = 1; i <= i_1; ++i) {
/* L650: */
	yh[i + (yh_dim1 << 1)] = ls0001_3.h * savf[i];
    }
    ls0001_3.ipup = ls0001_3.miter;
    ls0001_3.ialth = 5;
    if (ls0001_3.nq == 1) {
	goto L200;
    }
    ls0001_3.nq = 1;
    ls0001_3.l = 2;
    iret = 3;
    goto L150;
/* -----------------------------------------------------------------------
 */
/* all returns are made through this section.  h is saved in hold */
/* to allow the caller to change h on the next step. */
/* -----------------------------------------------------------------------
 */
L660:
    ls0001_3.kflag = -1;
    goto L720;
L670:
    ls0001_3.kflag = -2;
    goto L720;
L680:
    ls0001_3.kflag = -3;
    goto L720;
L690:
    ls0001_3.rmax = 10.;
L700:
    r = 1. / ls0001_3.tesco[ls0001_3.nqu * 3 - 2];
    i_1 = ls0001_3.n;
    for (i = 1; i <= i_1; ++i) {
/* L710: */
	acor[i] *= r;
    }
L720:
    ls0001_3.hold = ls0001_3.h;
    ls0001_3.jstart = 1;
    return 0;
/* ----------------------- end of subroutine stoda -----------------------
 */
} /* stoda_ */

doublereal vmnorm_(n, v, w)
integer *n;
doublereal *v, *w;
{
    /* System generated locals */
    integer i_1;
    doublereal ret_val, d_1, d_2, d_3;

    /* Local variables */
    static integer i;
    static doublereal vm;

/* lll. optimize */
/* -----------------------------------------------------------------------
 */
/* this function routine computes the weighted max-norm */
/* of the vector of length n contained in the array v, with weights */
/* contained in the array w of length n.. */
/*   vmnorm = max(i=1,...,n) abs(v(i))*w(i) */
/* -----------------------------------------------------------------------
 */
    /* Parameter adjustments */
    --w;
    --v;

    /* Function Body */
    vm = 0.;
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
/* L10: */
/* Computing MAX */
	d_2 = vm, d_3 = (d_1 = v[i], abs(d_1)) * w[i];
	vm = max(d_2,d_3);
    }
    ret_val = vm;
    return ret_val;
/* ----------------------- end of function vmnorm ------------------------
 */
} /* vmnorm_ */

/* Subroutine */ int xerrwv_(msg, nmes, nerr, level, ni, i1, i2, nr, r1, r2, unused)
const char *msg;
integer *nmes;
integer *nerr;
integer *level, *ni, *i1, *i2, *nr;
doublereal *r1, *r2;
integer unused;
{
    /* Initialized data */

    static integer ncpw = 4;

    /* Format strings */
    static char fmt_10[] = "(1x,15a4)";
    static char fmt_20[] = "(6x,\002in above message,  i1 =\002,i10)";
    static char fmt_30[] = "(6x,\002in above message,  i1 =\002,i10,3x,\002i\
2 =\002,i10)";
    static char fmt_40[] = "(6x,\002in above message,  r1 =\002,d21.13)";
    static char fmt_50[] = "(6x,\002in above,  r1 =\002,d21.13,3x,\002r2 \
=\002,d21.13)";

    /* System generated locals */
    integer i_1;

    /* Builtin functions */
    integer s_wsfe(), do_fio(), e_wsfe();
    /* Subroutine */ int s_stop();

    /* Local variables */
    static integer nwds, i, nch, lun;

    /* Fortran I/O blocks */
    static cilist io__182 = { 0, 0, 0, fmt_10, 0 };
    static cilist io__184 = { 0, 0, 0, fmt_20, 0 };
    static cilist io__185 = { 0, 0, 0, fmt_30, 0 };
    static cilist io__186 = { 0, 0, 0, fmt_40, 0 };
    static cilist io__187 = { 0, 0, 0, fmt_50, 0 };


/* -----------------------------------------------------------------------
 */
/* subroutines xerrwv, xsetf, and xsetun, as given here, constitute */
/* a simplified version of the slatec error handling package. */
/* written by a. c. hindmarsh at llnl.  version of march 30, 1987. */
/* this version is in double precision. */

/* all arguments are input arguments. */

/* msg    = the message (hollerith literal or integer array). */
/* nmes   = the length of msg (number of characters). */
/* nerr   = the error number (not used). */
/* level  = the error level.. */
/*          0 or 1 means recoverable (control returns to caller). */
/*          2 means fatal (run is aborted--see note below). */
/* ni     = number of integers (0, 1, or 2) to be printed with message. */

/* i1,i2  = integers to be printed, depending on ni. */
/* nr     = number of reals (0, 1, or 2) to be printed with message. */
/* r1,r2  = reals to be printed, depending on nr. */

/* note..  this routine is machine-dependent and specialized for use */
/* in limited context, in the following ways.. */
/* 1. the number of hollerith characters stored per word, denoted */
/*    by ncpw below, is a data-loaded constant. */
/* 2. the value of nmes is assumed to be at most 60. */
/*    (multi-line messages are generated by repeated calls.) */
/* 3. if level = 2, control passes to the statement   stop */
/*    to abort the run.  this statement may be machine-dependent. */
/* 4. r1 and r2 are assumed to be in double precision and are printed */
/*    in d21.13 format. */
/* 5. the common block /eh0001/ below is data-loaded (a machine- */
/*    dependent feature) with default values. */
/*    this block is needed for proper retention of parameters used by */
/*    this routine which the user can reset by calling xsetf or xsetun. */

/*    the variables in this block are as follows.. */
/*       mesflg = print control flag.. */
/*                1 means print all messages (the default). */
/*                0 means no printing. */
/*       lunit  = logical unit number for messages. */
/*                the default is 6 (machine-dependent). */
/* -----------------------------------------------------------------------
 */
/* the following are instructions for installing this routine */
/* in different machine environments. */

/* to change the default output unit, change the data statement */
/* in the block data subprogram below. */

/* for a different number of characters per word, change the */
/* data statement setting ncpw below, and format 10.  alternatives for */
/* various computers are shown in comment cards. */

/* for a different run-abort command, change the statement following */
/* statement 100 at the end. */
/* -----------------------------------------------------------------------
 */
/* -----------------------------------------------------------------------
 */
/* the following data-loaded value of ncpw is valid for the cdc-6600 */
/* and cdc-7600 computers. */
/*     data ncpw/10/ */
/* the following is valid for the cray-1 computer. */
/*     data ncpw/8/ */
/* the following is valid for the burroughs 6700 and 7800 computers. */
/*     data ncpw/6/ */
/* the following is valid for the pdp-10 computer. */
/*     data ncpw/5/ */
/* the following is valid for the vax computer with 4 bytes per integer, 
*/
/* and for the ibm-360, ibm-370, ibm-303x, and ibm-43xx computers. */
    /* Parameter adjustments */
    --msg;

    /* Function Body */
/* the following is valid for the pdp-11, or vax with 2-byte integers. */
/*     data ncpw/2/ */
/* -----------------------------------------------------------------------
 */
    if (eh0001_1.mesflg == 0) {
	goto L100;
    }
/* get logical unit number. --------------------------------------------- 
*/
    lun = eh0001_1.lunit;
/* get number of words in message. -------------------------------------- 
*/
    nch = min(*nmes,60);
    nwds = nch / ncpw;
    if (nch != nwds * ncpw) {
	++nwds;
    }
/* write the message. --------------------------------------------------- 
*/
    io__182.ciunit = lun;
    s_wsfe(&io__182);
    i_1 = nwds;
    for (i = 1; i <= i_1; ++i) {
	do_fio(&c__1, (char *)&msg[i], (ftnlen)sizeof(integer));
    }
    e_wsfe();
/* -----------------------------------------------------------------------
 */
/* the following format statement is to have the form */
/* 10  format(1x,mmann) */
/* where nn = ncpw and mm is the smallest integer .ge. 60/ncpw. */
/* the following is valid for ncpw = 10. */
/* 10  format(1x,6a10) */
/* the following is valid for ncpw = 8. */
/* 10  format(1x,8a8) */
/* the following is valid for ncpw = 6. */
/* 10  format(1x,10a6) */
/* the following is valid for ncpw = 5. */
/* 10  format(1x,12a5) */
/* the following is valid for ncpw = 4. */
/* the following is valid for ncpw = 2. */
/* 10  format(1x,30a2) */
/* -----------------------------------------------------------------------
 */
    if (*ni == 1) {
	io__184.ciunit = lun;
	s_wsfe(&io__184);
	do_fio(&c__1, (char *)&(*i1), (ftnlen)sizeof(integer));
	e_wsfe();
    }
    if (*ni == 2) {
	io__185.ciunit = lun;
	s_wsfe(&io__185);
	do_fio(&c__1, (char *)&(*i1), (ftnlen)sizeof(integer));
	do_fio(&c__1, (char *)&(*i2), (ftnlen)sizeof(integer));
	e_wsfe();
    }
    if (*nr == 1) {
	io__186.ciunit = lun;
	s_wsfe(&io__186);
	do_fio(&c__1, (char *)&(*r1), (ftnlen)sizeof(doublereal));
	e_wsfe();
    }
    if (*nr == 2) {
	io__187.ciunit = lun;
	s_wsfe(&io__187);
	do_fio(&c__1, (char *)&(*r1), (ftnlen)sizeof(doublereal));
	do_fio(&c__1, (char *)&(*r2), (ftnlen)sizeof(doublereal));
	e_wsfe();
    }
/* abort the run if level = 2. ------------------------------------------ 
*/
L100:
    if (*level != 2) {
	return 0;
    }
    s_stop("", 0L);
    return 0;
/* ----------------------- end of subroutine xerrwv ----------------------
 */
} /* xerrwv_ */

/* -----------------------------------------------------------------------
 */
/* this data subprogram loads variables into the internal common */
/* blocks used by the odepack solvers.  the variables are */
/* defined as follows.. */
/*   illin  = counter for the number of consecutive times the package */
/*            was called with illegal input.  the run is stopped when */
/*            illin reaches 5. */
/*   ntrep  = counter for the number of consecutive times the package */
/*            was called with istate = 1 and tout = t.  the run is */
/*            stopped when ntrep reaches 5. */
/*   mesflg = flag to control printing of error messages.  1 means print, 
*/
/*            0 means no printing. */
/*   lunit  = default value of logical unit number for printing of error 
*/
/*            messages. */
/* -----------------------------------------------------------------------
 */

/* ----------------------- end of block data -----------------------------
 */

doublereal bnorm_(n, a, nra, ml, mu, w)
integer *n;
doublereal *a;
integer *nra, *ml, *mu;
doublereal *w;
{
    /* System generated locals */
    integer a_dim1, a_offset, i_1, i_2;
    doublereal ret_val, d_1, d_2;

    /* Local variables */
    static integer i, j, i1;
    static doublereal an;
    static integer jhi, jlo;
    static doublereal sum;

/* lll. optimize */
/* -----------------------------------------------------------------------
 */
/* this function computes the norm of a banded n by n matrix, */
/* stored in the array a, that is consistent with the weighted max-norm */

/* on vectors, with weights stored in the array w. */
/* ml and mu are the lower and upper half-bandwidths of the matrix. */
/* nra is the first dimension of the a array, nra .ge. ml+mu+1. */
/* in terms of the matrix elements a(i,j), the norm is given by.. */
/*   bnorm = max(i=1,...,n) ( w(i) * sum(j=1,...,n) abs(a(i,j))/w(j) ) */
/* -----------------------------------------------------------------------
 */
    /* Parameter adjustments */
    --w;
    a_dim1 = *nra;
    a_offset = a_dim1 + 1;
    a -= a_offset;

    /* Function Body */
    an = 0.;
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
	sum = 0.;
	i1 = i + *mu + 1;
/* Computing MAX */
	i_2 = i - *ml;
	jlo = max(i_2,1);
/* Computing MIN */
	i_2 = i + *mu;
	jhi = min(i_2,*n);
	i_2 = jhi;
	for (j = jlo; j <= i_2; ++j) {
/* L10: */
	    sum += (d_1 = a[i1 - j + j * a_dim1], abs(d_1)) / w[j];
	}
/* Computing MAX */
	d_1 = an, d_2 = sum * w[i];
	an = max(d_1,d_2);
/* L20: */
    }
    ret_val = an;
    return ret_val;
/* ----------------------- end of function bnorm -------------------------
 */
} /* bnorm_ */

/* Subroutine */ int cfode_(meth, elco, tesco)
integer *meth;
doublereal *elco, *tesco;
{
    /* System generated locals */
    integer i_1;

    /* Local variables */
    static doublereal ragq, pint, xpin, fnqm1;
    static integer i;
    static doublereal agamq, rqfac, tsign, rq1fac;
    static integer ib;
    static doublereal pc[12];
    static integer nq;
    static doublereal fnq;
    static integer nqm1, nqp1;

/* lll. optimize */
/* -----------------------------------------------------------------------
 */
/* cfode is called by the integrator routine to set coefficients */
/* needed there.  the coefficients for the current method, as */
/* given by the value of meth, are set for all orders and saved. */
/* the maximum order assumed here is 12 if meth = 1 and 5 if meth = 2. */
/* (a smaller value of the maximum order is also allowed.) */
/* cfode is called once at the beginning of the problem, */
/* and is not called again unless and until meth is changed. */

/* the elco array contains the basic method coefficients. */
/* the coefficients el(i), 1 .le. i .le. nq+1, for the method of */
/* order nq are stored in elco(i,nq).  they are given by a genetrating */
/* polynomial, i.e., */
/*     l(x) = el(1) + el(2)*x + ... + el(nq+1)*x**nq. */
/* for the implicit adams methods, l(x) is given by */
/*     dl/dx = (x+1)*(x+2)*...*(x+nq-1)/factorial(nq-1),    l(-1) = 0. */
/* for the bdf methods, l(x) is given by */
/*     l(x) = (x+1)*(x+2)* ... *(x+nq)/k, */
/* where         k = factorial(nq)*(1 + 1/2 + ... + 1/nq). */

/* the tesco array contains test constants used for the */
/* local error test and the selection of step size and/or order. */
/* at order nq, tesco(k,nq) is used for the selection of step */
/* size at order nq - 1 if k = 1, at order nq if k = 2, and at order */
/* nq + 1 if k = 3. */
/* -----------------------------------------------------------------------
 */

    /* Parameter adjustments */
    tesco -= 4;
    elco -= 14;

    /* Function Body */
    switch ((int)*meth) {
	case 1:  goto L100;
	case 2:  goto L200;
    }

L100:
    elco[14] = 1.;
    elco[15] = 1.;
    tesco[4] = 0.;
    tesco[5] = 2.;
    tesco[7] = 1.;
    tesco[39] = 0.;
    pc[0] = 1.;
    rqfac = 1.;
    for (nq = 2; nq <= 12; ++nq) {
/* ------------------------------------------------------------------
----- */
/* the pc array will contain the coefficients of the polynomial */
/*     p(x) = (x+1)*(x+2)*...*(x+nq-1). */
/* initially, p(x) = 1. */
/* ------------------------------------------------------------------
----- */
	rq1fac = rqfac;
	rqfac /= (doublereal) nq;
	nqm1 = nq - 1;
	fnqm1 = (doublereal) nqm1;
	nqp1 = nq + 1;
/* form coefficients of p(x)*(x+nq-1). ------------------------------
---- */
	pc[nq - 1] = 0.;
	i_1 = nqm1;
	for (ib = 1; ib <= i_1; ++ib) {
	    i = nqp1 - ib;
/* L110: */
	    pc[i - 1] = pc[i - 2] + fnqm1 * pc[i - 1];
	}
	pc[0] = fnqm1 * pc[0];
/* compute integral, -1 to 0, of p(x) and x*p(x). -------------------
---- */
	pint = pc[0];
	xpin = pc[0] / 2.;
	tsign = 1.;
	i_1 = nq;
	for (i = 2; i <= i_1; ++i) {
	    tsign = -tsign;
	    pint += tsign * pc[i - 1] / (doublereal) i;
/* L120: */
	    xpin += tsign * pc[i - 1] / (doublereal) (i + 1);
	}
/* store coefficients in elco and tesco. ----------------------------
---- */
	elco[nq * 13 + 1] = pint * rq1fac;
	elco[nq * 13 + 2] = 1.;
	i_1 = nq;
	for (i = 2; i <= i_1; ++i) {
/* L130: */
	    elco[i + 1 + nq * 13] = rq1fac * pc[i - 1] / (doublereal) i;
	}
	agamq = rqfac * xpin;
	ragq = 1. / agamq;
	tesco[nq * 3 + 2] = ragq;
	if (nq < 12) {
	    tesco[nqp1 * 3 + 1] = ragq * rqfac / (doublereal) nqp1;
	}
	tesco[nqm1 * 3 + 3] = ragq;
/* L140: */
    }
    return 0;

L200:
    pc[0] = 1.;
    rq1fac = 1.;
    for (nq = 1; nq <= 5; ++nq) {
/* ------------------------------------------------------------------
----- */
/* the pc array will contain the coefficients of the polynomial */
/*     p(x) = (x+1)*(x+2)*...*(x+nq). */
/* initially, p(x) = 1. */
/* ------------------------------------------------------------------
----- */
	fnq = (doublereal) nq;
	nqp1 = nq + 1;
/* form coefficients of p(x)*(x+nq). --------------------------------
---- */
	pc[nqp1 - 1] = 0.;
	i_1 = nq;
	for (ib = 1; ib <= i_1; ++ib) {
	    i = nq + 2 - ib;
/* L210: */
	    pc[i - 1] = pc[i - 2] + fnq * pc[i - 1];
	}
	pc[0] = fnq * pc[0];
/* store coefficients in elco and tesco. ----------------------------
---- */
	i_1 = nqp1;
	for (i = 1; i <= i_1; ++i) {
/* L220: */
	    elco[i + nq * 13] = pc[i - 1] / pc[1];
	}
	elco[nq * 13 + 2] = 1.;
	tesco[nq * 3 + 1] = rq1fac;
	tesco[nq * 3 + 2] = (doublereal) nqp1 / elco[nq * 13 + 1];
	tesco[nq * 3 + 3] = (doublereal) (nq + 2) / elco[nq * 13 + 1];
	rq1fac /= fnq;
/* L230: */
    }
    return 0;
/* ----------------------- end of subroutine cfode -----------------------
 */
} /* cfode_ */

/* Subroutine */ int ewset_(n, itol, rtol, atol, ycur, ewt)
integer *n, *itol;
doublereal *rtol, *atol, *ycur, *ewt;
{
    /* System generated locals */
    integer i_1;
    doublereal d_1;

    /* Local variables */
    static integer i;

/* lll. optimize */
/* -----------------------------------------------------------------------
 */
/* this subroutine sets the error weight vector ewt according to */
/*     ewt(i) = rtol(i)*abs(ycur(i)) + atol(i),  i = 1,...,n, */
/* with the subscript on rtol and/or atol possibly replaced by 1 above, */

/* depending on the value of itol. */
/* -----------------------------------------------------------------------
 */

    /* Parameter adjustments */
    --ewt;
    --ycur;
    --atol;
    --rtol;

    /* Function Body */
    switch ((int)*itol) {
	case 1:  goto L10;
	case 2:  goto L20;
	case 3:  goto L30;
	case 4:  goto L40;
    }
L10:
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
/* L15: */
	ewt[i] = rtol[1] * (d_1 = ycur[i], abs(d_1)) + atol[1];
    }
    return 0;
L20:
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
/* L25: */
	ewt[i] = rtol[1] * (d_1 = ycur[i], abs(d_1)) + atol[i];
    }
    return 0;
L30:
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
/* L35: */
	ewt[i] = rtol[i] * (d_1 = ycur[i], abs(d_1)) + atol[1];
    }
    return 0;
L40:
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
/* L45: */
	ewt[i] = rtol[i] * (d_1 = ycur[i], abs(d_1)) + atol[i];
    }
    return 0;
/* ----------------------- end of subroutine ewset -----------------------
 */
} /* ewset_ */

doublereal fnorm_(n, a, w)
integer *n;
doublereal *a, *w;
{
    /* System generated locals */
    integer a_dim1, a_offset, i_1, i_2;
    doublereal ret_val, d_1, d_2;

    /* Local variables */
    static integer i, j;
    static doublereal an, sum;

/* lll. optimize */
/* -----------------------------------------------------------------------
 */
/* this function computes the norm of a full n by n matrix, */
/* stored in the array a, that is consistent with the weighted max-norm */

/* on vectors, with weights stored in the array w.. */
/*   fnorm = max(i=1,...,n) ( w(i) * sum(j=1,...,n) abs(a(i,j))/w(j) ) */
/* -----------------------------------------------------------------------
 */
    /* Parameter adjustments */
    --w;
    a_dim1 = *n;
    a_offset = a_dim1 + 1;
    a -= a_offset;

    /* Function Body */
    an = 0.;
    i_1 = *n;
    for (i = 1; i <= i_1; ++i) {
	sum = 0.;
	i_2 = *n;
	for (j = 1; j <= i_2; ++j) {
/* L10: */
	    sum += (d_1 = a[i + j * a_dim1], abs(d_1)) / w[j];
	}
/* Computing MAX */
	d_1 = an, d_2 = sum * w[i];
	an = max(d_1,d_2);
/* L20: */
    }
    ret_val = an;
    return ret_val;
/* ----------------------- end of function fnorm -------------------------
 */
} /* fnorm_ */

/* Subroutine */ int intdy_(t, k, yh, nyh, dky, iflag)
doublereal *t;
integer *k;
doublereal *yh;
integer *nyh;
doublereal *dky;
integer *iflag;
{
    /* System generated locals */
    integer yh_dim1, yh_offset, i_1, i_2;

    /* Builtin functions */
    double pow_di();

    /* Local variables */
    static doublereal c;
    static integer i, j;
    static doublereal r, s;
    static integer ic, jb, jj;
    static doublereal tp;
    static integer jb2, jj1, jp1;
    extern /* Subroutine */ int xerrwv_();

/* lll. optimize */
/* -----------------------------------------------------------------------
 */
/* intdy computes interpolated values of the k-th derivative of the */
/* dependent variable vector y, and stores it in dky.  this routine */
/* is called within the package with k = 0 and t = tout, but may */
/* also be called by the user for any k up to the current order. */
/* (see detailed instructions in the usage documentation.) */
/* -----------------------------------------------------------------------
 */
/* the computed values in dky are gotten by interpolation using the */
/* nordsieck history array yh.  this array corresponds uniquely to a */
/* vector-valued polynomial of degree nqcur or less, and dky is set */
/* to the k-th derivative of this polynomial at t. */
/* the formula for dky is.. */
/*              q */
/*  dky(i)  =  sum  c(j,k) * (t - tn)**(j-k) * h**(-j) * yh(i,j+1) */
/*             j=k */
/* where  c(j,k) = j*(j-1)*...*(j-k+1), q = nqcur, tn = tcur, h = hcur. */

/* the quantities  nq = nqcur, l = nq+1, n = neq, tn, and h are */
/* communicated by common.  the above sum is done in reverse order. */
/* iflag is returned negative if either k or t is out of bounds. */
/* -----------------------------------------------------------------------
 */
    /* Parameter adjustments */
    --dky;
    yh_dim1 = *nyh;
    yh_offset = yh_dim1 + 1;
    yh -= yh_offset;

    /* Function Body */
    *iflag = 0;
    if (*k < 0 || *k > ls0001_2.nq) {
	goto L80;
    }
    tp = ls0001_2.tn - ls0001_2.hu - ls0001_2.uround * 100. * (ls0001_2.tn + 
	    ls0001_2.hu);
    if ((*t - tp) * (*t - ls0001_2.tn) > 0.) {
	goto L90;
    }

    s = (*t - ls0001_2.tn) / ls0001_2.h;
    ic = 1;
    if (*k == 0) {
	goto L15;
    }
    jj1 = ls0001_2.l - *k;
    i_1 = ls0001_2.nq;
    for (jj = jj1; jj <= i_1; ++jj) {
/* L10: */
	ic *= jj;
    }
L15:
    c = (doublereal) ic;
    i_1 = ls0001_2.n;
    for (i = 1; i <= i_1; ++i) {
/* L20: */
	dky[i] = c * yh[i + ls0001_2.l * yh_dim1];
    }
    if (*k == ls0001_2.nq) {
	goto L55;
    }
    jb2 = ls0001_2.nq - *k;
    i_1 = jb2;
    for (jb = 1; jb <= i_1; ++jb) {
	j = ls0001_2.nq - jb;
	jp1 = j + 1;
	ic = 1;
	if (*k == 0) {
	    goto L35;
	}
	jj1 = jp1 - *k;
	i_2 = j;
	for (jj = jj1; jj <= i_2; ++jj) {
/* L30: */
	    ic *= jj;
	}
L35:
	c = (doublereal) ic;
	i_2 = ls0001_2.n;
	for (i = 1; i <= i_2; ++i) {
/* L40: */
	    dky[i] = c * yh[i + jp1 * yh_dim1] + s * dky[i];
	}
/* L50: */
    }
    if (*k == 0) {
	return 0;
    }
L55:
    i_1 = -(*k);
    r = pow_di(&ls0001_2.h, &i_1);
    i_1 = ls0001_2.n;
    for (i = 1; i <= i_1; ++i) {
/* L60: */
	dky[i] = r * dky[i];
    }
    return 0;

L80:
    xerrwv_("intdy--  k (=i1) illegal      ", &c__30, &c__51, &c__0, &c__1, k,
	     &c__0, &c__0, &c_b136, &c_b136, 0L);
    *iflag = -1;
    return 0;
L90:
    xerrwv_("intdy--  t (=r1) illegal      ", &c__30, &c__52, &c__0, &c__0, &
	    c__0, &c__0, &c__1, t, &c_b136, 0L);
    xerrwv_("      t not in interval tcur - hu (= r1) to tcur (=r2)      ", &
	    c__60, &c__52, &c__0, &c__0, &c__0, &c__0, &c__2, &tp, &
	    ls0001_2.tn, 0L);
    *iflag = -2;
    return 0;
/* ----------------------- end of subroutine intdy -----------------------
 */
} /* intdy_ */

