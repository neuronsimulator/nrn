from math import pi

import pytest

from testutils import compare_data, tol


@pytest.fixture
def ecs_example(neuron_instance):
    """A model where something is created in one cell and diffuses to another.

    This model makes use of parameters, multicompartment reactions, an NMODL
    and the extracellular space. A substance is created in an organelle in cell1
    and leaks into the cytosol. It then enters the ECS using an NMODL mechanism,
    where it diffuses to cell 2 and enters via the same NMODL mechanism.
    """

    h, rxd, data, save_path = neuron_instance

    def make_model(alpha, lambd, mech=True):
        # create cell1 where `x` will be created and leak out
        cell1 = h.Section(name="cell1")
        cell1.pt3dclear()
        cell1.pt3dadd(-2, 0, 0, 1)
        cell1.pt3dadd(-1, 0, 0, 1)
        cell1.nseg = 11
        if mech:
            cell1.insert("pump")

        # create cell2 where `x` will be pumped in and accumulate
        cell2 = h.Section(name="cell2")
        cell2.pt3dclear()
        cell2.pt3dadd(1, 0, 0, 1)
        cell2.pt3dadd(2, 0, 0, 1)
        cell2.nseg = 11
        if mech:
            cell2.insert("pump")

        # Where?
        # the intracellular spaces
        cyt = rxd.Region(
            h.allsec(),
            name="cyt",
            nrn_region="i",
            geometry=rxd.FractionalVolume(0.9, surface_fraction=1.0),
        )

        org = rxd.Region(h.allsec(), name="org", geometry=rxd.FractionalVolume(0.1))

        cyt_org_membrane = rxd.Region(
            h.allsec(),
            name="mem",
            geometry=rxd.ScalableBorder(pi / 2.0, on_cell_surface=False),
        )

        # the extracellular space
        ecs = rxd.Extracellular(
            -55, -55, -55, 55, 55, 55, dx=33, volume_fraction=alpha, tortuosity=lambd
        )

        # Who?
        x = rxd.Species(
            [cyt, org, cyt_org_membrane, ecs], name="x", d=1.0, charge=1, initial=0
        )
        Xcyt = x[cyt]
        Xorg = x[org]

        # What? - produce X in cell 1
        # parameter to limit production to cell 1
        cell1_param = rxd.Parameter(
            org, initial=lambda node: 1.0 if node.segment.sec == cell1 else 0
        )

        # production with a rate following Michaels Menton kinetics
        createX = rxd.Rate(Xorg, cell1_param[org] * 1.0 / (10.0 + Xorg))

        # leak between organelles and cytosol
        cyt_org_leak = rxd.MultiCompartmentReaction(
            Xcyt, Xorg, 1e4, 1e4, membrane=cyt_org_membrane
        )
        model = [
            cell1,
            cell2,
            cyt,
            org,
            cyt_org_membrane,
            ecs,
            x,
            Xcyt,
            Xorg,
            createX,
            cell1_param,
            createX,
            cyt_org_leak,
        ]
        if not mech:
            e = 1.60217662e-19
            scale = 1e-14 / e
            cyt_ecs_membrane = rxd.Region(
                h.allsec(), name="cell_mem", geometry=rxd.membrane()
            )
            Xecs = x[ecs]
            cyt_ecs_pump = rxd.MultiCompartmentReaction(
                Xcyt,
                Xecs,
                1.0 * scale * Xcyt / (1.0 + Xcyt),
                1.0 * scale * Xecs / (1.0 + Xecs),
                mass_action=False,
                membrane_flux=True,
                membrane=cyt_ecs_membrane,
            )
            model += [cyt_ecs_membrane, Xecs, cyt_ecs_pump]
        return model

    yield (neuron_instance, make_model)


def test_ecs_example(ecs_example):
    """Test ecs_example with fixed step methods"""

    (h, rxd, data, save_path), make_model = ecs_example
    model = make_model(0.2, 1.6)
    h.finitialize(-65)
    h.continuerun(1000)

    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ecs_example_cvode(ecs_example):
    """Test ecs_example with variable step methods"""

    (h, rxd, data, save_path), make_model = ecs_example
    model = make_model(0.2, 1.6)
    h.CVode().active(True)
    h.CVode().atol(1e-5)

    h.finitialize(-65)
    h.continuerun(1000)

    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ecs_example_alpha(ecs_example):
    """Test ecs_example with fixed step and inhomogeneous volume fraction methods"""

    (h, rxd, data, save_path), make_model = ecs_example
    model = make_model(lambda x, y, z: 0.2, 1.6)
    h.finitialize(-65)
    h.continuerun(1000)

    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ecs_example_cvode_alpha(ecs_example):
    """Test ecs_example with variable step and inhomogeneous volume fraction
    methods"""

    (h, rxd, data, save_path), make_model = ecs_example
    model = make_model(lambda x, y, z: 0.2, 1.6)
    h.CVode().active(True)
    h.CVode().atol(1e-5)

    h.finitialize(-65)
    h.continuerun(1000)

    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ecs_example_tort(ecs_example):
    """Test ecs_example with fixed step and inhomogeneous tortuosity methods"""

    (h, rxd, data, save_path), make_model = ecs_example
    model = make_model(lambda x, y, z: 0.2, 1.6)
    h.finitialize(-65)
    h.continuerun(1000)

    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ecs_example_cvode_tort(ecs_example):
    """Test ecs_example with variable step and inhomogeneous tortuosity methods"""

    (h, rxd, data, save_path), make_model = ecs_example
    model = make_model(lambda x, y, z: 0.2, 1.6)
    h.CVode().active(True)
    h.CVode().atol(1e-5)

    h.finitialize(-65)
    h.continuerun(1000)

    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ecs_nodelists(ecs_example):
    """Test accessing species nodes with both Node1D and NodeExtracellular"""

    (h, rxd, data, save_path), make_model = ecs_example
    model = make_model(0.2, 1.6)
    ecs, x = model[5], model[6]
    # test accessing NodeExtracellular from species with both 1D and ECS
    assert len(x.nodes(ecs)) == 64
    assert all([nd.region == ecs for nd in x.nodes(ecs)])
    # test accessing specific node by location
    nd = x[ecs].nodes((-38, 27, 27))[0]
    assert (nd.x3d, nd.y3d, nd.z3d) == (-38.5, 27.5, 27.5)


def test_ecs_example_dynamic_tort(ecs_example):
    """Test ecs_example with dynamic tortuosity"""

    (h, rxd, data, save_path), make_model = ecs_example
    model = make_model(lambda x, y, z: 1.0, 1)
    h.finitialize(-65)
    (
        cell1,
        cell2,
        cyt,
        org,
        cyt_org_membrane,
        ecs,
        x,
        Xcyt,
        Xorg,
        createX,
        cell1_param,
        createX,
        cyt_org_leak,
    ) = model
    perm = rxd.Species(ecs, name="perm", initial=1.0 / 1.6**2)
    ecs.permeability = perm
    h.continuerun(1000)

    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ecs_example_dynamic_alpha(ecs_example):
    """Test ecs_example with fixed step and inhomogeneous tortuosity methods"""

    (h, rxd, data, save_path), make_model = ecs_example
    model = make_model(lambda x, y, z: 1.0, 1.6)
    h.finitialize(-65)
    (
        cell1,
        cell2,
        cyt,
        org,
        cyt_org_membrane,
        ecs,
        x,
        Xcyt,
        Xorg,
        createX,
        cell1_param,
        createX,
        cyt_org_leak,
    ) = model
    alpha = rxd.Species(ecs, name="alpha", initial=0.2)
    ecs.alpha = alpha
    h.continuerun(1000)

    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ecs_multi_example(ecs_example):
    """Test ecs_example with multicompartment reactions and fixed step methods"""

    (h, rxd, data, save_path), make_model = ecs_example
    model = make_model(0.2, 1.6, mech=False)
    h.finitialize(-65)
    h.continuerun(1000)

    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ecs_multi_example_cvode(ecs_example):
    """Test ecs_example with multicompartment reactions and variable step methods"""

    (h, rxd, data, save_path), make_model = ecs_example
    model = make_model(0.2, 1.6)
    h.CVode().active(True)
    h.CVode().atol(1e-5)

    h.finitialize(-65)
    h.continuerun(1000)

    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol


def test_ecs_example_extracellular_multi(ecs_example):
    """Test multicompartment reaction with extracellular sources and destinations."""
    (h, rxd, data, save_path), make_model = ecs_example
    model = make_model(0.2, 1.6, mech=False)
    h.finitialize(-65)
    (
        cell1,
        cell2,
        cyt,
        org,
        cyt_org_membrane,
        ecs,
        x,
        Xcyt,
        Xorg,
        createX,
        cell1_param,
        createX,
        cyt_org_leak,
        cyt_ecs_membrane,
        Xecs,
        cyt_to_ecs_pump,
    ) = model
    y = rxd.State([ecs], name="y", charge=1, initial=100)
    e = 1.60217662e-19
    scale = 1e-14 / e
    ecs_ecs_pump = rxd.MultiCompartmentReaction(
        y[ecs],
        Xecs,
        1 * scale * Xcyt / (1.0 + Xcyt),
        mass_action=False,
        membrane_flux=False,
        membrane=cyt_ecs_membrane,
    )
    h.finitialize(-65)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol
