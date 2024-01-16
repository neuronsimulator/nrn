from neuron import h


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
                if size == 1:
                    print(name[0], i, varname[0], size, m.get(varname[0]))
                    assert getattr(s(0.5), varname[0]) == m.get(varname[0])
                else:
                    for j in range(size):
                        print(name[0], i, varname[0], j, size, m.get(varname[0]))
                        assert getattr(s, varname[0])[j] == m.get(varname[0], j)

    mnames = ["hh", "pas", "fastpas", "capacitance", "na_ion", "extracellular"]
    for vartype in [1, 2, 3, 0]:
        mechstd(vartype, mnames)
    for nlayer in [3, 1, 2]:
        h.nlayer_extracellular(nlayer)
        mechstd(0, ["extracellular"])


def test_mechstd():
    m = h.MechanismStandard("extracellular")
    m.set("vext", 5, 0)


def test_mech_getattro():
    s = h.Section("soma")
    s.insert("extracellular")
    s(0.5).extracellular.e = 10.0
    s(0.5).vext[1] = 7.0


if __name__ == "__main__":
    test_mechstd1()
#    test_mech_getattro()
#    test_mechstd()
