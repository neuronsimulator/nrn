from neuron import h

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


sl = test()
