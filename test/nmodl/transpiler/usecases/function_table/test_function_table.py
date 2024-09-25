from neuron import h, gui

import numpy as np
import scipy


def test_constant_1d():
    s = h.Section()
    s.insert("function_table")

    c = 42.0
    h.table_cnst1_function_table(c)

    for vv in np.linspace(-10.0, 10.0, 14):
        np.testing.assert_equal(h.cnst1_function_table(vv), c)


def test_constant_2d():
    s = h.Section()
    s.insert("function_table")

    c = 42.0
    h.table_cnst2_function_table(c)

    for vv in np.linspace(-10.0, 10.0, 7):
        for xx in np.linspace(-20.0, 10.0, 9):
            np.testing.assert_equal(h.cnst2_function_table(vv, xx), c)


def test_1d():
    v = np.array([0.0, 1.0])
    tau1 = np.array([1.0, 2.0])

    h.table_tau1_function_table(h.Vector(tau1), h.Vector(v))

    for vv in np.linspace(v[0], v[-1], 20):
        expected = np.interp(vv, v, tau1)
        actual = h.tau1_function_table(vv)

        np.testing.assert_approx_equal(actual, expected, significant=11)


def test_2d():
    v = np.array([0.0, 1.0])
    x = np.array([1.0, 2.0, 3.0])

    tau2 = np.array([[1.0, 2.0, 3.0], [20.0, 30.0, 40.0]])

    # h.Matrix seems to be column major.
    hoc_tau2 = h.Matrix(*tau2.shape)
    hoc_tau2.from_vector(h.Vector(tau2.transpose().reshape(-1)))

    h.table_tau2_function_table(
        hoc_tau2._ref_x[0][0], v.size, v[0], v[-1], x.size, x[0], x[-1]
    )

    for vv in np.linspace(v[0], v[-1], 20):
        for xx in np.linspace(x[0], x[-1], 20):
            expected = scipy.interpolate.interpn((v, x), tau2, (vv, xx))
            actual = h.tau2_function_table(vv, xx)

            np.testing.assert_approx_equal(actual, expected, significant=11)


def test_use_table():
    s = h.Section()
    s.insert("function_table")

    h.setdata_function_table(s(0.5))

    vv, xx = 0.33, 2.24

    expected = h.tau2_function_table(vv, xx)
    actual = h.use_tau2_function_table(vv, xx)
    np.testing.assert_approx_equal(actual, expected, significant=11)


if __name__ == "__main__":
    test_constant_1d()
    test_constant_2d()

    test_1d()
    test_2d()

    test_use_table()
