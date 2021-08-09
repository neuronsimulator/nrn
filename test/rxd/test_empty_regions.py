import pytest
from testutils import compare_data, tol


@pytest.fixture
def empty_regions_model(neuron_instance):
    """A model with an empty region"""

    h, rxd, data, save_path = neuron_instance
    sec = h.Section()
    sec.L = 100
    sec.diam = 1
    sec.nseg = 100
    cells = []
    cyt = rxd.Region(cells, name="cyt", nrn_region="i")
    mem = rxd.Region(cells, name="mem", geometry=rxd.membrane)
    ecs = rxd.Extracellular(-5, -5, -5, 5, 5, 5, dx=5)
    k = rxd.Species(
        [ecs, cyt],
        name="k",
        d=0,
        charge=1,
        initial=lambda nd: 140 if hasattr(nd, "x") else 3,
    )

    model = (sec, cyt, mem, ecs, k)
    yield (neuron_instance, model)


def test_empty_region_no_rate(empty_regions_model):
    """Test a rate that should do nothing with an empty region"""

    neuron_instance, model = empty_regions_model
    h, rxd, data, save_path = neuron_instance
    sec, cyt, mem, ecs, k = model
    kcyt = k[cyt]
    react = rxd.Rate(kcyt, -10 * kcyt)
    h.finitialize(-70)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_empty_region_ecs_rate(empty_regions_model):
    """Test a rate that should change the ECS with an empty region"""

    neuron_instance, model = empty_regions_model
    h, rxd, data, save_path = neuron_instance
    sec, cyt, mem, ecs, k = model
    react = rxd.Rate(k, -10 * k)
    h.finitialize(-70)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_empty_region_no_reaction(empty_regions_model):
    """Test a reaction that should change the ECS with an empty region"""

    neuron_instance, model = empty_regions_model
    h, rxd, data, save_path = neuron_instance
    sec, cyt, mem, ecs, k = model
    react = rxd.Reaction(2 * k, k, 10, regions=[cyt])
    h.finitialize(-70)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_empty_region_ecs_reaction(empty_regions_model):
    """Test a reaction that should change the ECS with an empty region"""

    neuron_instance, model = empty_regions_model
    h, rxd, data, save_path = neuron_instance
    sec, cyt, mem, ecs, k = model
    react = rxd.Reaction(2 * k, k, 10)
    h.finitialize(-70)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_empty_region_multicompartment_reaction(empty_regions_model):
    """Test a reaction that should change the ECS with an empty region"""

    neuron_instance, model = empty_regions_model
    h, rxd, data, save_path = neuron_instance
    sec, cyt, mem, ecs, k = model
    kcyt, kecs = k[cyt], k[ecs]
    react = rxd.MultiCompartmentReaction(
        kcyt, kecs, 1e5, 0, membrane=mem, membrane_flux=False
    )
    h.finitialize(-70)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol
