import pytest
from neuron import h, rxd
from testutils import collect_data
from importlib import reload


@pytest.fixture
def neuron_instance():
    data = {'record_count': 0, 'data': []}
    h.load_file('stdrun.hoc')
    cvode = h.CVode()
    cvode.active(False)
    cvode.atol(1E-3)
    cvode.extra_scatter_gather(0, lambda: collect_data(h, rxd, data))
    yield (h, rxd, data)
    rxd.region._all_regions = []
    rxd.region._region_count = 0
    for r in rxd.rxd._all_reactions[:]:
        if r():
            rxd.rxd._unregister_reaction(r)
    for s in rxd.species._all_species + rxd.species._all_defined_species:
        if s():
            s().__del__()
    rxd.species._all_species = []
    rxd.species._species_counts = 0
    rxd.section1d._rxd_sec_lookup = {}
    rxd.section1d._purge_cptrs()
    for sec in h.allsec():
        h.disconnect(sec=sec)
        sref = h.SectionRef(sec=sec)
        sref.rename('delete')
        h.delete_section(sec=sec)
