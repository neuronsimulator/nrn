import os.path as osp
import numpy
import pytest
import gc

from .testutils import collect_data


def pytest_addoption(parser):
    parser.addoption("--mpi", action="store_true", default=False, help="use MPI")
    parser.addoption("--save", action="store", default=None, help="save the test data")

@pytest.fixture(scope="session")
def neuron_import(request):
    """Provides an instance of neuron h and rxd for tests"""

    # to use NEURON with MPI, mpi4py must be imported first.
    if request.config.getoption("--mpi"):
        from mpi4py import MPI  # noqa: F401
    
    save_path = request.config.getoption("--save")

    # we may not be not running in the test path so we have to load the mechanisms
    import neuron

    neuron.load_mechanisms(osp.abspath(osp.dirname(__file__)))
    from neuron import h, rxd
    return h, rxd, save_path

@pytest.fixture
def neuron_instance(neuron_import):
    """Sets/Resets the rxd test environment.

    Provides 'data', a dictionary used to store voltages and rxd node
    values for comparisons with the 'correct_data'.
    """

    h, rxd, save_path = neuron_import
    data = {'record_count': 0, 'data': []}
    h.load_file('stdrun.hoc')

    h.nrnunit_use_legacy(True) 
    
    # pytest fixtures at the function scope that require neuron_instance will go    # out of scope after neuron_instance. So species, sections, etc. will go 
    # out of scope after neuron_instance is torn down. 
    # Here we assert that no section left alive. If the assertion fails it is
    # due to a problem with the previous test, not the test which failed.
    gc.collect()
    for sec in h.allsec(): assert(False)
    cvode = h.CVode()
    cvode.active(False)
    cvode.atol(1e-3)
    h.dt = 0.025
    h.stoprun = False

    def gather():
        return collect_data(h, rxd, data, save_path)

    cvode.extra_scatter_gather(0, gather)
    yield (h, rxd, data)
    for r in rxd.rxd._all_reactions[:]:
        if r():
            rxd.rxd._unregister_reaction(r)

    for s in rxd.species._all_species:
        if s():
            s().__del__()
    rxd.region._all_regions = []
    rxd.region._region_count = 0
    rxd.region._c_region_lookup = None
    rxd.species._species_counts = 0
    rxd.section1d._purge_cptrs()
    rxd.initializer.has_initialized = False
    rxd.rxd.free_conc_ptrs()
    rxd.rxd.free_curr_ptrs()
    rxd.rxd.rxd_include_node_flux1D(0, None, None, None)
    rxd.species._has_1d = False
    rxd.species._has_3d = False
    rxd.rxd._zero_volume_indices = numpy.ndarray(0, dtype=numpy.int_)
    rxd.set_solve_type(dimension=1)
    cvode.extra_scatter_gather_remove(gather)
    
