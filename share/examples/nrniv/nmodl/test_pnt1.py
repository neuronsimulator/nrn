import pytest

from neuron import h


def test_pnt1():
    a = h.Section()
    vec = h.Vector(10).indgen().add(100)
    tp = []
    for i in range(0, 10):
        tp.append(h.t1(a(0.5)))

    a.v = 10

    tp[2].f1()

    # h.setpointer(a(.5)._ref_v, 'p1', tp[0])
    tp[0]._ref_p1 = a(0.5)._ref_v
    for i in range(1, 10):
        # h.setpointer(vec._ref_x[i], 'p1', tp[i])
        tp[i]._ref_p1 = vec._ref_x[i]

    tp[2].p1 = 25

    z = h.t1()

    with pytest.raises(TypeError):
        h.setpointer(a(0.5)._ref_v, "p1", z)

    with pytest.raises(TypeError):
        z._ref_p1 = a(0.5)._ref_v


if __name__ == "__main__":
    test_pnt1()
