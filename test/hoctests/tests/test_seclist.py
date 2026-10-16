from neuron import h


def test_sectionlist_len_size():
    """Check to make sure len(sectionlist) and SectionList::size() handle deleted sections

    See: https://github.com/neuronsimulator/nrn/issues/3522
    """
    soma = h.Section("soma")
    dend = h.Section("dend")

    sl = h.SectionList()
    sl.append(soma)
    sl.append(dend)
    assert len(sl) == 2
    assert sl.size() == 2

    soma = None
    assert len(sl) == 1
    assert isinstance(len(sl), int)
    assert sl.size() == 1
    assert isinstance(sl.size(), int)


secs = [h.Section() for _ in range(10)]


def test():
    sl = h.SectionList()
    sl.allroots()
    assert len(sl) == len(list(sl))
    b = [(s1, s2) for s1 in sl for s2 in sl]
    n = len(sl)
    for i in range(n):
        for j in range(n):
            assert b[i * n + j][0] == b[i + j * n][1]
    return sl


if __name__ == "__main__":
    test_sectionlist_len_size()
    sl = test()
