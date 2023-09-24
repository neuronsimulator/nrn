/*
 *  MATRIX SOLVE MODULE
 *
 *  Author:                     Advising professor:
 *      Kenneth S. Kundert          Alberto Sangiovanni-Vincentelli
 *      UC Berkeley
 *
 *  This file contains the forward and backward substitution routines for
 *  the sparse matrix routines.
 *
 *  >>> User accessible functions contained in this file:
 *  spSolve
 *  spSolveTransposed
 *
 *  >>> Other functions contained in this file:
 *  SolveComplexMatrix
 *  SolveComplexTransposedMatrix
 */

/*
 *  Revision and copyright information.
 *
 *  Copyright (c) 1985,86,87,88
 *  by Kenneth S. Kundert and the University of California.
 *
 *  Permission to use, copy, modify, and distribute this software and
 *  its documentation for any purpose and without fee is hereby granted,
 *  provided that the copyright notices appear in all copies and
 *  supporting documentation and that the authors and the University of
 *  California are properly credited.  The authors and the University of
 *  California make no representations as to the suitability of this
 *  software for any purpose.  It is provided `as is', without express
 *  or implied warranty.
 */

#ifndef lint
static char copyright[] = "Sparse1.3: Copyright (c) 1985,86,87,88 by Kenneth S. Kundert";
static char RCSid[] = "@(#)$Header$";
#endif

/*
 *  IMPORTS
 *
 *  >>> Import descriptions:
 *  spconfig.h
 *     Macros that customize the sparse matrix routines.
 *  spmatrix.h
 *     Macros and declarations to be imported by the user.
 *  spdefs.h
 *     Matrix type and macro definitions for the sparse matrix routines.
 */

#define spINSIDE_SPARSE
#include "spconfig.h"
#include "spdefs.h"
#include "spmatrix.h"

/* avoid "declared implicitly `extern' and later `static' " warnings. */
static void SolveComplexMatrix(MatrixPtr Matrix, RealVector RHS, RealVector Solution, std::optional<RealVector> iRHS = std::nullopt, std::optional<RealVector> iSolution = std::nullopt);
static void SolveComplexTransposedMatrix(MatrixPtr Matrix, RealVector RHS, RealVector Solution, std::optional<RealVector> iRHS = std::nullopt, std::optional<RealVector> iSolution = std::nullopt);

/*
 *  SOLVE MATRIX EQUATION
 *
 *  Performs forward elimination and back substitution to find the
 *  unknown vector from the RHS vector and factored matrix.  This
 *  routine assumes that the pivots are associated with the lower
 *  triangular (L) matrix and that the diagonal of the upper triangular
 *  (U) matrix consists of ones.  This routine arranges the computation
 *  in different way than is traditionally used in order to exploit the
 *  sparsity of the right-hand side.  See the reference in spRevision.
 *
 *  >>> Arguments:
 *  Matrix  <input>  (char *)
 *      Pointer to matrix.
 *  RHS  <input>  (RealVector)
 *      RHS is the input data array, the right hand side. This data is
 *      undisturbed and may be reused for other solves.
 *  Solution  <output>  (RealVector)
 *      Solution is the output data array. This routine is constructed such that
 *      RHS and Solution can be the same array.
 *  iRHS  <optional input>  (RealVector)
 *      iRHS is the imaginary portion of the input data array, the right
 *      hand side. This data is undisturbed and may be reused for other solves.
 *      This argument is only necessary if matrix is complex and if
 *      spSEPARATED_COMPLEX_VECTOR is set true.
 *  iSolution  <optional output>  (RealVector)
 *      iSolution is the imaginary portion of the output data array. This
 *      routine is constructed such that iRHS and iSolution can be
 *      the same array.  This argument is only necessary if matrix is complex
 *      and if spSEPARATED_COMPLEX_VECTOR is set true.
 *
 *  >>> Local variables:
 *  Intermediate  (RealVector)
 *      Temporary storage for use in forward elimination and backward
 *      substitution.  Commonly referred to as c, when the LU factorization
 *      equations are given as  Ax = b, Lc = b, Ux = c Local version of
 *      Matrix->Intermediate, which was created during the initial
 *      factorization in function CreateInternalVectors() in the matrix
 *      factorization module.
 *  pElement  (ElementPtr)
 *      Pointer used to address elements in both the lower and upper triangle
 *      matrices.
 *  pExtOrder  (int *)
 *      Pointer used to sequentially access each entry in IntToExtRowMap
 *      and IntToExtColMap arrays.  Used to quickly scramble and unscramble
 *      RHS and Solution to account for row and column interchanges.
 *  pPivot  (ElementPtr)
 *      Pointer that points to current pivot or diagonal element.
 *  Size  (int)
 *      Size of matrix. Made local to reduce indirection.
 *  Temp  (RealNumber)
 *      Temporary storage for entries in arrays.
 */

/*VARARGS3*/
void spSolve(char* eMatrix, RealVector RHS, RealVector Solution, std::optional<RealVector> iRHS, std::optional<RealVector> iSolution)
{
    MatrixPtr Matrix = (MatrixPtr)eMatrix;
    ElementPtr pElement;
    RealVector Intermediate;
    RealNumber Temp;
    int I, *pExtOrder, Size;
    ElementPtr pPivot;

    /* Begin `spSolve'. */
    ASSERT(IS_VALID(Matrix) AND IS_FACTORED(Matrix));

#if spCOMPLEX
    if (Matrix->Complex) {
        SolveComplexMatrix(Matrix, RHS, Solution, iRHS, iSolution);
        return;
    }
#endif

#if REAL
    Intermediate = Matrix->Intermediate;
    Size = Matrix->Size;

/* Correct array pointers for ARRAY_OFFSET. */
#if NOT ARRAY_OFFSET
    --RHS;
    --Solution;
#endif

    /* Initialize Intermediate vector. */
    pExtOrder = &Matrix->IntToExtRowMap[Size];
    for (I = Size; I > 0; I--)
        Intermediate[I] = RHS[*(pExtOrder--)];

    /* Forward elimination. Solves Lc = b.*/
    for (I = 1; I <= Size; I++) {
        /* This step of the elimination is skipped if Temp equals zero. */
        if ((Temp = Intermediate[I]) != 0.0) {
            pPivot = Matrix->Diag[I];
            Intermediate[I] = (Temp *= pPivot->Real);

            pElement = pPivot->NextInCol;
            while (pElement != NULL) {
                Intermediate[pElement->Row] -= Temp * pElement->Real;
                pElement = pElement->NextInCol;
            }
        }
    }

    /* Backward Substitution. Solves Ux = c.*/
    for (I = Size; I > 0; I--) {
        Temp = Intermediate[I];
        pElement = Matrix->Diag[I]->NextInRow;
        while (pElement != NULL) {
            Temp -= pElement->Real * Intermediate[pElement->Col];
            pElement = pElement->NextInRow;
        }
        Intermediate[I] = Temp;
    }

    /* Unscramble Intermediate vector while placing data in to Solution vector. */
    pExtOrder = &Matrix->IntToExtColMap[Size];
    for (I = Size; I > 0; I--)
        Solution[*(pExtOrder--)] = Intermediate[I];

    return;
#endif /* REAL */
}

#if spCOMPLEX
/*
 *  SOLVE COMPLEX MATRIX EQUATION
 *
 *  Performs forward elimination and back substitution to find the
 *  unknown vector from the RHS vector and factored matrix.  This
 *  routine assumes that the pivots are associated with the lower
 *  triangular (L) matrix and that the diagonal of the upper triangular
 *  (U) matrix consists of ones.  This routine arranges the computation
 *  in different way than is traditionally used in order to exploit the
 *  sparsity of the right-hand side.  See the reference in spRevision.
 *
 *  >>> Arguments:
 *  Matrix  <input>  (char *)
 *      Pointer to matrix.
 *  RHS  <input>  (RealVector)
 *      RHS is the real portion of the input data array, the right hand
 *      side. This data is undisturbed and may be reused for other solves.
 *  Solution  <output>  (RealVector)
 *      Solution is the real portion of the output data array. This routine
 *      is constructed such that RHS and Solution can be the same
 *      array.
 *  iRHS  <input>  (RealVector)
 *      iRHS is the imaginary portion of the input data array, the right
 *      hand side. This data is undisturbed and may be reused for other solves.
 *      If spSEPARATED_COMPLEX_VECTOR is set false, there is no need to
 *      supply this array.
 *  iSolution  <output>  (RealVector)
 *      iSolution is the imaginary portion of the output data array. This
 *      routine is constructed such that iRHS and iSolution can be
 *      the same array.  If spSEPARATED_COMPLEX_VECTOR is set false, there is no
 *      need to supply this array.
 *
 *  >>> Local variables:
 *  Intermediate  (ComplexVector)
 *      Temporary storage for use in forward elimination and backward
 *      substitution.  Commonly referred to as c, when the LU factorization
 *      equations are given as  Ax = b, Lc = b, Ux = c.
 *      Local version of Matrix->Intermediate, which was created during
 *      the initial factorization in function CreateInternalVectors() in the
 *      matrix factorization module.
 *  pElement  (ElementPtr)
 *      Pointer used to address elements in both the lower and upper triangle
 *      matrices.
 *  pExtOrder  (int *)
 *      Pointer used to sequentially access each entry in IntToExtRowMap
 *      and IntToExtColMap arrays.  Used to quickly scramble and unscramble
 *      RHS and Solution to account for row and column interchanges.
 *  pPivot  (ElementPtr)
 *      Pointer that points to current pivot or diagonal element.
 *  Size  (int)
 *      Size of matrix. Made local to reduce indirection.
 *  Temp  (ComplexNumber)
 *      Temporary storage for entries in arrays.
 */

static void SolveComplexMatrix(MatrixPtr Matrix, RealVector RHS, RealVector Solution, std::optional<RealVector> iRHS, std::optional<RealVector> iSolution)
{
    ElementPtr pElement;
    ComplexVector Intermediate;
    int I, *pExtOrder, Size;
    ElementPtr pPivot;
    ComplexVector ExtVector;
    ComplexNumber Temp;

    /* Begin `SolveComplexMatrix'. */

    Size = Matrix->Size;
    Intermediate = (ComplexVector)Matrix->Intermediate;

/* Correct array pointers for ARRAY_OFFSET. */
#if NOT ARRAY_OFFSET
#if spSEPARATED_COMPLEX_VECTORS
    --RHS;
    --iRHS;
    --Solution;
    --iSolution;
#else
    RHS -= 2;
    Solution -= 2;
#endif
#endif

    /* Initialize Intermediate vector. */
    pExtOrder = &Matrix->IntToExtRowMap[Size];

#if spSEPARATED_COMPLEX_VECTORS
    for (I = Size; I > 0; I--) {
        Intermediate[I].Real = RHS[*(pExtOrder)];
        ASSERT(iRHS.has_value());
        Intermediate[I].Imag = iRHS.value()[*(pExtOrder--)];
    }
#else
    ExtVector = (ComplexVector)RHS;
    for (I = Size; I > 0; I--)
        Intermediate[I] = ExtVector[*(pExtOrder--)];
#endif

    /* Forward substitution. Solves Lc = b.*/
    for (I = 1; I <= Size; I++) {
        Temp = Intermediate[I];

        /* This step of the substitution is skipped if Temp equals zero. */
        if ((Temp.Real != 0.0) OR(Temp.Imag != 0.0)) {
            pPivot = Matrix->Diag[I];
            /* Cmplx expr: Temp *= (1.0 / Pivot). */
            CMPLX_MULT_ASSIGN(Temp, *pPivot);
            Intermediate[I] = Temp;
            pElement = pPivot->NextInCol;
            while (pElement != NULL) {
                /* Cmplx expr: Intermediate[Element->Row] -= Temp * *Element. */
                CMPLX_MULT_SUBT_ASSIGN(Intermediate[pElement->Row],
                    Temp, *pElement);
                pElement = pElement->NextInCol;
            }
        }
    }

    /* Backward Substitution. Solves Ux = c.*/
    for (I = Size; I > 0; I--) {
        Temp = Intermediate[I];
        pElement = Matrix->Diag[I]->NextInRow;

        while (pElement != NULL) {
            /* Cmplx expr: Temp -= *Element * Intermediate[Element->Col]. */
            CMPLX_MULT_SUBT_ASSIGN(Temp, *pElement, Intermediate[pElement->Col]);
            pElement = pElement->NextInRow;
        }
        Intermediate[I] = Temp;
    }

    /* Unscramble Intermediate vector while placing data in to Solution vector. */
    pExtOrder = &Matrix->IntToExtColMap[Size];

#if spSEPARATED_COMPLEX_VECTORS
    for (I = Size; I > 0; I--) {
        Solution[*(pExtOrder)] = Intermediate[I].Real;
        iSolution.value()[*(pExtOrder--)] = Intermediate[I].Imag;
    }
#else
    ExtVector = (ComplexVector)Solution;
    for (I = Size; I > 0; I--)
        ExtVector[*(pExtOrder--)] = Intermediate[I];
#endif

    return;
}
#endif /* spCOMPLEX */

#if TRANSPOSE
/*
 *  SOLVE TRANSPOSED MATRIX EQUATION
 *
 *  Performs forward elimination and back substitution to find the
 *  unknown vector from the RHS vector and transposed factored
 *  matrix. This routine is useful when performing sensitivity analysis
 *  on a circuit using the adjoint method.  This routine assumes that
 *  the pivots are associated with the untransposed lower triangular
 *  (L) matrix and that the diagonal of the untransposed upper
 *  triangular (U) matrix consists of ones.
 *
 *  >>> Arguments:
 *  Matrix  <input>  (char *)
 *      Pointer to matrix.
 *  RHS  <input>  (RealVector)
 *      RHS is the input data array, the right hand side. This data is
 *      undisturbed and may be reused for other solves.
 *  Solution  <output>  (RealVector)
 *      Solution is the output data array. This routine is constructed such that
 *      RHS and Solution can be the same array.
 *  iRHS  <optional input>  (RealVector)
 *      iRHS is the imaginary portion of the input data array, the right
 *      hand side. This data is undisturbed and may be reused for other solves.
 *      If spSEPARATED_COMPLEX_VECTOR is set false, or if matrix is real, there
 *      is no need to supply this array.
 *  iSolution  <optional output>  (RealVector)
 *      iSolution is the imaginary portion of the output data array. This
 *      routine is constructed such that iRHS and iSolution can be
 *      the same array.  If spSEPARATED_COMPLEX_VECTOR is set false, or if
 *      matrix is real, there is no need to supply this array.
 *
 *  >>> Local variables:
 *  Intermediate  (RealVector)
 *      Temporary storage for use in forward elimination and backward
 *      substitution.  Commonly referred to as c, when the LU factorization
 *      equations are given as  Ax = b, Lc = b, Ux = c.  Local version of
 *      Matrix->Intermediate, which was created during the initial
 *      factorization in function CreateInternalVectors() in the matrix
 *      factorization module.
 *  pElement  (ElementPtr)
 *      Pointer used to address elements in both the lower and upper triangle
 *      matrices.
 *  pExtOrder  (int *)
 *      Pointer used to sequentially access each entry in IntToExtRowMap
 *      and IntToExtRowMap arrays.  Used to quickly scramble and unscramble
 *      RHS and Solution to account for row and column interchanges.
 *  pPivot  (ElementPtr)
 *      Pointer that points to current pivot or diagonal element.
 *  Size  (int)
 *      Size of matrix. Made local to reduce indirection.
 *  Temp  (RealNumber)
 *      Temporary storage for entries in arrays.
 *
 */

/*VARARGS3*/

void spSolveTransposed(char* eMatrix, RealVector RHS, RealVector Solution, std::optional<RealVector> iRHS, std::optional<RealVector> iSolution)
{
    MatrixPtr Matrix = (MatrixPtr)eMatrix;
    ElementPtr pElement;
    RealVector Intermediate;
    int I, *pExtOrder, Size;
    ElementPtr pPivot;
    RealNumber Temp;

    /* Begin `spSolveTransposed'. */
    ASSERT(IS_VALID(Matrix) AND IS_FACTORED(Matrix));

#if spCOMPLEX
    if (Matrix->Complex) {
        SolveComplexTransposedMatrix(Matrix, RHS, Solution, iRHS, iSolution);
        return;
    }
#endif

#if REAL
    Size = Matrix->Size;
    Intermediate = Matrix->Intermediate;

/* Correct array pointers for ARRAY_OFFSET. */
#if NOT ARRAY_OFFSET
    --RHS;
    --Solution;
#endif

    /* Initialize Intermediate vector. */
    pExtOrder = &Matrix->IntToExtColMap[Size];
    for (I = Size; I > 0; I--)
        Intermediate[I] = RHS[*(pExtOrder--)];

    /* Forward elimination. */
    for (I = 1; I <= Size; I++) {
        /* This step of the elimination is skipped if Temp equals zero. */
        if ((Temp = Intermediate[I]) != 0.0) {
            pElement = Matrix->Diag[I]->NextInRow;
            while (pElement != NULL) {
                Intermediate[pElement->Col] -= Temp * pElement->Real;
                pElement = pElement->NextInRow;
            }
        }
    }

    /* Backward Substitution. */
    for (I = Size; I > 0; I--) {
        pPivot = Matrix->Diag[I];
        Temp = Intermediate[I];
        pElement = pPivot->NextInCol;
        while (pElement != NULL) {
            Temp -= pElement->Real * Intermediate[pElement->Row];
            pElement = pElement->NextInCol;
        }
        Intermediate[I] = Temp * pPivot->Real;
    }

    /* Unscramble Intermediate vector while placing data in to Solution vector. */
    pExtOrder = &Matrix->IntToExtRowMap[Size];
    for (I = Size; I > 0; I--)
        Solution[*(pExtOrder--)] = Intermediate[I];

    return;
#endif /* REAL */
}
#endif /* TRANSPOSE */

#if TRANSPOSE AND spCOMPLEX
/*
 *  SOLVE COMPLEX TRANSPOSED MATRIX EQUATION
 *
 *  Performs forward elimination and back substitution to find the
 *  unknown vector from the RHS vector and transposed factored
 *  matrix. This routine is useful when performing sensitivity analysis
 *  on a circuit using the adjoint method.  This routine assumes that
 *  the pivots are associated with the untransposed lower triangular
 *  (L) matrix and that the diagonal of the untransposed upper
 *  triangular (U) matrix consists of ones.
 *
 *  >>> Arguments:
 *  Matrix  <input>  (char *)
 *      Pointer to matrix.
 *  RHS  <input>  (RealVector)
 *      RHS is the input data array, the right hand
 *      side. This data is undisturbed and may be reused for other solves.
 *      This vector is only the real portion if the matrix is complex and
 *      spSEPARATED_COMPLEX_VECTORS is set true.
 *  Solution  <output>  (RealVector)
 *      Solution is the real portion of the output data array. This routine
 *      is constructed such that RHS and Solution can be the same array.
 *      This vector is only the real portion if the matrix is complex and
 *      spSEPARATED_COMPLEX_VECTORS is set true.
 *  iRHS  <optional input>  (RealVector)
 *      iRHS is the imaginary portion of the input data array, the right
 *      hand side. This data is undisturbed and may be reused for other solves.
 *      If either spCOMPLEX or spSEPARATED_COMPLEX_VECTOR is set false, there
 *      is no need to supply this array.
 *  iSolution  <optional output>  (RealVector)
 *      iSolution is the imaginary portion of the output data array. This
 *      routine is constructed such that iRHS and iSolution can be
 *      the same array.  If spCOMPLEX or spSEPARATED_COMPLEX_VECTOR is set
 *      false, there is no need to supply this array.
 *
 *  >>> Local variables:
 *  Intermediate  (ComplexVector)
 *      Temporary storage for use in forward elimination and backward
 *      substitution.  Commonly referred to as c, when the LU factorization
 *      equations are given as  Ax = b, Lc = b, Ux = c.  Local version of
 *      Matrix->Intermediate, which was created during
 *      the initial factorization in function CreateInternalVectors() in the
 *      matrix factorization module.
 *  pElement  (ElementPtr)
 *      Pointer used to address elements in both the lower and upper triangle
 *      matrices.
 *  pExtOrder  (int *)
 *      Pointer used to sequentially access each entry in IntToExtRowMap
 *      and IntToExtColMap arrays.  Used to quickly scramble and unscramble
 *      RHS and Solution to account for row and column interchanges.
 *  pPivot  (ElementPtr)
 *      Pointer that points to current pivot or diagonal element.
 *  Size  (int)
 *      Size of matrix. Made local to reduce indirection.
 *  Temp  (ComplexNumber)
 *      Temporary storage for entries in arrays.
 *
 */

static void
SolveComplexTransposedMatrix(MatrixPtr Matrix, RealVector RHS, RealVector Solution, std::optional<RealVector> iRHS, std::optional<RealVector> iSolution)
{
    ElementPtr pElement;
    ComplexVector Intermediate;
    int I, *pExtOrder, Size;
    ComplexVector ExtVector;
    ElementPtr pPivot;
    ComplexNumber Temp;

    /* Begin `SolveComplexTransposedMatrix'. */

    Size = Matrix->Size;
    Intermediate = (ComplexVector)Matrix->Intermediate;

/* Correct array pointers for ARRAY_OFFSET. */
#if NOT ARRAY_OFFSET
#if spSEPARATED_COMPLEX_VECTORS
    --RHS;
    --iRHS;
    --Solution;
    --iSolution;
#else
    RHS -= 2;
    Solution -= 2;
#endif
#endif

    /* Initialize Intermediate vector. */
    pExtOrder = &Matrix->IntToExtColMap[Size];

#if spSEPARATED_COMPLEX_VECTORS
    for (I = Size; I > 0; I--) {
        Intermediate[I].Real = RHS[*(pExtOrder)];
        ASSERT(iRHS.has_value());
        Intermediate[I].Imag = iRHS.value()[*(pExtOrder--)];
    }
#else
    ExtVector = (ComplexVector)RHS;
    for (I = Size; I > 0; I--)
        Intermediate[I] = ExtVector[*(pExtOrder--)];
#endif

    /* Forward elimination. */
    for (I = 1; I <= Size; I++) {
        Temp = Intermediate[I];

        /* This step of the elimination is skipped if Temp equals zero. */
        if ((Temp.Real != 0.0) OR(Temp.Imag != 0.0)) {
            pElement = Matrix->Diag[I]->NextInRow;
            while (pElement != NULL) {
                /* Cmplx expr: Intermediate[Element->Col] -= Temp * *Element. */
                CMPLX_MULT_SUBT_ASSIGN(Intermediate[pElement->Col],
                    Temp, *pElement);
                pElement = pElement->NextInRow;
            }
        }
    }

    /* Backward Substitution. */
    for (I = Size; I > 0; I--) {
        pPivot = Matrix->Diag[I];
        Temp = Intermediate[I];
        pElement = pPivot->NextInCol;

        while (pElement != NULL) {
            /* Cmplx expr: Temp -= Intermediate[Element->Row] * *Element. */
            CMPLX_MULT_SUBT_ASSIGN(Temp, Intermediate[pElement->Row], *pElement);

            pElement = pElement->NextInCol;
        }
        /* Cmplx expr: Intermediate = Temp * (1.0 / *pPivot). */
        CMPLX_MULT(Intermediate[I], Temp, *pPivot);
    }

    /* Unscramble Intermediate vector while placing data in to Solution vector. */
    pExtOrder = &Matrix->IntToExtRowMap[Size];

#if spSEPARATED_COMPLEX_VECTORS
    for (I = Size; I > 0; I--) {
        Solution[*(pExtOrder)] = Intermediate[I].Real;
        iSolution.value()[*(pExtOrder--)] = Intermediate[I].Imag;
    }
#else
    ExtVector = (ComplexVector)Solution;
    for (I = Size; I > 0; I--)
        ExtVector[*(pExtOrder--)] = Intermediate[I];
#endif

    return;
}
#endif /* TRANSPOSE AND spCOMPLEX */
