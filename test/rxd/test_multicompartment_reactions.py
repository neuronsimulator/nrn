from testutils import compare_data, tol


def test_multicompartment_reactions(neuron_instance):
    """A tests of mulicompartment reactions using an intracellular Ca model.

    Test based on example copied from the RxD tutorial;
    http://www.neuron.yale.edu/neuron/static/docs/rxd/index.html
    Where 1D intracellular space is divided into cytsol and endoplasmic
    reticulum. Calcium is transported between the regions by multicompartment
    reactions for a leak, SERCA pump and IP3 receptor.
    """

    h, rxd, data, save_path = neuron_instance
    sec = h.Section()
    sec.L = 100
    sec.diam = 1
    sec.nseg = 100

    h.CVode().active(1)
    h.CVode().atol(1e-4)

    caDiff = 0.016
    ip3Diff = 0.283
    cac_init = 1.0e-4
    ip3_init = 0.1
    gip3r = 12040
    gserca = 0.3913
    gleak = 6.020
    kserca = 0.1
    kip3 = 0.15
    kact = 0.4
    ip3rtau = 2000.0

    # These parameters where missing in the tutorial so arbitrary values were chosen
    # any resemblance to experimental values is purely coincidental.
    fc = 0.7
    fe = 0.3
    caCYT_init = 0.1

    cyt = rxd.Region(
        h.allsec(),
        name="cyt",
        nrn_region="i",
        geometry=rxd.FractionalVolume(fc, surface_fraction=1),
    )

    er = rxd.Region(h.allsec(), name="er", geometry=rxd.FractionalVolume(fe / 2.0))
    cyt_er_membrane = rxd.Region(
        h.allsec(), name="mem", geometry=rxd.ScalableBorder(1, on_cell_surface=False)
    )
    ca = rxd.Species([cyt, er], d=caDiff, name="ca", charge=2, initial=caCYT_init)

    ip3 = rxd.Species(cyt, d=ip3Diff, name="ip3", initial=ip3_init)
    ip3r_gate_state = rxd.Species(cyt_er_membrane, name="gate", initial=0.8)
    minf = ip3[cyt] * 1000.0 * ca[cyt] / (ip3[cyt] + kip3) / (1000.0 * ca[cyt] + kact)
    k = gip3r * (minf * ip3r_gate_state[cyt_er_membrane]) ** 3

    ip3r = rxd.MultiCompartmentReaction(ca[er], ca[cyt], k, k, membrane=cyt_er_membrane)

    serca = rxd.MultiCompartmentReaction(
        ca[cyt],
        ca[er],
        gserca / ((kserca / (1000.0 * ca[cyt])) ** 2 + 1),
        membrane=cyt_er_membrane,
        custom_dynamics=True,
    )
    leak = rxd.MultiCompartmentReaction(
        ca[er], ca[cyt], gleak, gleak, membrane=cyt_er_membrane
    )
    # test the SpeciesOnRegion remains in scope
    ip3rg = rxd.Rate(
        ip3r_gate_state[cyt_er_membrane],
        (1.0 / (1 + 1000.0 * ca[cyt] / (0.3)) - ip3r_gate_state[cyt_er_membrane])
        / ip3rtau,
    )

    h.finitialize(-65)
    assert (
        abs(
            serca._evaluate(
                location=(cyt, sec, 0.5), instruction="do_1d", units="mM/ms"
            )
            - 1.1818723974275922e-06
        )
        < tol
    )
    cae_init = (0.0017 - cac_init * fc) / fe
    ca[er].concentration = cae_init

    for node in ip3.nodes:
        if node.x < 0.2:
            node.concentration = 2

    h.CVode().re_init()

    h.continuerun(1000)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol
