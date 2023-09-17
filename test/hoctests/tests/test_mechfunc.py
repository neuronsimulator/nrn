# test pp.func(...) and sec(x).mech.func(...)

from neuron import h


def fc(name, m):  # callable
    return getattr(name, m)


def varref(name, m):  # variable reference
    return getattr("_ref_" + name, m)


def model():  # 3 cables each with nseg=2
    cables = [h.Section(name="cable%d" % i) for i in range(3)]
    mechs = []
    for c in cables:
        c.nseg = 2
        c.insert("sdata")
        c.insert("sdatats")
        for seg in c:
            mechs.append(h.SData(seg))
            mechs.append(h.SDataTS(seg))
            mechs.append(seg.sdata)
            mechs.append(seg.sdatats)
    return mechs  # cable section stay in existence since mechs ref them


def test1():
    mechs = model()
    for i, m in enumerate(mechs):
        m.A(i, 2 * i, 3 * i)
    for i, m in enumerate(mechs):
        assert m.a == float(i)
        assert m.b == float(2 * i)
        assert m.c[1] == float(3 * i)
    h.topology()


if __name__ == "__main__":
    test1()
    h.topology()
