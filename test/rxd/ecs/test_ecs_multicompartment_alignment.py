import pytest

from rxd_testutils import compare_data, tol


@pytest.fixture
def multicompartment_alignment(neuron_instance):
    """A model where the group Regions used for reactions result in too many
    1D indices found for a MulticompartmentReaction with the ECS.
    """

    h, rxd, data, save_path = neuron_instance
    soma = h.Section(name="soma")
    soma.L = soma.diam = 25
    soma.nseg = 1
    soma.pt3dclear()
    soma.pt3dadd(-12.5, 0, 0, 5)
    soma.pt3dadd(-7.5, 0, 0, 5)

    dend = h.Section(name="dend")
    dend.nseg = 101
    dend.pt3dclear()
    dend.pt3dadd(-7.5, 0, 0, 1)
    dend.pt3dadd(12.5, 0, 0, 1)
    dend.connect(soma)

    # Model parameters
    Dk = 2.62  # um^2/ms
    dx = 5  # um ECS voxel size
    alpha = 0.2  # ECS free volume fraction
    tortuosity = 1.6  # ECS tortuosity
    Dkeff = Dk / tortuosity**2  # um^2/ms effective diffusion coeffishent

    # Where -- shells and border between them
    # cytosol
    cyt = rxd.Region(h.allsec(), nrn_region="i", name="cyt")

    # cell membrane -- between cyt and FHS
    mem = rxd.Region(h.allsec(), name="mem", geometry=rxd.membrane())

    # Frankenhaeuser-Hodgkin space -- a shell with diameter 1.2 times that of
    #                                 the section
    fhs = rxd.Region([soma], name="fhs", nrn_region="o", geometry=rxd.Shell(1, 1.2))

    # a border -- between FHS and ECS
    bor = rxd.Region([soma], name="bor", geometry=rxd.ScalableBorder(diam_scale=1.2))

    # the shared ECS
    ecs = rxd.Extracellular(
        -15, -15, -15, 15, 15, 15, dx=dx, tortuosity=1.6, volume_fraction=alpha
    )

    # Who -- Potassium
    # scale factor so the flux (Dk/dx/2)*K has units molecules/um^2/ms
    # where dx/2 is the average distance between the center of the ECS voxel
    # and the FHS
    mM_to_mol_per_um = 6.0221409e23 * 1e-18

    # k in the cyt and FHS will link to the ion representation in NEURON, i.e.
    # seg.ki and seg.ko
    k = rxd.Species(
        [cyt, fhs],
        d=Dk,
        name="k",
        charge=1,
        initial=lambda nd: 140.0 if nd.region == cyt else 3.0,
    )

    # kecs is a fake extracellular potassium, it will only changed by rxd
    # mechanisms
    kecs = rxd.Species([ecs], d=Dk, name="kecs", charge=1, initial=3.0)

    # What -- use reactions to setup diffusion
    # Here we assume the FHS free volume fraction and tortuoisty are 1 and the
    # the thickness of the shell is much smaller than the ECS voxel size.
    diffusion = rxd.MultiCompartmentReaction(
        k[fhs],
        kecs[ecs],
        mM_to_mol_per_um * (Dkeff * alpha / (dx / 2)),
        mM_to_mol_per_um * (Dkeff * alpha / (dx / 2)),
        membrane=bor,
    )
    model = (soma, dend, cyt, fhs, bor, k, kecs, diffusion)
    yield (neuron_instance, model)


def test_ecs_multicompartment_alignment(multicompartment_alignment):
    """Test ecs_multicompartment_alignment model"""

    neuron_instance, model = multicompartment_alignment
    h, rxd, data, save_path = neuron_instance
    h.finitialize(-70)
    h.continuerun(10)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol
