"""plugins.py

Functions for allowing solver plugins/overrides.
"""
from .rxdException import RxDException
from . import rxd
from . import options
from . import species
from . import node

class SolverPlugin:
    """Interface for solver plugins."""
    def init(self, species_list, reactions_list):
        """Perform any needed initialization.
        
        This function may be called multiple times. Subsequent calls purge all
        data and restart the solver de novo.
        """ 
        self._species = []
        self._reactions = []
        self._values = []
        # NOTE: to use shared memory, simply set
        #           self._values = neuron.rxd.node._states
        #       and define self._index appropriately
        #
        # NOTE: when not using shared memory, self._values should be an object
        #       that transfers data only on demand
        
        self._species(self, species_list)
        self._reactions(self, reactions_list)

    
    def _species(self, species_list):
        """Set the species to be used.
        
        They should be stored internally with weakrefs.
        
        The plugin will likely need this information for at least five reasons:
            1. knowing how many species there are
            2. being able to identify the indices corresponding to a given node
            3. grabbing the mesh
            4. transferring states to/from the solver
            5. diffusion rates
        
        This function raises an RxDException if the plugin does not support
        the species options.
        """
        import weakref
        # TODO: use a callback to automatically invalidate the plugin state
        #       if a species is destroyed
        self._species = [weakref.proxy(s) for s in species_list]
        self._set_mesh()
        
    def _reactions(self, reactions_list):
        """Store the reactions.
        
        Invokes self._process_reactions to do any necessary preprocessing.
        """
        import weakref
        # TODO: use a callback to automatically invalidate the plugin state
        #       if a reaction is destroyed
        self._reactions = [weakref.proxy(s) for s in reactions_list]
        self._process_reactions()
    
    def _process_reactions(self):
        """Do any necessary preprocessing of the reactions."""
    
    def _set_mesh(self):
        """Use self._species to define a mesh."""
    
    def _index(self, node):
        """Return the index of a node in the plugin's internal storage."""
    
    def value(self, node):
        """Return the value (concentration or mass) associated with the node."""
        return self._values_from_indices([self._index(node)])
    
    def _values_from_indices(self, indices):
        """Return a vector of values associated with a vector of indices."""
        import numpy
        return numpy.asarray(self._values)[indices]
        
    def advance(self, dt):
        """Advance the solver by dt.
        
        By default, the solver will use its local copy of state variables.
        This behavior can be overriden by explicitly calling
        transfer_states_to_solver at the beginning of the advance function and
        calling transfer_states_to_neuron at the end. These two function calls
        allow pointers to concentrations to work at the expense of increased
        run-time.
        """
    
    def supported_sim(self):
        """Returns True if this simulation is supported; else False"""
        return False
    
    def set_rate_of_change(self, nodes, rates_of_change):
        """Sets steady influx to nodes."""
        import numpy
        self._roc = numpy.zeros(len(self._values))
        for node, r in zip(nodes, rates_of_change):
            self._roc[self._index(node)] += r
        
    def transfer_states_from_neuron(self):
        """Transfers all state variables to the solver."""
        # NOTE: the default approach described below is general but consequently
        #       inefficient; plugin developers should override this function
        for species in self._species:
            try:
                for node in species.nodes:
                    self._values[self._index(node)] = node.value
            except ReferenceError:
                pass
    
    def transfer_states_to_neuron(self, which=None):
        """Transfers state variables from the solver to NEURON.
        
        If which is None, transfer all states; otherwise, transfer at least
        the requested states.
        
        If the type of which is not supported, transfer all states.
        """
        
        # NOTE: the default approach described below is general but consequently
        #       inefficient; plugin developers should override this function
        for species in self._species:
            try:
                for node in species.nodes:
                    node.value = self._values[self._index(node)]
            except ReferenceError:
                pass
            
        
class SharedMemorySolverPlugin(SolverPlugin):
    def __init__(self):
        SolverPlugin.__init__(self)
        self._values = node._states
    def transfer_states_from_neuron(self):
        """no need to transfer states since shared memory"""
    def transfer_states_to_neuron(self):
        """no need to transfer states since shared memory"""
    def _index(self, node):
        """shared memory, so trivial index mapping"""
        return node._index
        
def store_simple_3d_mesh(plugin):
    mesh = None
    for s in plugin._species:
        if s._dimension != 3:
            raise RxDException('This plugin only supports 3D simulations')
        if len(s._regions) != 1:
            raise RxDException('This plugin only supports 1 region')
        if mesh is None:
            mesh = s._regions[0]._mesh
        if mesh != s._regions[0]._mesh:
            raise RxDException('This plugin requires a unique mesh')
    
    plugin._mesh = mesh._values
    plugin._dx = mesh._dx

def _cvode_error(*args, **kwargs):
    raise RxDException("Variable step method not supported for external plugin solvers.")

def set_solver(plugin_solver_object=None):
    """Set or remove a plugin solver for reaction-diffusion dynamics.
    
    If plugin_solver_object is None, removes any plugin solver.
    
    Otherwise:
    
    The plugin_solver_object must support the rxd.SolverPlugin interface,
    although it need subclass it.
    
    NOTE: External solver support currently requires the model to be simulated
          in three dimensions. 1D branching behavior is undefined.

    NOTE: For now, only fixed step advances are supported.
    
    NOTE: Correct pointer access to read/write state variables is not guaranteed
          in general, but can be obtained by having the advance method sync
          state values from neuron at the beginning and to neuron at the end.
    """
    if plugin_solver_object is None:
        _default_solver()
    else:
        structure_changed_event = rxd._w_setup
        change_currents = None
        change_conductance = None
        rxd._callbacks = [structure_changed_event, None, change_currents,
            change_conductance,
            lambda dt: _fixed_step_advance(dt, plugin_solver_object), _cvode_error,
            _invalidate_plugin, _cvode_error, _cvode_error, _cvode_error, None]
        rxd._external_solver = plugin_solver_object
    _invalidate_plugin()

def _invalidate_plugin():
    rxd._external_solver_initialized = False

def _initialize(solver):
    all_species = [s() for s in species._defined_species.values() if s() is not None]
    all_reactions = [r() for r in rxd._all_reactions.values() if r() is not None]
    
    solver.init(all_species, all_reactions)
    
    # TODO: transfer information about the reactions
    
    if not solver.supported_sim():
        raise RxDException('Selected plugin solver does not support the current simulation.')
    
    solver.transfer_states_from_neuron()

def _fixed_step_advance(dt, solver):
    if not rxd._external_solver_initialized:
        _initialize(solver)
        
    import warnings
    warnings.warn('Plugin support for membrane currents not yet implemented.')
    
    solver.set_rate_of_change([], [])
    
    # do the advance
    solver.advance(dt)
    
    # TODO: only collect values that are must-sync
    solver.transfer_states_to_neuron()


def _default_solver():
    """Restore the default solver."""
    rxd._callbacks = [rxd._w_setup, None, rxd._w_currents, rxd._w_conductance, rxd._w_fixed_step_solve,
              rxd._w_ode_count, rxd._w_ode_reinit, rxd._w_ode_fun, rxd._w_ode_solve, rxd._w_ode_jacobian, None]
    rxd._external_solver = None
    rxd._external_solver_initialized = False

