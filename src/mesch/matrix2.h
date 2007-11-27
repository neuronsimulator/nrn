
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
***************************************************************************/


/*
	Header file for ``matrix2.a'' library file
*/


#ifndef MATRIX2H
#define MATRIX2H

#include "matrix.h"

/* Unless otherwise specified, factorisation routines overwrite the
   matrix that is being factorised */

#ifndef ANSI_C

extern	MAT	*BKPfactor(), *CHfactor(), *LUfactor(), *QRfactor(),
		*QRCPfactor(), *LDLfactor(), *Hfactor(), *MCHfactor(),
		*m_inverse();
extern	double	LUcondest(), QRcondest();
extern	MAT	*makeQ(), *makeR(), *makeHQ(), *makeH();
extern	MAT	*LDLupdate(), *QRupdate();

extern	VEC	*BKPsolve(), *CHsolve(), *LUsolve(), *_Qsolve(), *QRsolve(),
		*LDLsolve(), *Usolve(), *Lsolve(), *Dsolve(), *LTsolve(),
		*UTsolve(), *LUTsolve(), *QRCPsolve();

extern  BAND    *bdLUfactor(), *bdLDLfactor();
extern  VEC     *bdLUsolve(), *bdLDLsolve();

extern	VEC	*hhvec();
extern	VEC	*hhtrvec();
extern	MAT	*hhtrrows();
extern	MAT	*hhtrcols();

extern	void	givens();
extern	VEC	*rot_vec();	/* in situ */
extern	MAT	*rot_rows();	/* in situ */
extern	MAT	*rot_cols();	/* in situ */


/* eigenvalue routines */
extern	VEC	*trieig(), *symmeig();
extern	MAT	*schur();
extern	void	schur_evals();
extern	MAT	*schur_vecs();

/* singular value decomposition */
extern	VEC	*bisvd(), *svd();

/* matrix powers and exponent */
MAT  *_m_pow();
MAT  *m_pow();
MAT  *m_exp(), *_m_exp();
MAT  *m_poly();

/* FFT */
void fft();
void ifft();


#else

                 /* forms Bunch-Kaufman-Parlett factorisation for
                        symmetric indefinite matrices */
extern	MAT	*BKPfactor(MAT *A,PERM *pivot,PERM *blocks),
                 /* Cholesky factorisation of A
                        (symmetric, positive definite) */
		*CHfactor(MAT *A),
                /* LU factorisation of A (with partial pivoting) */ 
                *LUfactor(MAT *A,PERM *pivot),
                /* QR factorisation of A; need dim(diag) >= # rows of A */
		*QRfactor(MAT *A,VEC *diag),
                /* QR factorisation of A with column pivoting */
		*QRCPfactor(MAT *A,VEC *diag,PERM *pivot),
                /* L.D.L^T factorisation of A */
		*LDLfactor(MAT *A), 
                /* Hessenberg factorisation of A -- for schur() */
                *Hfactor(MAT *A,VEC *diag1,VEC *diag2),
                /* modified Cholesky factorisation of A;
                        actually factors A+D, D diagonal with no
                        diagonal entry in the factor < sqrt(tol) */
                *MCHfactor(MAT *A,double tol),
		*m_inverse(MAT *A,MAT *out);

                /* returns condition estimate for A after LUfactor() */
extern	double	LUcondest(MAT *A,PERM *pivot),
                /* returns condition estimate for Q after QRfactor() */
                QRcondest(MAT *A);

/* Note: The make..() and ..update() routines assume that the factorisation
        has already been carried out */

     /* Qout is the "Q" (orthongonal) matrix from QR factorisation */
extern	MAT	*makeQ(MAT *A,VEC *diag,MAT *Qout),
                /* Rout is the "R" (upper triangular) matrix
                        from QR factorisation */
		*makeR(MAT *A,MAT *Rout),
                /* Qout is orthogonal matrix in Hessenberg factorisation */
		*makeHQ(MAT *A,VEC *diag1,VEC *diag2,MAT *Qout),
                /* Hout is the Hessenberg matrix in Hessenberg factorisation */
		*makeH(MAT *A,MAT *Hout);

                /* updates L.D.L^T factorisation for A <- A + alpha.u.u^T */
extern	MAT	*LDLupdate(MAT *A,VEC *u,double alpha),
                /* updates QR factorisation for QR <- Q.(R+u.v^T)
		   Note: we need explicit Q & R matrices,
                        from makeQ() and makeR() */
		*QRupdate(MAT *Q,MAT *R,VEC *u,VEC *v);

/* Solve routines assume that the corresponding factorisation routine
        has already been applied to the matrix along with auxiliary
        objects (such as pivot permutations)

        These solve the system A.x = b,
        except for LUTsolve and QRTsolve which solve the transposed system
                                A^T.x. = b.
        If x is NULL on entry, then it is created.
*/

extern	VEC	*BKPsolve(MAT *A,PERM *pivot,PERM *blocks,VEC *b,VEC *x),
		*CHsolve(MAT *A,VEC *b,VEC *x),
		*LDLsolve(MAT *A,VEC *b,VEC *x),
		*LUsolve(MAT *A,PERM *pivot,VEC *b,VEC *x),
		*_Qsolve(MAT *A,VEC *,VEC *,VEC *, VEC *),
		*QRsolve(MAT *A,VEC *,VEC *b,VEC *x),
    		*QRTsolve(MAT *A,VEC *,VEC *b,VEC *x),


     /* Triangular equations solve routines;
        U for upper triangular, L for lower traingular, D for diagonal
        if diag_val == 0.0 use that values in the matrix */

		*Usolve(MAT *A,VEC *b,VEC *x,double diag_val),
		*Lsolve(MAT *A,VEC *b,VEC *x,double diag_val),
		*Dsolve(MAT *A,VEC *b,VEC *x),
		*LTsolve(MAT *A,VEC *b,VEC *x,double diag_val),
		*UTsolve(MAT *A,VEC *b,VEC *x,double diag_val),
                *LUTsolve(MAT *A,PERM *,VEC *,VEC *),
                *QRCPsolve(MAT *QR,VEC *diag,PERM *pivot,VEC *b,VEC *x);

extern  BAND    *bdLUfactor(BAND *A,PERM *pivot),
                *bdLDLfactor(BAND *A);
extern  VEC     *bdLUsolve(BAND *A,PERM *pivot,VEC *b,VEC *x),
                *bdLDLsolve(BAND *A,VEC *b,VEC *x);



extern	VEC	*hhvec(VEC *,u_int,Real *,VEC *,Real *);
extern	VEC	*hhtrvec(VEC *,double,u_int,VEC *,VEC *);
extern	MAT	*hhtrrows(MAT *,u_int,u_int,VEC *,double);
extern	MAT	*hhtrcols(MAT *,u_int,u_int,VEC *,double);

extern	void	givens(double,double,Real *,Real *);
extern	VEC	*rot_vec(VEC *,u_int,u_int,double,double,VEC *); /* in situ */
extern	MAT	*rot_rows(MAT *,u_int,u_int,double,double,MAT *); /* in situ */
extern	MAT	*rot_cols(MAT *,u_int,u_int,double,double,MAT *); /* in situ */


/* eigenvalue routines */

               /* compute eigenvalues of tridiagonal matrix
                  with diagonal entries a[i], super & sub diagonal entries
                  b[i]; eigenvectors stored in Q (if not NULL) */
extern	VEC	*trieig(VEC *a,VEC *b,MAT *Q),
                 /* sets out to be vector of eigenvectors; eigenvectors
                   stored in Q (if not NULL). A is unchanged */
		*symmeig(MAT *A,MAT *Q,VEC *out);

               /* computes real Schur form = Q^T.A.Q */
extern	MAT	*schur(MAT *A,MAT *Q);
         /* computes real and imaginary parts of the eigenvalues
                        of A after schur() */
extern	void	schur_evals(MAT *A,VEC *re_part,VEC *im_part);
          /* computes real and imaginary parts of the eigenvectors
                        of A after schur() */
extern	MAT	*schur_vecs(MAT *T,MAT *Q,MAT *X_re,MAT *X_im);


/* singular value decomposition */

        /* computes singular values of bi-diagonal matrix with
                   diagonal entries a[i] and superdiagonal entries b[i];
                   singular vectors stored in U and V (if not NULL) */
VEC	*bisvd(VEC *a,VEC *b,MAT *U,MAT *V),
               /* sets out to be vector of singular values;
                   singular vectors stored in U and V */
	*svd(MAT *A,MAT *U,MAT *V,VEC *out);

/* matrix powers and exponent */
MAT  *_m_pow(MAT *,int,MAT *,MAT *);
MAT  *m_pow(MAT *,int, MAT *);
MAT  *m_exp(MAT *,double,MAT *);
MAT  *_m_exp(MAT *,double,MAT *,int *,int *);
MAT  *m_poly(MAT *,VEC *,MAT *);

/* FFT */
void fft(VEC *,VEC *);
void ifft(VEC *,VEC *);

#endif


#endif
