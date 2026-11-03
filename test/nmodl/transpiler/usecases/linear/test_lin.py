from neuron import h, gui

import numpy as np


def test_lin():
    s = h.Section()

    s.insert("lin")

    h.finitialize()

    a = h.a_lin
    b = h.b_lin
    c = h.c_lin
    d = h.d_lin

    A = np.array([[a, b], [c, d]])
    exact = np.linalg.solve(A, [0, 0])

    x = s(0.5).lin.xx
    y = s(0.5).lin.yy

    assert x == exact[0]
    assert y == exact[1]


if __name__ == "__main__":
    test_lin()
