"""plugins.py

Functions for allowing solver plugins/overrides.
"""
from .rxdException import RxDException
from . import rxd
from . import options
from . import species

class SolverPlugin:
    """Interface for solver plugins."""
    def init(self):
        """Perform any needed initialization.
        
        This function may be called multiple times. Subsequent calls purge all
        data and restart the solver de novo.
        """ 
    
    def support_deterministic():
        """True if the plugin supports deterministic simulations; else false.
        
        Deterministic simulations are assumed to be done with value = concentration.
        """
        return False

    def support_stochastic():
        """True if the plugin supports stochastic simulations; else false.
        
        Stochastic simulations are assumed to be done with value = molecules.
        """
        return False
    
    def geometry_mesh(self, mesh, dx):
        """Sets the geometry mesh when only one region."""
        
    def species(self, species_list):
        """Set the species to be used.
        
        They should be stored internally with weakrefs.
        
        The plugin will likely need this information for at least four reasons:
            1. knowing how many species there are
            2. being able to identify the indices corresponding to a given node
            3. transferring states to/from the solver
            4. diffusion rates
        """
    
    def index(self, node):
        """Return the index of a node in the plugin's internal storage."""
    
    def value(self, node):
        """Return the value (concentration or mass) associated with the node."""
    
    def values_from_indices(self, indices):
        """Return a vector of values associated with a vector of indices."""
        
    def update_value(self, node):
        """Updates the plugin's node value to match that of the node."""
    
    def mass_action_reaction(self, reactants, reactant_multiplicities, products,
                             product_multiplicities, kf, kb):
        """Declares a mass-action reaction."""
    
    def advance(self, dt):
        """Advance the solver by dt.
        
        By default, the solver will use its local copy of state variables.
        This behavior can be overriden by explicitly calling
        transfer_states_to_solver at the beginning of the advance function and
        calling transfer_states_to_neuron at the end. These two function calls
        allow pointers to concentrations to work at the expense of increased
        run-time.
        """
    
    def set_rate_of_change(self, nodes, rates_of_change):
        """Sets steady influx to nodes."""
    
    def purge_reactions(self):
        """Purge reaction list"""
    
    def purge_rates_of_change(self):
        """Purge the rates of change lists"""
        set_rate_of_change(self, [], [])
    
    def transfer_states_from_neuron(self):
        """Transfers all state variables to the solver."""
    
    def transfer_states_to_neuron(self):
        """Transfers all state variables from the solver to NEURON."""
        

def _cvode_error(*args, **kwargs):
    raise RxDException("Variable step method not supported for external plugin solvers.")

def set_solver(plugin_solver_object):
    """Override the default solver with an external solver.
    
    The plugin_solver_object must support the rxd.SolverPlugin interface,
    although it need subclass it.
    
    NOTE: External solver support currently requires the model to be simulated
          in three dimensions. 1D branching behavior is undefined.

    NOTE: For now, only fixed step advances are supported.
    
    NOTE: Correct pointer access to read/write state variables is not guaranteed
          in general, but can be obtained by having the advance method sync
          state values from neuron at the beginning and to neuron at the end.
    """
    structure_changed_event = rxd._w_setup
    change_currents = None
    change_conductance = None
    rxd._callbacks = [structure_changed_event, None, change_currents,
        change_conductance,
        lambda dt: _fixed_step_advance(dt, plugin_solver_object), _cvode_error,
        _cvode_error, _cvode_error, _cvode_error, _cvode_error, None]
    rxd._external_solver = plugin_solver_object
    rxd._external_solver_initialized = False

def _initialize(solver):
    solver.init()
    # TODO: check support_stochastic/support_deterministic as appropriate
    all_species_ptrs = species._defined_species.values()
    all_species = []
    mesh = None
    for s in all_species:
        s = s()
        if s is not None:
            all_species.append(s)
            if s._dimension != 3:
                raise RxDException('External solvers currently only support 3D simulations')
            if len(s._regions) != 1:
                raise RxDException('External solvers currently only work on 1 region')
            if mesh is None:
                mesh = s._regions[0]._mesh
            if mesh != s._regions[0]._mesh:
                raise RxDException('Exteral solvers currently require a unique mesh')
    
    # TODO: allow alternate forms of specification of the mesh (e.g. connectivity for NTW)
    solver.geometry_mesh(mesh._values, mesh._dx)

    solver.species(all_species)            
    
    # TODO: transfer information about the reactions
    
    solver.transfer_states_from_neuron()

def _fixed_step_advance(dt, solver):
    if not rxd._external_solver_initialized:
        _initialize(solver)
    # TODO: transfer information about currents
    solver.purge_rates_of_change()
    
    # do the advance
    solver.advance(dt)
    
    # TODO: only collect values that are must-sync
    solver.transfer_states_to_neuron()


def default_solver():
    """Restore the default solver."""
    rxd._callbacks = [rxd._w_setup, None, rxd._w_currents, rxd._w_conductance, rxd._w_fixed_step_solve,
              rxd._w_ode_count, rxd._w_ode_reinit, rxd._w_ode_fun, rxd._w_ode_solve, rxd._w_ode_jacobian, None]
    rxd._external_solver = None
    rxd._external_solver_initialized = False

