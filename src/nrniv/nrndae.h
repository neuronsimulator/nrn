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

#ifndef nrndae_h
#define nrndae_h
#include "ivocvect.h"
#include "matrixmap.h"

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
    void dkmap(std::vector<neuron::container::data_handle<double>>& pv,
               std::vector<neuron::container::data_handle<double>>& pvdot);

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

typedef std::list<NrnDAE*> NrnDAEPtrList;
typedef NrnDAEPtrList::const_iterator NrnDAEPtrListIterator;

#endif
