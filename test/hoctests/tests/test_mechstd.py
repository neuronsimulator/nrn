from neuron import h


def mkmodel(mnames):
    s = h.Section(name="dend")
    for name in mnames:
        if name != "capacitance":
            s.insert(name)
    return s


# for MechStan, mechname, varname, isarray, size var_ref_in_seg in variter(mslist, seg)
def variter(mslist, seg=None):
    for m in mslist:
        mname = h.ref("")
        m.name(mname)
        for i in range(m.count()):
            varname = h.ref("")
            size = m.name(varname, i)
            isarray = m.is_array(i)
            refname = "_ref_" + varname[0] + ("[0]" if isarray else "")
            ref = getattr(seg, refname) if seg else None
            yield m, mname[0], varname[0], isarray, size, ref


def test_mechstd1():  # default params
    def mechstd(vartype, mnames):
        print("\nvartype ", vartype)
        ms = [h.MechanismStandard(name, vartype) for name in mnames]
        s = h.Section()
        for name in mnames:
            if name != "capacitance":
                s.insert(name)

        for m in ms:
            name = h.ref("")
            m.name(name)
            for i in range(m.count()):
                varname = h.ref("")
                size = m.name(varname, i)
                if not m.is_array(i):
                    print(name[0], i, varname[0], size, m.get(varname[0]))
                    assert getattr(s(0.5), varname[0]) == m.get(varname[0])
                else:
                    for j in range(size):
                        print(name[0], i, varname[0], j, size, m.get(varname[0]))
                        assert getattr(s(0.5), varname[0])[j] == m.get(varname[0], j)

    mnames = ["hh", "pas", "fastpas", "capacitance", "na_ion", "extracellular"]
    for vartype in [1, 2, 3, 0]:
        mechstd(vartype, mnames)
    for nlayer in [3, 1, 2]:
        h.nlayer_extracellular(nlayer)
        mechstd(0, ["extracellular"])


def test_mechstd2():  # test set, get, in, out
    mnames = ["hh", "pas", "fastpas", "capacitance", "na_ion", "extracellular"]
    mslist = [h.MechanismStandard(name, 0) for name in mnames]
    # fill with distinct values
    val = 0.0
    for ms, mname, varname, isarray, size, ref in variter(mslist, None):
        for i in range(size):
            val += 0.1
            print(ms, mname, varname, isarray, size, ref, val)
            ms.set(varname, i, val)


if __name__ == "__main__":
    test_mechstd2()
