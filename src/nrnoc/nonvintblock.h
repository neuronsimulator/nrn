#ifndef nonvintblock_h
#define nonvintblock_h


/*
Interface for adding blocks of equations setup and solved in python
analogous to the LONGITUDINAL_DIFFUSION at nonvint time for fixed and
variable step methods. Of course, limited to single thread.

The substantive idea is embodied in the last 7 definitions below that look
like functions with distinct prototypes. The obscurity is due to having
a single function pointer in nrniv/nrnoc library space which gets dynamically
filled in when the Python nonvint_block_supervisor module is imported.
When that function is called, the supervisor interprets the remaining arguments
according to the first "method" argument. The tid (thread id) argument
should always be 0. Only one of the methods has a meaningful int return value.
The other uses can merely return 0.
*/

#if defined(nrnoc_fadvance_c)
/* define only in fadvance.cpp */
#define nonvintblock_extern /**/
#else
/* declare everywhere else */
#define nonvintblock_extern extern
#endif

extern int nrn_nonvint_block_helper(int method, int length, double* pd1, double* pd2, int tid);

nonvintblock_extern int (
    *nrn_nonvint_block)(int method, int length, double* pd1, double* pd2, int tid);

#define nonvint_block(method, size, pd1, pd2, tid) \
    nrn_nonvint_block ? nrn_nonvint_block_helper(method, size, pd1, pd2, tid) : 0

/* called near end of nrnoc/treeset.cpp:v_setup_vectors after structure_change_cnt is incremented.
 */
#define nrn_nonvint_block_setup() nonvint_block(0, 0, 0, 0, 0)

/* called in nrnoc/fadvance.cpp:nrn_finitialize before mod file INITIAL blocks */
#define nrn_nonvint_block_init(tid) nonvint_block(1, 0, 0, 0, tid)

/* called at end of nrnoc/treeset.cpp:rhs and nrncvode/cvtrset.cpp:rhs */
#define nrn_nonvint_block_current(size, rhs, tid) nonvint_block(2, size, rhs, 0, tid)
/*if any ionic membrane currents are generated, they subtract from
  NrnThread.node_rhs_storage()*/

/* called at end of nrnoc/treeset.cpp:lhs and nrncvode/cvtrset.cpp:lhs */
#define nrn_nonvint_block_conductance(size, d, tid) nonvint_block(3, size, d, 0, tid)
/*if any ionic membrane currents are generated, di/dv adds to the vector of diagonal values */

/* called at end of nrnoc/fadvance.cpp:nonvint */
#define nrn_nonvint_block_fixed_step_solve(tid) nonvint_block(4, 0, 0, 0, tid)

/* returns the number of extra equations solved by cvode or ida */
#define nrn_nonvint_block_ode_count(offset, tid) nonvint_block(5, offset, 0, 0, tid)

/* fill in the double* y with the initial values */
#define nrn_nonvint_block_ode_reinit(size, y, tid) nonvint_block(6, size, y, 0, tid)

/* using the values in double* y, fill in double* ydot so that ydot = f(y) */
#define nrn_nonvint_block_ode_fun(size, y, ydot, tid) nonvint_block(7, size, y, ydot, tid)

/* Solve (1 + dt*jacobian)*x = b replacing b values with the x values.
   Note that y (state values) are available for constructing the jacobian
   (if the problem is non-linear) */
#define nrn_nonvint_block_ode_solve(size, b, y, tid) nonvint_block(8, size, b, y, tid)

/* Do any desired preprocessing of Jacobian in preparation for ode_solve.
   This will be called at least every time dt changes */
#define nrn_nonvint_block_jacobian(size, ypred, ydot, tid) nonvint_block(9, size, ypred, ydot, tid)

/* multiply the existing values in y (cvode.atol()) with appropriate scale factors */
#define nrn_nonvint_block_ode_abstol(size, y, tid) nonvint_block(10, size, y, 0, tid)


#endif
