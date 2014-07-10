"""plugins.py

Functions for allowing solver plugins/overrides.
"""
from .rxdException import RxDException
from . import rxd
from . import options

class SolverPlugin:
    """Interface for solver plugins."""
    def init(self):
        """Perform any needed initialization""" 
    
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
        
        The plugin will likely need this information for at least two reasons:
            1. knowing how many species there are
            2. being able to identify the indices corresponding to a given node
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
        """Advance the solver by dt."""
    
    def set_rate_of_change(self, nodes, rates_of_change):
        """Sets steady influx to nodes."""
    
        


def set_solver(plugin_solver_object):
    """Override the default solver with an external solver.
    
    The plugin_solver_object must support the rxd.SolverPlugin interface,
    although it need subclass it.
    
    NOTE: External solver support currently requires the model to be simulated
          in three dimensions. 1D branching behavior is undefined.

    NOTE: For now, only fixed step advances are supported.
    """
    rxd._callbacks = [setup, None, currents, conductance, lambda dt: _fixed_step_advance(dt, plugin_solver_object)]
    pass


def default_solver():
    """Restore the default solver."""
    rxd._callbacks = [rxd._w_setup, None, rxd._w_currents, rxd._w_conductance, rxd._w_fixed_step_solve,
              rxd._w_ode_count, rxd._w_ode_reinit, rxd._w_ode_fun, rxd._w_ode_solve, rxd._w_ode_jacobian, None]
