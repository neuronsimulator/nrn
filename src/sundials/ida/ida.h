/*
 * -----------------------------------------------------------------
 * $Revision: 855 $
 * $Date: 2005-02-10 00:15:46 +0100 (Thu, 10 Feb 2005) $
 * ----------------------------------------------------------------- 
 * Programmers: Allan G. Taylor, Alan C. Hindmarsh, and
 *              Radu Serban @ LLNL
 * -----------------------------------------------------------------
 * Copyright (c) 2002, The Regents of the University of California  
 * Produced at the Lawrence Livermore National Laboratory
 * All rights reserved
 * For details, see sundials/ida/LICENSE
 * -----------------------------------------------------------------
 * This is the header (include) file for the main IDA solver.
 * -----------------------------------------------------------------
 */

#ifndef _IDA_H
#define _IDA_H

#ifdef __cplusplus     /* wrapper to enable C++ usage */
extern "C" {
#endif

#include <stdio.h>
#include <sundials/shared/sundialstypes.h>
#include <sundials/shared/nvector.h>

/*
 * -----------------------------------------------------------------
 * IDA is used to solve numerically the initial value problem     
 * for the differential algebraic equation (DAE) system           
 *   F(t,y,y') = 0,                                               
 * given initial conditions                                       
 *   y(t0) = y0,   y'(t0) = yp0.                                  
 * Here y and F are vectors of length N.                          
 * -----------------------------------------------------------------
 */

/*
 * ----------------------------------------------------------------
 * Type : IDAResFn                                                   
 * ----------------------------------------------------------------
 * The F function which defines the DAE system   F(t,y,y')=0      
 * must have type IDAResFn.                                          
 * Symbols are as follows: 
 *                  t  <-> t        y <-> yy               
 *                  y' <-> yp       F <-> rr
 * A IDAResFn takes as input the independent variable value t,    
 * the dependent variable vector yy, and the derivative (with     
 * respect to t) of the yy vector, yp.  It stores the result of   
 * F(t,y,y') in the vector rr. The yy, yp, and rr arguments are of 
 * type N_Vector. The res_data parameter is the pointer res_data 
 * passed by the user to the IDASetRdata routine. This user-supplied 
 * pointer is passed to the user's res function every time it is called, 
 * to provide access in res to user data.                                    
 *                                                                
 * A IDAResFn res should return a value of 0 if successful, a positive
 * value if a recoverable error occured (e.g. yy has an illegal value),
 * or a negative value if a nonrecoverable error occured. In the latter
 * case, the program halts. If a recoverable error occured, the integrator
 * will attempt to correct and retry.
 * ----------------------------------------------------------------
 */

typedef int (*IDAResFn)(realtype tt, 
                        N_Vector yy, N_Vector yp, N_Vector rr, 
                        void *res_data);
 
/*
 * ----------------------------------------------------------------
 * Inputs to IDAMalloc, IDAReInit, IDACalcIC, and IDASolve.       
 * ----------------------------------------------------------------
 */

/* itol */
#define IDA_SS 1
#define IDA_SV 2

/* itask */
#define IDA_NORMAL         1
#define IDA_ONE_STEP       2
#define IDA_NORMAL_TSTOP   3 
#define IDA_ONE_STEP_TSTOP 4

/* icopt */
#define IDA_YA_YDP_INIT 1 
#define IDA_Y_INIT      2

/*
 * ================================================================
 *          U S E R - C A L L A B L E   R O U T I N E S           
 * ================================================================
 */

/* 
 * ----------------------------------------------------------------
 * Function : IDACreate                                           
 * ----------------------------------------------------------------
 * IDACreate creates an internal memory block for a problem to    
 * be solved by IDA.                                              
 *                                                                
 * If successful, IDACreate returns a pointer to initialized      
 * problem memory. This pointer should be passed to IDAMalloc.    
 * If an initialization error occurs, IDACreate prints an error   
 * message to standard err and returns NULL.                      
 *                                                                
 * ----------------------------------------------------------------
 */

void *IDACreate(void);

/*
 * ----------------------------------------------------------------
 * Integrator optional input specification functions              
 * ----------------------------------------------------------------
 * The following functions can be called to set optional inputs   
 * to values other than the defaults given below:                 
 *                                                                
 *                      |                                         
 * Function             |  Optional input / [ default value ]     
 *                      |                                          
 * ---------------------------------------------------------------- 
 *                      |                                          
 * IDASetRdata          | a pointer to user data that will be     
 *                      | passed to the user's res function every 
 *                      | time res is called.                     
 *                      | [NULL]                                  
 *                      |                                          
 * IDASetErrFile        | the file pointer for an error file      
 *                      | where all IDA warning and error         
 *                      | messages will be written. This parameter
 *                      | can be stdout (standard output), stderr 
 *                      | (standard error), a file pointer        
 *                      | (corresponding to a user error file     
 *                      | opened for writing) returned by fopen.  
 *                      | If not called, then all messages will   
 *                      | be written to standard output.          
 *                      | [NULL]                                  
 *                      |                                          
 * IDASetMaxOrd         | maximum lmm order to be used by the     
 *                      | solver.                                 
 *                      | [5]                                      
 *                      |                                          
 * IDASetMaxNumSteps    | maximum number of internal steps to be  
 *                      | taken by the solver in its attempt to   
 *                      | reach tout.                             
 *                      | [500]                                   
 *                      |                                          
 * IDASetInitStep       | initial step size.                      
 *                      | [estimated by IDA]                       
 *                      |                                          
 * IDASetMaxStep        | maximum absolute value of step size     
 *                      | allowed.                                
 *                      | [infinity]                              
 *                      |                                          
 * IDASetStopTime       | the independent variable value past     
 *                      | which the solution is not to proceed.   
 *                      | [infinity]                              
 *                      |                                          
 * IDASetNonlinConvCoef | Newton convergence test  constant       
 *                      | for use during integration.             
 *                      | [0.33]                                  
 *                      |                                          
 * IDASetMaxErrTestFails| Maximum number of error test failures   
 *                      | in attempting one step.                 
 *                      | [10]                                    
 *                      |                                         
 * IDASetMaxNonlinIters | Maximum number of nonlinear solver      
 *                      | iterations at one solution.             
 *                      | [4]                                     
 *                      |                                         
 * IDASetMaxConvFails   | Maximum number of allowable conv.       
 *                      | failures in attempting one step.        
 *                      | [10]                                    
 *                      |                                         
 * IDASetSuppressAlg    | flag to indicate whether or not to      
 *                      | suppress algebraic variables in the     
 *                      | local error tests:                      
 *                      | FALSE = do not suppress;                 
 *                      | TRUE = do suppress;                     
 *                      | [FALSE]                                 
 *                      | NOTE: if suppressed algebraic variables 
 *                      | is selected, the nvector 'id' must be   
 *                      | supplied for identification of those    
 *                      | algebraic components (see IDASetId).    
 *                      |                                          
 * IDASetId             | an N_Vector, which states a given       
 *                      | element to be either algebraic or       
 *                      | differential.                           
 *                      | A value of 1.0 indicates a differential 
 *                      | variable while a 0.0 indicates an       
 *                      | algebraic variable. 'id' is required    
 *                      | if optional input SUPPRESSALG is set,   
 *                      | or if IDACalcIC is to be called with    
 *                      | icopt = IDA_YA_YDP_INIT.               
 *                      |                                         
 * IDASetConstraints    | an N_Vector defining inequality         
 *                      | constraints for each component of the   
 *                      | solution vector y. If a given element   
 *                      | of this vector has values +2 or -2,     
 *                      | then the corresponding component of y   
 *                      | will be constrained to be > 0.0 or      
 *                      | <0.0, respectively, while if it is +1   
 *                      | or -1, the y component is constrained   
 *                      | to be >= 0.0 or <= 0.0, respectively.   
 *                      | If a component of constraints is 0.0,   
 *                      | then no constraint is imposed on the    
 *                      | corresponding component of y.           
 *                      | The presence of a non-NULL constraints  
 *                      | vector that is not 0.0 (ZERO) in all    
 *                      | components will cause constraint        
 *                      | checking to be performed.               
 *                      |                                         
 * -----------------------------------------------------------------
 *                      |
 * IDASetTolerances     | Changes the integration tolerances
 *                      | between calls to IDASolve().
 *                      | [set by IDAMalloc/IDAReInit]
 *                      |
 * ---------------------------------------------------------------- 
 * Return flag:
 *   IDA_SUCCESS   if successful
 *   IDA_MEM_NULL  if the ida memory is NULL
 *   IDA_ILL_INPUT if an argument has an illegal value
 *
 * ----------------------------------------------------------------
 */

int IDASetRdata(void *ida_mem, void *res_data);
int IDASetErrFile(void *ida_mem, FILE *errfp);
int IDASetMaxOrd(void *ida_mem, int maxord);
int IDASetMaxNumSteps(void *ida_mem, long int mxsteps);
int IDASetInitStep(void *ida_mem, realtype hin);
int IDASetMaxStep(void *ida_mem, realtype hmax);
int IDASetStopTime(void *ida_mem, realtype tstop);
int IDASetNonlinConvCoef(void *ida_mem, realtype epcon);
int IDASetMaxErrTestFails(void *ida_mem, int maxnef);
int IDASetMaxNonlinIters(void *ida_mem, int maxcor);
int IDASetMaxConvFails(void *ida_mem, int maxncf);
int IDASetSuppressAlg(void *ida_mem, booleantype suppressalg);
int IDASetId(void *ida_mem, N_Vector id);
int IDASetConstraints(void *ida_mem, N_Vector constraints);

int IDASetTolerances(void *cvode_mem, int itol, realtype *rtol, void *atol);

/*
 * ----------------------------------------------------------------
 * Function : IDAMalloc                                           
 * ----------------------------------------------------------------
 * IDAMalloc allocates and initializes memory for a problem to    
 * to be solved by IDA.                                           
 *                                                                
 * res     is the residual function F in F(t,y,y') = 0.                     
 *                                                                
 * t0      is the initial value of t, the independent variable.   
 *                                                                
 * yy0     is the initial condition vector y(t0).                 
 *                                                                
 * yp0     is the initial condition vector y'(t0)                 
 *                                                                
 * itol    is the type of tolerances to be used.                  
 *            The legal values are:                               
 *               SS (scalar relative and absolute  tolerances),   
 *               SV (scalar relative tolerance and vector         
 *                   absolute tolerance).                         
 *                                                                
 * rtol    is a pointer to the relative tolerance scalar.         
 *                                                                
 * atol    is a pointer (void) to the absolute tolerance scalar or
 *            an N_Vector tolerance.                              
 * (ewt)                                                          
 *         Both rtol and atol are used to compute the error weight
 *         vector, ewt. The error test required of a correction   
 *         delta is that the weighted-RMS norm of delta be less   
 *         than or equal to 1.0. Other convergence tests use the  
 *         same norm. The weighting vector used in this norm is   
 *         ewt. The components of ewt are defined by              
 *         ewt[i] = 1.0/(rtol*yy[i] + atol[i]). Here, yy is the   
 *         current approximate solution.  See the routine         
 *         N_VWrmsNorm for the norm used in this error test.      
 *                                                                
 * Note: The tolerance values may be changed in between calls to  
 *       IDASolve for the same problem. These values refer to     
 *       (*rtol) and either (*atol), for a scalar absolute        
 *       tolerance, or the components of atol, for a vector       
 *       absolute tolerance.                                      
 *                                                                 
 *  IDA_SUCCESS if successful
 *  IDA_MEM_NULL if the ida memory was NULL
 *  IDA_MEM_FAIL if a memory allocation failed
 *  IDA_ILL_INPUT f an argument has an illegal value.
 *                                                                
 * ----------------------------------------------------------------
 */

int IDAMalloc(void *ida_mem, IDAResFn res,
              realtype t0, N_Vector yy0, N_Vector yp0, 
              int itol, realtype *rtol, void *atol);

/*
 * ----------------------------------------------------------------
 * Function : IDAReInit                                           
 * ----------------------------------------------------------------
 * IDAReInit re-initializes IDA for the solution of a problem,    
 * where a prior call to IDAMalloc has been made.                 
 * IDAReInit performs the same input checking and initializations 
 * that IDAMalloc does.                                           
 * But it does no memory allocation, assuming that the existing   
 * internal memory is sufficient for the new problem.             
 *                                                                
 * The use of IDAReInit requires that the maximum method order,   
 * maxord, is no larger for the new problem than for the problem  
 * specified in the last call to IDAMalloc.  This condition is    
 * automatically fulfilled if the default value for maxord is     
 * specified.                                                     
 *                                                                
 * Following the call to IDAReInit, a call to the linear solver   
 * specification routine is necessary if a different linear solver
 * is chosen, but may not be otherwise.  If the same linear solver
 * is chosen, and there are no changes in its input parameters,   
 * then no call to that routine is needed.                        
 *                                                                
 * The first argument to IDAReInit is:                            
 *                                                                
 * ida_mem = pointer to IDA memory returned by IDACreate.         
 *                                                                
 * All the remaining arguments to IDAReInit have names and        
 * meanings identical to those of IDAMalloc.                      
 *                                                                
 * The return value of IDAReInit is equal to SUCCESS = 0 if there 
 * were no errors; otherwise it is a negative int equal to:       
 *   IDA_MEM_NULL   indicating ida_mem was NULL, or            
 *   IDA_NO_MALLOC  indicating that ida_mem was not allocated. 
 *   IDA_ILL_INPUT  indicating an input argument was illegal   
 *                  (including an attempt to increase maxord). 
 * In case of an error return, an error message is also printed.  
 * ----------------------------------------------------------------
 */                                                                

int IDAReInit(void *ida_mem, IDAResFn res,
              realtype t0, N_Vector yy0, N_Vector yp0,
              int itol, realtype *rtol, void *atol);
 
/* ----------------------------------------------------------------
 * Initial Conditions optional input specification functions      
 * ----------------------------------------------------------------
 * The following functions can be called to set optional inputs   
 * to control the initial conditions calculations.                
 *                                                                
 *                        |                                        
 * Function               |  Optional input / [ default value ]   
 *                        |                                        
 * -------------------------------------------------------------- 
 *                        |                                        
 * IDASetNonlinConvCoefIC | positive coeficient in the Newton     
 *                        | convergence test.  This test uses a   
 *                        | weighted RMS norm (with weights       
 *                        | defined by the tolerances, as in      
 *                        | IDASolve).  For new initial value     
 *                        | vectors y and y' to be accepted, the  
 *                        | norm of J-inverse F(t0,y,y') is       
 *                        | required to be less than epiccon,     
 *                        | where J is the system Jacobian.       
 *                        | [0.01 * 0.33]                          
 *                        |                                        
 * IDASetMaxNumStepsIC    | maximum number of values of h allowed 
 *                        | when icopt = IDA_YA_YDP_INIT, where  
 *                        | h appears in the system Jacobian,     
 *                        | J = dF/dy + (1/h)dF/dy'.              
 *                        | [5]                                   
 *                        |                                        
 * IDASetMaxNumJacsIC     | maximum number of values of the       
 *                        | approximate Jacobian or preconditioner
 *                        | allowed, when the Newton iterations   
 *                        | appear to be slowly converging.       
 *                        | [4]                                    
 *                        |                                        
 * IDASetMaxNumItersIC    | maximum number of Newton iterations   
 *                        | allowed in any one attempt to solve   
 *                        | the IC problem.                       
 *                        | [10]                                  
 *                        |                                        
 * IDASetLineSearchOffIC  | a bool flag to turn off the        
 *                        | linesearch algorithm.                 
 *                        | [FALSE]                               
 *                        |                                        
 * IDASetStepToleranceIC  | positive lower bound on the norm of   
 *                        | a Newton step.                        
 *                        | [(unit roundoff)^(2/3)                
 *                                                                
 * ---------------------------------------------------------------- 
 * Return flag:
 *   IDA_SUCCESS   if successful
 *   IDA_MEM_NULL  if the ida memory is NULL
 *   IDA_ILL_INPUT if an argument has an illegal value
 *
 * ----------------------------------------------------------------
 */

int IDASetNonlinConvCoefIC(void *ida_mem, realtype epiccon);
int IDASetMaxNumStepsIC(void *ida_mem, int maxnh);
int IDASetMaxNumJacsIC(void *ida_mem, int maxnj);
int IDASetMaxNumItersIC(void *ida_mem, int maxnit);
int IDASetLineSearchOffIC(void *ida_mem, booleantype lsoff);
int IDASetStepToleranceIC(void *ida_mem, realtype steptol);

/*
 * ----------------------------------------------------------------
 * Function : IDACalcIC                                           
 * ----------------------------------------------------------------
 * IDACalcIC calculates corrected initial conditions for the DAE  
 * system for a class of index-one problems of semi-implicit form.
 * It uses Newton iteration combined with a Linesearch algorithm. 
 * Calling IDACalcIC is optional. It is only necessary when the   
 * initial conditions do not solve the given system.  I.e., if    
 * y0 and yp0 are known to satisfy F(t0, y0, yp0) = 0, then       
 * a call to IDACalcIC is NOT necessary (for index-one problems). 
 *                                                                
 * A call to IDACalcIC must be preceded by a successful call to   
 * IDAMalloc or IDAReInit for the given DAE problem, and by a     
 * successful call to the linear system solver specification      
 * routine.                                                       
 * In addition, IDACalcIC assumes that the vectors y0, yp0, and   
 * (if relevant) id and constraints that were set through         
 * IDASetConstraints remain unaltered since that call.            
 *                                                                
 * The call to IDACalcIC should precede the call(s) to IDASolve   
 * for the given problem.                                         
 *                                                                
 * The arguments to IDACalcIC are as follows.  The first three -- 
 * ida_mem, icopt, tout1 -- are required; the others are optional.
 * A zero value passed for any optional input specifies that the  
 * default value is to be used.                                   
 *                                                                
 * IDA_mem is the pointer to IDA memory returned by IDACreate.    
 *                                                                
 * icopt  is the option of IDACalcIC to be used.                  
 *        icopt = IDA_YA_YDP_INIT   directs IDACalcIC to compute 
 *                the algebraic components of y and differential  
 *                components of y', given the differential        
 *                components of y.  This option requires that the 
 *                N_Vector id was set through a call to IDASetId  
 *                specifying the differential and algebraic       
 *                components.                                     
 *        icopt = IDA_Y_INIT   directs IDACalcIC to compute all  
 *                components of y, given y'.  id is not required. 
 *                                                                
 * tout1  is the first value of t at which a soluton will be      
 *        requested (from IDASolve).  (This is needed here to     
 *        determine the direction of integration and rough scale  
 *        in the independent variable t.                          
 *                                                                
 *                                                                
 * IDACalcIC returns an int flag.  Its symbolic values and their  
 * meanings are as follows.  (The numerical return values are set 
 * above in this file.)  All unsuccessful returns give a negative 
 * return value.  If IFACalcIC failed, y0 and yp0 contain         
 * (possibly) altered values, computed during the attempt.        
 *                                                                
 * SUCCESS             IDACalcIC was successful.  The corrected   
 *                     initial value vectors are in y0 and yp0.    
 *                                                                
 * IDA_MEM_NULL        The argument ida_mem was NULL.             
 *                                                                
 * IDA_ILL_INPUT       One of the input arguments was illegal.    
 *                     See printed message.                       
 *                                                                
 * IDA_LINIT_FAIL      The linear solver's init routine failed.   
 *                                                                
 * IDA_BAD_EWT         Some component of the error weight vector  
 *                     is zero (illegal), either for the input    
 *                     value of y0 or a corrected value.          
 *                                                                
 * IDA_RES_FAIL        The user's residual routine returned 
 *                     a non-recoverable error flag.              
 *                                                                
 * IDA_FIRST_RES_FAIL  The user's residual routine returned 
 *                     a recoverable error flag on the first call,
 *                     but IDACalcIC was unable to recover.       
 *                                                                
 * IDA_LSETUP_FAIL     The linear solver's setup routine had a    
 *                     non-recoverable error.                     
 *                                                                
 * IDA_LSOLVE_FAIL     The linear solver's solve routine had a    
 *                     non-recoverable error.                     
 *                                                                
 * IDA_NO_RECOVERY     The user's residual routine, or the linear 
 *                     solver's setup or solve routine had a      
 *                     recoverable error, but IDACalcIC was       
 *                     unable to recover.                         
 *                                                                
 * IDA_CONSTR_FAIL     IDACalcIC was unable to find a solution    
 *                     satisfying the inequality constraints.     
 *                                                                
 * IDA_LINESEARCH_FAIL The Linesearch algorithm failed to find a  
 *                     solution with a step larger than steptol   
 *                     in weighted RMS norm.                      
 *                                                                
 * IDA_CONV_FAIL       IDACalcIC failed to get convergence of the 
 *                     Newton iterations.                         
 *                                                                
 * ----------------------------------------------------------------
 */

int IDACalcIC (void *ida_mem, int icopt, realtype tout1); 

/*
 * ----------------------------------------------------------------
 * Function : IDASolve                                            
 * ----------------------------------------------------------------
 * IDASolve integrates the DAE over an interval in t, the         
 * independent variable. If itask is NORMAL, then the solver      
 * integrates from its current internal t value to a point at or  
 * beyond tout, then interpolates to t = tout and returns y(tret) 
 * in the user-allocated vector yret. In general, tret = tout.    
 * If itask is ONE_STEP, then the solver takes one internal step  
 * of the independent variable and returns in yret the value of y 
 * at the new internal independent variable value. In this case,  
 * tout is used only during the first call to IDASolve to         
 * determine the direction of integration and the rough scale of  
 * the problem. In either case, the independent variable value    
 * reached by the solver is placed in (*tret). The user is         
 * responsible for allocating the memory for this value.          
 *                                                                
 * IDA_mem is the pointer (void) to IDA memory returned by        
 *         IDACreate.                                             
 *                                                                
 * tout    is the next independent variable value at which a      
 *         computed solution is desired.                          
 *                                                                
 * *tret   is the actual independent variable value corresponding 
 *         to the solution vector yret.                           
 *                                                                
 * yret    is the computed solution vector.  With no errors,      
 *         yret = y(tret).                                        
 *                                                                
 * ypret   is the derivative of the computed solution at t = tret.
 *                                                                
 * Note: yret and ypret may be the same N_Vectors as y0 and yp0   
 * in the call to IDAMalloc or IDAReInit.                         
 *                                                                
 * itask   is NORMAL, NORMAL_TSTOP, ONE_STEP, or ONE_STEP_TSTOP.  
 *         These modes are described above.                       
 *                                                                
 *                                                                
 * The return values for IDASolve are described below.            
 * (The numerical return values are defined above in this file.)  
 * All unsuccessful returns give a negative return value.         
 *                                                                
 * IDA_SUCCESS
 *   IDASolve succeeded.                       
 *                                                                
 * IDA_TSTOP_RETURN: 
 *   IDASolve returns computed results for the independent variable 
 *   value tstop. That is, tstop was reached.                            
 *                                                                
 * IDA_MEM_NULL: 
 *   The IDA_mem argument was NULL.            
 *                                                                
 * IDA_ILL_INPUT: 
 *   One of the inputs to IDASolve is illegal. This includes the 
 *   situation when a component of the error weight vectors 
 *   becomes < 0 during internal stepping. The ILL_INPUT flag          
 *   will also be returned if the linear solver function IDA
 *   (called by the user after calling IDACreate) failed to set one 
 *   of the linear solver-related fields in IDA_mem or if the linear 
 *   solver's init routine failed. In any case, the user should see 
 *   the printed error message for more details.                
 *                                                                
 * IDA_TOO_MUCH_WORK: 
 *   The solver took mxstep internal steps but could not reach tout. 
 *   The default value for mxstep is MXSTEP_DEFAULT = 500.                
 *                                                                
 * IDA_TOO_MUCH_ACC: 
 *   The solver could not satisfy the accuracy demanded by the user 
 *   for some internal step.   
 *                                                                
 * IDA_ERR_FAIL:
 *   Error test failures occurred too many times (=MXETF = 10) during 
 *   one internal step.  
 *                                                                
 * IDA_CONV_FAIL: 
 *   Convergence test failures occurred too many times (= MXNCF = 10) 
 *   during one internal step.                                          
 *                                                                
 * IDA_LSETUP_FAIL: 
 *   The linear solver's setup routine failed  
 *   in an unrecoverable manner.                    
 *                                                                
 * IDA_LSOLVE_FAIL: 
 *   The linear solver's solve routine failed  
 *   in an unrecoverable manner.                    
 *                                                                
 * IDA_CONSTR_FAIL:
 *    The inequality constraints were violated, 
 *    and the solver was unable to recover.         
 *                                                                
 * IDA_REP_RES_ERR: 
 *    The user's residual function repeatedly returned a recoverable 
 *    error flag, but the solver was unable to recover.                 
 *                                                                
 * IDA_RES_FAIL:
 *    The user's residual function returned a nonrecoverable error 
 *    flag.
 *                                                                
 * ----------------------------------------------------------------
 */

int IDASolve(void *ida_mem, realtype tout, realtype *tret,
             N_Vector yret, N_Vector ypret, int itask);

/*
 * ----------------------------------------------------------------
 * Function: IDAGetSolution                                       
 * ----------------------------------------------------------------
 *                                                                
 * This routine evaluates y(t) and y'(t) as the value and         
 * derivative of the interpolating polynomial at the independent  
 * variable t, and stores the results in the vectors yret and     
 * ypret.  It uses the current independent variable value, tn,    
 * and the method order last used, kused. This function is        
 * called by IDASolve with t = tout, t = tn, or t = tstop.        
 *                                                                
 * If kused = 0 (no step has been taken), or if t = tn, then the  
 * order used here is taken to be 1, giving yret = phi[0],        
 * ypret = phi[1]/psi[0].                                         
 *                                                                
 * The return values are:                                         
 *   IDA_SUCCESS:  succeess.                                  
 *   IDA_BAD_T:    t is not in the interval [tn-hu,tn].                   
 *   IDA_MEM_NULL: The ida_mem argument was NULL.     
 *                                                                
 * ----------------------------------------------------------------
 */

int IDAGetSolution(void *ida_mem, realtype t, 
                   N_Vector yret, N_Vector ypret);

/* ----------------------------------------------------------------
 * Integrator optional output extraction functions                
 * ----------------------------------------------------------------
 *                                                                
 * The following functions can be called to get optional outputs  
 * and statistics related to the main integrator.                 
 * ---------------------------------------------------------------- 
 *                                                                
 * IDAGetWorkSpace returns the IDA real and integer workspace sizes      
 * IDAGetNumSteps returns the cumulative number of internal       
 *       steps taken by the solver                                
 * IDAGetNumRhsEvals returns the number of calls to the user's    
 *       res function                                             
 * IDAGetNumLinSolvSetups returns the number of calls made to     
 *       the linear solver's setup routine                        
 * IDAGetNumErrTestFails returns the number of local error test   
 *       failures that have occured                               
 * IDAGetNumBacktrackOps returns the number of backtrack          
 *       operations done in the linesearch algorithm in IDACalcIC 
 * IDAGetLastOrder returns the order used during the last         
 *       internal step                                            
 * IDAGetCurentOrder returns the order to be used on the next     
 *       internal step                                            
 * IDAGetActualInitStep returns the actual initial step size      
 *       used by IDA                                              
 * IDAGetLAstStep returns the step size for the last internal     
 *       step (if from IDASolve), or the last value of the        
 *       artificial step size h (if from IDACalcIC)               
 * IDAGetCurrentStep returns the step size to be attempted on the 
 *       next internal step                                       
 * IDAGetCurrentTime returns the current internal time reached    
 *       by the solver                                            
 * IDAGetTolScaleFactor returns a suggested factor by which the   
 *       user's tolerances should be scaled when too much         
 *       accuracy has been requested for some internal step       
 * IDAGetErrWeights returns the state error weight vector.        
 *       The user need not allocate space for ewt.                
 * IDAGetEstLocalErrors returns the vector of estimated local     
 *       errors. The user need not allocate space for ele.        
 *                                                                
 * IDAGet* return values:
 *   IDA_SUCCESS   if succesful
 *   IDA_MEM_NULL  if the ida memory was NULL
 *
 * ----------------------------------------------------------------
 */

int IDAGetWorkSpace(void *ida_mem, long int *lenrw, long int *leniw);
int IDAGetNumSteps(void *ida_mem, long int *nsteps);
int IDAGetNumResEvals(void *ida_mem, long int *nrevals);
int IDAGetNumLinSolvSetups(void *ida_mem, long int *nlinsetups);
int IDAGetNumErrTestFails(void *ida_mem, long int *netfails);
int IDAGetNumBacktrackOps(void *ida_mem, long int *nbacktr);
int IDAGetLastOrder(void *ida_mem, int *klast);
int IDAGetCurrentOrder(void *ida_mem, int *kcur);
int IDAGetActualInitStep(void *ida_mem, realtype *hinused);
int IDAGetLastStep(void *ida_mem, realtype *hlast);
int IDAGetCurrentStep(void *ida_mem, realtype *hcur);
int IDAGetCurrentTime(void *ida_mem, realtype *tcur);
int IDAGetTolScaleFactor(void *ida_mem, realtype *tolsfact);
int IDAGetErrWeights(void *ida_mem, N_Vector *eweight);

int IDAGetIntegratorStats(void *ida_mem, long int *nsteps, 
                          long int *nrevals, long int *nlinsetups, 
                          long int *netfails, int *qlast, int *qcur, 
                          realtype *hlast, realtype *hcur, 
                          realtype *tcur);

/*
 * ----------------------------------------------------------------
 * Nonlinear solver optional output extraction functions          
 * ----------------------------------------------------------------
 *                                                                
 * The following functions can be called to get optional outputs  
 * and statistics related to the nonlinear solver.                
 * -------------------------------------------------------------- 
 *                                                                
 * IDAGetNumNonlinSolvIters returns the number of nonlinear       
 *       solver iterations performed.                             
 * IDAGetNumNonlinSolvConvFails returns the number of nonlinear   
 *       convergence failures.                                    
 *                                                                
 * ----------------------------------------------------------------
 */

int IDAGetNumNonlinSolvIters(void *ida_mem, long int *nniters);
int IDAGetNumNonlinSolvConvFails(void *ida_mem, long int *nncfails);

/*
 * ----------------------------------------------------------------
 * As a convenience, the following function provides the          
 * optional outputs in a group.                                   
 * ----------------------------------------------------------------
 */

int IDAGetNonlinSolvStats(void *ida_mem, long int *nniters, 
                          long int *nncfails);

/*
 * ----------------------------------------------------------------
 * Function : IDAFree                                             
 * ----------------------------------------------------------------
 * IDAFree frees the problem memory IDA_mem allocated by          
 * IDAMalloc.  Its only argument is the pointer idamem            
 * returned by IDAMalloc.                                         
 * ----------------------------------------------------------------
 */

void IDAFree(void *ida_mem);

/* 
 * ----------------------------------------
 * IDA return flags 
 * ----------------------------------------
 */


#define IDA_SUCCESS          0
#define IDA_TSTOP_RETURN     1

#define IDA_MEM_NULL        -1
#define IDA_ILL_INPUT       -2
#define IDA_NO_MALLOC       -3
#define IDA_TOO_MUCH_WORK   -4
#define IDA_TOO_MUCH_ACC    -5
#define IDA_ERR_FAIL        -6
#define IDA_CONV_FAIL       -7
#define IDA_LINIT_FAIL      -8
#define IDA_LSETUP_FAIL     -9
#define IDA_LSOLVE_FAIL     -10
#define IDA_RES_FAIL        -11
#define IDA_CONSTR_FAIL     -12
#define IDA_REP_RES_ERR     -13

#define IDA_MEM_FAIL        -14

#define IDA_BAD_T           -15

#define IDA_BAD_EWT         -16
#define IDA_FIRST_RES_FAIL  -17
#define IDA_LINESEARCH_FAIL -18
#define IDA_NO_RECOVERY     -19

#define IDA_PDATA_NULL      -20

/*
 * =================================================================
 *     I N T E R F A C E   T O    L I N E A R   S O L V E R S     
 * =================================================================
 */

/*
 * -----------------------------------------------------------------
 * int (*ida_linit)(IDAMem IDA_mem);                               
 * -----------------------------------------------------------------
 * The purpose of ida_linit is to allocate memory for the          
 * solver-specific fields in the structure *(idamem->ida_lmem) and 
 * perform any needed initializations of solver-specific memory,   
 * such as counters/statistics. An (*ida_linit) should return      
 * 0 if it has successfully initialized the IDA linear solver and 
 * a negative value otherwise. If an error does occur, an 
 * appropriate message should be sent to (idamem->errfp).          
 * ----------------------------------------------------------------
 */                                                                 

/*
 * -----------------------------------------------------------------
 * int (*ida_lsetup)(IDAMem IDA_mem, N_Vector yyp, N_Vector ypp,   
 *                  N_Vector resp,                                 
 *            N_Vector tempv1, N_Vector tempv2, N_Vector tempv3);  
 * -----------------------------------------------------------------
 * The job of ida_lsetup is to prepare the linear solver for       
 * subsequent calls to ida_lsolve. Its parameters are as follows:  
 *                                                                 
 * idamem - problem memory pointer of type IDAMem. See the big     
 *          typedef earlier in this file.                          
 *                                                                 
 *                                                                 
 * yyp   - the predicted y vector for the current IDA internal     
 *         step.                                                   
 *                                                                 
 * ypp   - the predicted y' vector for the current IDA internal    
 *         step.                                                   
 *                                                                 
 * resp  - F(tn, yyp, ypp).                                        
 *                                                                 
 * tempv1, tempv2, tempv3 - temporary N_Vectors provided for use   
 *         by ida_lsetup.                                          
 *                                                                 
 * The ida_lsetup routine should return 0 if successful,
 * a positive value for a recoverable error, and a negative value 
 * for an unrecoverable error.
 * -----------------------------------------------------------------
 */                                                                 

/*
 * -----------------------------------------------------------------
 * int (*ida_lsolve)(IDAMem IDA_mem, N_Vector b, N_Vector weight,  
 *               N_Vector ycur, N_Vector ypcur, N_Vector rescur);  
 * -----------------------------------------------------------------
 * ida_lsolve must solve the linear equation P x = b, where        
 * P is some approximation to the system Jacobian                  
 *                  J = (dF/dy) + cj (dF/dy')                      
 * evaluated at (tn,ycur,ypcur) and the RHS vector b is input.     
 * The N-vector ycur contains the solver's current approximation   
 * to y(tn), ypcur contains that for y'(tn), and the vector rescur 
 * contains the N-vector residual F(tn,ycur,ypcur).                
 * The solution is to be returned in the vector b. 
 *                                                                 
 * The ida_lsolve routine should return 0 if successful,
 * a positive value for a recoverable error, and a negative value 
 * for an unrecoverable error.
 * -----------------------------------------------------------------
 */                                                                 

/*
 * -----------------------------------------------------------------
 * int (*ida_lperf)(IDAMem IDA_mem, int perftask);                 
 * -----------------------------------------------------------------
 * ida_lperf is called two places in IDA where linear solver       
 * performance data is required by IDA. For perftask = 0, an       
 * initialization of performance variables is performed, while for 
 * perftask = 1, the performance is evaluated.                     
 * -----------------------------------------------------------------
 */                                                                 

/*
 * -----------------------------------------------------------------
 * int (*ida_lfree)(IDAMem IDA_mem);                               
 * -----------------------------------------------------------------
 * ida_lfree should free up any memory allocated by the linear     
 * solver. This routine is called once a problem has been          
 * completed and the linear solver is no longer needed.            
 * -----------------------------------------------------------------
 */                                                                 

#ifdef __cplusplus
}
#endif

#endif
