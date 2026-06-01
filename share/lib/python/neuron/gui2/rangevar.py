from neuron import h

try:
    from neuron.rxd import species

    have_rxd = True
except:
    have_rxd = False


def get_mech_names():
    _mech_names = []
    mt = h.MechanismType(0)
    mname = h.ref("")
    for i in range(int(mt.count())):
        mt.select(i)
        mt.selected(mname)
        _mech_names.append(mname[0])
    return _mech_names


def get_rangevars(mech_name):
    ms = h.MechanismStandard(mech_name)
    suffix = "_" + mech_name
    lensuffix = len(suffix)
    mname = h.ref("")
    result = []
    for i in range(int(ms.count())):
        ms.name(mname, i)
        my_mname = mname[0]
        if my_mname.endswith(suffix):
            my_mname = my_mname[:-lensuffix]
        result.append(
            {
                "type": "NEURONMECH",
                "mech": mech_name,
                "var": my_mname,
                "name": mech_name + "." + my_mname,
            }
        )
    return result


def _get_ptrs(plot_what):
    ptrs = []
    if plot_what["type"] == "NEURON":
        plot_what_var = "_ref_" + plot_what["var"]
        for sec in h.allsec():
            try:
                getattr(sec(0.5), plot_what_var)
                bad_section = False
            except NameError:
                bad_section = True
            except AttributeError:
                bad_section = True
            if not bad_section:
                ptrs += [getattr(seg, plot_what_var) for seg in sec]
            else:
                ptrs += [None] * sec.nseg
    elif plot_what["type"] == "SECVAR":
        plot_what_var = "_ref_" + plot_what["var"]
        for sec in h.allsec():
            ptrs += [getattr(sec, plot_what_var)] * sec.nseg
    elif plot_what["type"] == "NEURONMECH":
        plot_what_mech = plot_what["mech"]
        plot_what_var = "_ref_" + plot_what["var"]
        for sec in h.allsec():
            for seg in sec:
                try:
                    mech = getattr(seg, plot_what_mech)
                except:
                    ptrs.append(None)
                    continue
                ptrs.append(getattr(mech, plot_what_var))
    elif plot_what["type"] == "NEURONRXD":
        # TODO: there has to be something more efficient than this
        # TODO: this probably won't work well with 3D nodes
        sp = plot_what["species"]
        region = plot_what["region"]
        for s in species._all_species:
            s = s()
            if s is None:
                continue
            if s.name == sp:
                rs = s._regions
                if hasattr(rs, "__len__"):
                    rs = list(rs)
                else:
                    rs = list([rs])
                for r in rs:
                    if r._name == region:
                        base_obj = s[r].nodes
                        for sec in h.allsec():
                            sec_obj = base_obj(sec)
                            if sec_obj:
                                for seg in sec:
                                    nodes = sec_obj(seg.x)
                                    if len(nodes) != 1:
                                        print("gui: node length mismatch; 3D?")
                                    if not nodes:
                                        ptrs.append(None)
                                    else:
                                        # TODO: change this to use molecules when appropriate (stochastic sims)
                                        ptrs.append(nodes[0]._ref_concentration)
                        break
                else:
                    continue
                break
        else:
            # no such species found
            for sec in h.allsec():
                ptrs += [None] * sec.nseg

    elif plot_what["type"] is None:
        # no such species found
        for sec in h.allsec():
            ptrs += [None] * sec.nseg

    else:
        print("gui: unknown plot_what type", plot_what)

    return ptrs


def _neuron_var(name):
    return {"type": "NEURON", "var": name, "name": name}


def _section_var(name):
    return {"type": "SECVAR", "var": name, "name": name}


def get_rxd_vars(secs):
    # TODO: filter by sections present
    result = []
    if have_rxd:
        for sp in species._all_species:
            sp = sp()
            if sp is not None and sp.name is not None:
                regions = sp._regions
                if hasattr(regions, "__len__"):
                    regions = list(regions)
                else:
                    regions = list([regions])
                result += [
                    {
                        "type": "NEURONRXD",
                        "name": f"{sp.name}[{r._name}]",
                        "species": sp.name,
                        "region": r._name,
                    }
                    for r in regions
                    if r._name is not None
                ]
    return result


def rangevars_present(secs):
    result = []
    for name in get_mech_names():
        for sec in secs:
            try:
                if hasattr(sec(0.5), name):
                    if name[-4:] != "_ion":
                        result += get_rangevars(name)
                    else:
                        ionname = name[:-4]
                        result += [
                            _neuron_var(name)
                            for name in [
                                "i" + ionname,
                                ionname + "i",
                                ionname + "o",
                                "e" + ionname,
                            ]
                        ]
                    break
            except NameError:
                pass
    result += get_rxd_vars(secs)
    return [{"type": None, "name": "No coloration"}] + sorted(
        [_neuron_var("cm"), _neuron_var("diam"), _neuron_var("i_cap"), _neuron_var("v")]
        + result,
        key=lambda n: n["name"].lower(),
    )  # _section_var('Ra')
