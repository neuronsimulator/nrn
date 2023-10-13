NA_modern = 6.02214076e23


def NA():
    try:
        from neuron import h

        val = h.Avogadro_constant
    except:
        val = NA_modern
    return val


def molecules_per_mM_um3():
    # converting from mM um^3 to molecules
    # = 6.02214076e23 * 1000. / 1.e18 / 1000
    # = avogadro * (L / m^3) * (m^3 / um^3) * (mM / M)
    return NA() / 1e18
