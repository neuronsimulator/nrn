def test_savestate_basic(neuron_nosave_instance):
    """Test SaveState works rxd when rxd states are not saved/restored by save
    state"""
    h, rxd, save_path = neuron_nosave_instance
    vtrap, v = rxd.rxdmath.vtrap, rxd.v
    exp, log = rxd.rxdmath.exp, rxd.rxdmath.log

    def saveSS():
        svst = h.SaveState()
        svst.save()
        svst_extra = save_rxd_extra()  # only for below model with one segment.
        return svst, svst_extra

    def restoreSS(svst, svst_extra):
        svst.restore()
        restore_rxd_extra(svst_extra)  # only for below model with one segment.
        h.fcurrent()  # failed hope was that this would properly initialize the currents.

    # parameters
    h.celsius = 6.3
    e = 1.60217662e-19
    scale = 1e-14 / e
    gnabar = 0.12 * scale  # molecules/um2 ms mV
    gkbar = 0.036 * scale
    gl = 0.0003 * scale
    el = -54.3
    q10 = 3.0 ** ((h.celsius - 6.3) / 10.0)

    # sodium activation 'm'
    alpha = 0.1 * vtrap(-(v + 40.0), 10)
    beta = 4.0 * exp(-(v + 65) / 18.0)
    mtau = 1.0 / (q10 * (alpha + beta))
    minf = alpha / (alpha + beta)

    # sodium inactivation 'h'
    alpha = 0.07 * exp(-(v + 65.0) / 20.0)
    beta = 1.0 / (exp(-(v + 35.0) / 10.0) + 1.0)
    htau = 1.0 / (q10 * (alpha + beta))
    hinf = alpha / (alpha + beta)

    # potassium activation 'n'
    alpha = 0.01 * vtrap(-(v + 55.0), 10.0)
    beta = 0.125 * exp(-(v + 65.0) / 80.0)
    ntau = 1.0 / (q10 * (alpha + beta))
    ninf = alpha / (alpha + beta)

    somaA = h.Section("somaA")
    somaA.pt3dclear()
    somaA.pt3dadd(-90, 0, 0, 30)
    somaA.pt3dadd(-60, 0, 0, 30)
    somaA.nseg = 1  # 11

    # Where?
    # intracellular
    cyt = rxd.Region(h.allsec(), name="cyt", nrn_region="i")

    # membrane
    mem = rxd.Region(h.allsec(), name="cell_mem", geometry=rxd.membrane())

    # extracellular
    ecs = rxd.Extracellular(-100, -100, -100, 100, 100, 100, dx=33)

    # Who?  ions & gates

    # intracellular sodium & potassium
    na = rxd.Species([cyt, mem], name="na", d=1, charge=1, initial=10)
    k = rxd.Species([cyt, mem], name="k", d=1, charge=1, initial=54.4)

    # extracellular parameters provide a constant concentration for the Nernst potential and reactions.
    kecs = rxd.Parameter(ecs, name="k", charge=1, value=2.5)
    naecs = rxd.Parameter(ecs, name="na", charge=1, value=140)

    # an undistinguished charged ion for the leak current
    x = rxd.Species([cyt, mem, ecs], name="x", charge=1)

    # define the various species and parameters on the intracellular and extracellular regions
    ki, ko, nai, nao, xi, xo = k[cyt], kecs[ecs], na[cyt], naecs[ecs], x[cyt], x[ecs]

    # the gating states
    ngate = rxd.State([cyt, mem], name="ngate", initial=0.24458654944007166)
    mgate = rxd.State([cyt, mem], name="mgate", initial=0.028905534475191907)
    hgate = rxd.State([cyt, mem], name="hgate", initial=0.7540796658225246)

    # parameter to limit rxd reaction to somaA
    paramA = rxd.Parameter(
        [cyt, mem], name="paramA", value=lambda nd: 1 if nd.segment in somaA else 0
    )

    # What? gates and currents
    m_gate = rxd.Rate(mgate, (minf - mgate) / mtau)
    h_gate = rxd.Rate(hgate, (hinf - hgate) / htau)
    n_gate = rxd.Rate(ngate, (ninf - ngate) / ntau)

    # Nernst potentials
    ena = 1e3 * h.R * (h.celsius + 273.15) * log(nao / nai) / h.FARADAY
    ek = 1e3 * h.R * (h.celsius + 273.15) * log(ko / ki) / h.FARADAY

    gna = paramA * gnabar * mgate**3 * hgate
    gk = paramA * gkbar * ngate**4

    na_current = rxd.MultiCompartmentReaction(
        nai, nao, gna * (v - ena), mass_action=False, membrane=mem, membrane_flux=True
    )
    k_current = rxd.MultiCompartmentReaction(
        ki, ko, gk * (v - ek), mass_action=False, membrane=mem, membrane_flux=True
    )
    leak_current = rxd.MultiCompartmentReaction(
        xi,
        xo,
        paramA * gl * (v - el),
        mass_action=False,
        membrane=mem,
        membrane_flux=True,
    )

    def save_rxd_extra():
        # only for one segment
        sav_rxd_extra_ = []
        sav_rxd_extra_.append(mgate[cyt].nodes(somaA(0.5)).value)
        sav_rxd_extra_.append(ngate[cyt].nodes(somaA(0.5)).value)
        sav_rxd_extra_.append(hgate[cyt].nodes(somaA(0.5)).value)
        sav_rxd_extra_.append(mgate[mem].nodes(somaA(0.5)).value)
        sav_rxd_extra_.append(ngate[mem].nodes(somaA(0.5)).value)
        sav_rxd_extra_.append(hgate[mem].nodes(somaA(0.5)).value)
        return sav_rxd_extra_

    def restore_rxd_extra(sav_rxd_extra_):
        mgate[cyt].nodes(somaA(0.5)).value = sav_rxd_extra_[0]
        ngate[cyt].nodes(somaA(0.5)).value = sav_rxd_extra_[1]
        hgate[cyt].nodes(somaA(0.5)).value = sav_rxd_extra_[2]
        mgate[mem].nodes(somaA(0.5)).value = sav_rxd_extra_[3]
        ngate[mem].nodes(somaA(0.5)).value = sav_rxd_extra_[4]
        hgate[mem].nodes(somaA(0.5)).value = sav_rxd_extra_[5]
        k[cyt].nodes.value = somaA(0.5).ki
        na[cyt].nodes.value = somaA(0.5).nai

    # return list of current values
    def curval():
        a = [h.t, somaA(0.5).v]
        a.append(mgate[cyt].nodes(somaA(0.5)).value)
        a.append(ngate[cyt].nodes(somaA(0.5)).value)
        a.append(hgate[cyt].nodes(somaA(0.5)).value)
        a.append(somaA(0.5).ik)
        a.append(somaA(0.5).ina)
        return a

    def test_savestate_rxd():
        t1 = 7
        t2 = 10
        # record
        tvec = h.Vector().record(h._ref_t)
        vvecA = h.Vector().record(somaA(0.5)._ref_v)
        mvecA = h.Vector().record(mgate[cyt].nodes(somaA(0.5))._ref_value)
        nvecA = h.Vector().record(ngate[cyt].nodes(somaA(0.5))._ref_value)
        hvecA = h.Vector().record(hgate[cyt].nodes(somaA(0.5))._ref_value)
        kvecA = h.Vector().record(somaA(0.5)._ref_ik)
        navecA = h.Vector().record(somaA(0.5)._ref_ina)

        vecs = [tvec, vvecA, mvecA, nvecA, hvecA, kvecA, navecA]

        # Full run to 100 to get standard results.
        h.finitialize(-70)
        h.continuerun(t2)
        vecs_std = [i.c() for i in vecs]

        # Run to t1, savestate, continue run to t2.
        # compare results to standard results (i.e. savestate should
        # not corrupt the run.)
        h.finitialize(-70)
        h.continuerun(t1)
        sav_val = curval()
        print(sav_val)
        svst, svst_extra = saveSS()
        print(curval())
        h.continuerun(t2)
        for i, vec in enumerate(vecs):
            if not vec.eq(vecs_std[i]):
                print(
                    "vecs[%d] error %g" % (i, vecs[i].c().sub(vecs_std[i]).abs().sum())
                )
        for i, vec in enumerate(vecs):
            assert vec.eq(vecs_std[i])

        # Another run stopping at t1, resizing the record vectors to 0
        # and continuing to t2 to be the standard for a later run that will be
        # savestate restored at t1.
        h.finitialize(-70)
        h.continuerun(t1)
        print(curval())
        for vec in vecs:
            vec.resize(0)
        h.continuerun(t2)
        vecs_std_save = [i.c() for i in vecs]

        # Finally, savestate restore at t1, continue to t2, and
        # compare vecs to vecs_std_save
        h.finitialize(-70)
        print(curval())
        restoreSS(svst, svst_extra)
        print(curval())
        for vec in vecs:
            vec.resize(0)
        h.continuerun(t2)
        for i, vec in enumerate(vecs):
            if not vec.eq(vecs_std_save[i]):
                print(
                    "vecs[%d] error %g"
                    % (i, vecs[i].c().sub(vecs_std_save[i]).abs().sum())
                )
        for i, vec in enumerate(vecs):
            assert vec.eq(vecs_std_save[i])
            pass

        h.topology()
        return vecs, vecs_std_save

    # run the test
    test_savestate_rxd()


if __name__ == "__main__":
    from neuron import h, rxd

    h.load_file("stdrun.hoc")
    instance = h, rxd, None
    vecs, vecs_std_save = test_savestate_basic(instance)
