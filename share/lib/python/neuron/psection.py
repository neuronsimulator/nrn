def psection(sec):
    from neuron import h

    try:
        from neuron import rxd
        from neuron.rxd import region, species
        from neuron.rxd import rxd as rxd_module

        have_rxd = True
    except:
        have_rxd = False

    results = {}

    mname = h.ref("")

    # point processes
    pps = {}
    mt = h.MechanismType(1)
    for i in range(int(mt.count())):
        mt.select(i)
        mypps = set()
        pp = mt.pp_begin(sec=sec)
        while pp is not None:
            mypps.add(pp)
            pp = mt.pp_next()
        if mypps:
            mt.selected(mname)
            pps[mname[0]] = mypps
    results["point_processes"] = pps

    center_seg_dir = dir(sec(0.5))

    mechs_present = []

    # membrane mechanisms
    mt = h.MechanismType(0)

    for i in range(int(mt.count())):
        mt.select(i)
        mt.selected(mname)
        name = mname[0]
        if name in center_seg_dir:
            mechs_present.append(name)

    results["density_mechs"] = {}
    results["ions"] = {}

    for mech in mechs_present:
        my_results = {}
        ms = h.MechanismStandard(mech, 0)
        for j in range(int(ms.count())):
            n = int(ms.name(mname, j))
            name = mname[0]
            pvals = []
            # TODO: technically this is assuming everything that ends with _ion
            #       is an ion. Check this.
            if mech.endswith("_ion"):
                pvals = [getattr(seg, name) for seg in sec]
            else:
                mechname = name  # + '_' + mech
                for seg in sec:
                    if n > 1:
                        pvals.append([getattr(seg, mechname)[i] for i in range(n)])
                    else:
                        pvals.append(getattr(seg, mechname))
            my_results[
                name[: -(len(mech) + 1)] if name.endswith("_" + mech) else name
            ] = pvals
        # TODO: should be a better way of testing if an ion
        if mech.endswith("_ion"):
            results["ions"][mech[:-4]] = my_results
        else:
            results["density_mechs"][mech] = my_results

    morphology = {
        "L": sec.L,
        "diam": [seg.diam for seg in sec],
        "pts3d": [
            (sec.x3d(i), sec.y3d(i), sec.z3d(i), sec.diam3d(i))
            for i in range(sec.n3d())
        ],
        "parent": sec.parentseg(),
        "trueparent": sec.trueparentseg(),
    }

    results["morphology"] = morphology
    results["nseg"] = sec.nseg
    results["Ra"] = sec.Ra
    results["cm"] = [seg.cm for seg in sec]

    if have_rxd:
        regions = {
            r() for r in region._all_regions if r() is not None and sec in r().secs
        }
        results["regions"] = regions

        my_species = []
        for sp in species._all_species:
            sp = sp()
            if sp is not None:
                sp_regions = sp._regions
                if not hasattr(sp_regions, "__len__"):
                    sp_regions = [sp_regions]
                if any(r in sp_regions for r in regions):
                    my_species.append(sp)
        results["species"] = set(my_species)
        results["name"] = sec.hname()
        results["hoc_internal_name"] = sec.hoc_internal_name()
        results["cell"] = sec.cell()

        # all_active_reactions = [r() for r in rxd_module._all_reactions if r() is not None]

    return results


if __name__ == "__main__":
    from pprint import pprint
    from neuron import h, rxd

    soma = h.Section(name="soma")
    soma.insert("hh")
    soma.insert("pas")
    soma.nseg = 3
    iclamp = h.IClamp(soma(0.3))
    gaba = h.ExpSyn(soma(0.7))
    dend = h.Section(name="dend")
    dend.connect(soma)
    cyt = rxd.Region([dend], name="cyt", nrn_region="i")
    er = rxd.Region([dend], name="er")
    na = rxd.Species(cyt, name="na", charge=1)
    ca = rxd.Species([cyt, er], name="ca", charge=2)
    h.finitialize(-65)
    pprint(psection(soma))
    print("\n")
    pprint(psection(dend))
