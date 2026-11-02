from testutils import compare_data, tol


def test_multicompartment_reactions_aligment(neuron_instance):
    """A test for multicompartment reactions where one regions has more
    sections than the other.
    """

    h, rxd, data, save_path = neuron_instance

    # Dendrite and spine
    dend = h.Section(name="dend")
    dend.nseg = 101
    dend.pt3dclear()
    dend.pt3dadd(-10, 0, 0, 1)
    dend.pt3dadd(10, 0, 0, 1)

    spine = h.Section(name="spine")
    spine.nseg = 11
    spine.pt3dclear()
    spine.pt3dadd(0, 0, 0, 0.2)
    spine.pt3dadd(0, 1, 0, 0.2)
    spine.connect(dend(0.5))

    # Where -- define the shells and borders
    N = 5  # number of shells -- must be >= 2

    # Where -- shells and border between them
    shells = []
    border = []
    for i in range(N - 1):
        shells.append(
            rxd.Region(
                [dend],
                name=f"shell{i}",
                geometry=rxd.Shell(float(i) / N, (1.0 + i) / N),
            )
        )
        border.append(
            rxd.Region(
                [dend],
                name=f"border{i}",
                geometry=rxd.FixedPerimeter(2.0 * h.PI * (1.0 + i)),
            )
        )

    shells.append(
        rxd.Region(
            [dend, spine],
            nrn_region="i",
            name=f"shell{N - 1}",
            geometry=rxd.MultipleGeometry(
                secs=[dend, spine], geos=[rxd.Shell((N - 1.0) / N, 1), rxd.inside]
            ),
        )
    )

    # Who -- calcium with an inhomogeneous initial condition
    Dca = 0.6  # um^2/ms

    # scale factor so the flux (Dca/dr)*Ca has units molecules/um^2/ms (where dr is the thickness of the shell)
    mM_to_mol_per_um = 6.0221409e23 * 1e-18
    ca = rxd.Species(
        shells,
        d=Dca,
        name="ca",
        charge=2,
        initial=lambda nd: 1000e-6 if nd.segment in spine else 100e-6,
    )

    # What -- use reactions to setup diffusion between the shells
    cas = []  # calcium on the shells
    for reg in shells:
        cas.append(ca[reg])

    # create the multi-compartment reactions between the pairs of shells
    diffusions = []
    for i in range(N - 1):
        diffusions.append(
            rxd.MultiCompartmentReaction(
                cas[i],
                cas[i + 1],
                mM_to_mol_per_um * Dca,
                mM_to_mol_per_um * Dca,
                border=border[i],
            )
        )

    h.finitialize(-65)
    h.continuerun(2.5)
    if not save_path:
        max_err = compare_data(data)
        assert max_err < tol
