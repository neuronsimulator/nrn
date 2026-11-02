/**
 * @file nrndae.h
 * @brief Supports modifying voltage equations and adding new equations.
 * @author Michael Hines, Robert McDougal
 *
 * @remark Subclasses of NrnDAE can work with equations of the form
 * \f[$C \frac{dy}{dt} = f(y)$\f]. LinearModelAddition, defined in linmod.h
 * and linmod.cpp is an example that supports linear dynamics of the
 * form \f[$C \frac{dy}{dt} = A y + b$\f].
 */

#pragma once
#include "ivocvect.h"
#include "matrixmap.h"
#include "vecplay_tplus.h"

#include "neuron/container/data_handle.hpp"

#include <list>
#include <vector>

/**
 * NEURON Differential Algebraic Equations.
 *
 * @remark This is an abstract class; subclass this (or use a subclass) to
 *         add dynamics. LinearModelAddition is an example. See linmod.h.
 */
class NrnDAE {
  public:
    /**
     * Find the number of state variables introduced by this object.
     *
     * @return The number of states added (not modified) by this instance.
     */
    int extra_eqn_count();

    /**
     * Allocate space for these dynamics in the overall system.
     *
     * @param start_index   starting index for new states
     */
    void alloc(int start_index);

    /**
     * Compute the left side portion of \f[$(C - J) \frac{dy}{dt} = f(y)$\f].
     */
    void lhs();

    /**
     * Compute the right side portion of \f[$(C - J) \frac{dy}{dt} = f(y)$\f].
     */
    void rhs();

    /**
     * Compute the residual: \f[$f(y) - C \frac{dy}{dt}$\f]
     *
     * @param y             array of state variables
     * @param yprime        array of derivatives of state variables
     * @param delta         array to store the difference $f(y)-Cy'$
     */
    void dkres(double* y, double* yprime, double* delta);

    /**
     * Seed local contributions to yp from C*yp = f for simple C structure
     * (identity, pure diagonal, or single-column lag). Floating mutual C
     * uses a zero common-mode gauge when the row is a pure difference stamp.
     *
     * @param f   full-system rhs f(y) (same layout as IDA residual f-part)
     * @param yp  full-system y' to fill (may already hold membrane rates)
     */
    void seed_yp_from_f(double* f, double* yp);

    /**
     * Initialize the dynamics.
     *
     * @remark Does this by calling f_init_. If f_init_ is NULL, initializes to
     *         values in y0_. If y0_ is NULL, initializes all states to 0.
     */
    void init();

    /**
     * Update states to reflect the changes over a time-step.
     *
     * @remark When this function is called, the changes have already been
     *         computed by the solver. This just updates the local variables.
     */
    void update();

    /**
     * Setup the map between voltages and states in y_.
     *
     * @param pv            pointers to voltage nodes (set by this function)
     * @param pvdot         pointers to voltage derivatives (set by this
     *                      function)
     */
    void dkmap(std::vector<double*>& pv, std::vector<double*>& pvdot);

    /**
     * Destructor.
     */
    virtual ~NrnDAE();

  protected:
    /**
     * Constructor.
     *
     * @param cmat          the matrix \f[$C$\f] in \f[$Cy'=f(y)$\f].
     * @param yvec          vector to store the state variables in
     * @param y0            initial conditions
     * @param nnode         number of voltage equations to modify
     * @param nodes         pointers to voltage nodes
     * @param elayer        which potential layer to use for each voltage node
     * @param f_init        function to call during an finitialize
     * @param data          data to pass to f_init
     *
     * @remark If cmat is NULL, then assumes \f[$C$\f] is the identity matrix.
     * @remark If f_init is non-NULL, that takes priority. Otherwise, if
     *         y0 is non-NULL, then initializes to those values. Otherwise
     *         initializes by setting all states to 0.
     */
    NrnDAE(Matrix* cmat,
           Vect* const yvec,
           Vect* const y0,
           int nnode,
           Node** const nodes,
           Vect* const elayer,
           void (*f_init)(void* data) = NULL,
           void* const data = NULL);

  private:
    /**
     * The right-hand-side function.
     *
     * @param y             the state variables
     * @param yprime        a vector to store the derivatives
     * @param size          the number of state variables
     */
    virtual void f_(Vect& y, Vect& yprime, int size) = 0;

    /**
     * Compute the Jacobian.
     *
     * @param y             the state variables
     * @return Pointer to a MatrixMap containing the jacobian.
     *
     * @remark The calling function will not delete this pointer.
     * @remark It is occasionally easier to return the Jacobian divided by
     *         a constant factor. If so, have jacobian_multiplier_ return a
     *         number that should be multiplied by the matrix returned by
     *         this function to get the true Jacobian.
     */
    virtual MatrixMap* jacobian_(Vect& y) = 0;

    /// Function used for initializing the state variables.
    void (*f_init_)(void* data);

    /// Data to pass to f_init_.
    void* data_;

    // value of jacobian_ must be multiplied by this value before use
    virtual double jacobian_multiplier_() {
        return 1;
    }

    /**
     * Additional allocation for subclasses.
     *
     * @remark Called during alloc(). Unless overriden, this function is empty.
     */
    virtual void alloc_(int size, int start, int nnode, Node** nodes, int* elayer);

  protected:
    /// the matrix \f[$C$ in $C y' = f(y)$\f]
    MatrixMap* c_;

    /// identity matrix if constructed with \f[$C$\f] NULL; else NULL.
    Matrix* assumed_identity_;

    /// vector of initial conditions
    Vect* y0_;

    /// vector to store the state variables in
    Vect& y_;

    /// total number of states declared or modified in this object
    int size_;

    /// mapping between the states in y and the states in the whole system
    int* bmap_;

    /// Number of voltage nodes used by the dynamics.
    int nnode_;

    /// Pointers to voltage nodes used by the dynamics.
    Node** nodes_;

    /// the position of the first added equation (if any) in the global system
    int start_;

    /// temporary vector used for residual calculation.
    Vect cyp_;

    /// temporary vector used for residual calculation.
    Vect yptmp_;

    /**
     * Which voltage layers to read from.
     *
     * @remark Normally elements are 0 and refer to internal potential.
     *         Otherwise range from 1 to nlayer and refer to vext[elayer-1].
     *         vm+vext and vext must be placed in y for calculation of rhs
     */
    int* elayer_;

    /// Transfer any voltage states to y_.
    void v2y();
};

/**
 * Add a NrnDAE object to the system.
 *
 * @param n The NrnDAE object (ie the dynamics) to add.
 */
void nrndae_register(NrnDAE* n);

/**
 * Remove a NrnDAE object from the system.
 *
 * @param n The NrnDAE object (ie the dynamics) to remove.
 */
void nrndae_deregister(NrnDAE* n);

/**
 * Battery-style IC: for each LinearModelAddition, project states
 * (capacitors → held Δv voltage sources, etc.). Returns 0 if all projections OK.
 */
int nrndae_battery_ic_project();

/**
 * After y is fixed, seed yp from diagonal / single-column C rows so that
 * C*yp ≈ f on LinearMechanism equations (used by dae_init_mode 3).
 * f and yp are full IDA state vectors (neq).
 */
void nrndae_seed_yp_from_f(double* f, double* yp);

/**
 * A2: using continuous Vector.play forcing t+ (u'), map db/dt into each
 * LinearMechanism and complete free yp components (null space of C).
 * forcing may be empty (no-op). yp is the full IDA y' vector.
 */
// Returns bitmask of sources used for free y': 1=play, 2=dforce/bdot, 4=FD, 8=applied.
int nrndae_complete_yp_from_forcing(double* yp, const std::vector<NrnForcingTPlus>& forcing);

/** Append LinearMechanism.dforce / FD b' entries for IC audit (A4). */
void nrndae_append_dforce_to_forcing_list(double tt, std::vector<NrnForcingTPlus>& out);

// Source bits for nrndae_complete_yp_from_forcing
enum {
    NRN_IC_FORCING_PLAY = 1,
    NRN_IC_FORCING_DFORCE = 2,
    NRN_IC_FORCING_FD = 4,
    NRN_IC_FORCING_APPLIED = 8
};

typedef std::list<NrnDAE*> NrnDAEPtrList;
typedef NrnDAEPtrList::const_iterator NrnDAEPtrListIterator;
