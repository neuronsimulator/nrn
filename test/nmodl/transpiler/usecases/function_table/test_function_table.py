from neuron import h, gui

import numpy as np
import scipy


def make_callable(inst, name, mech_name):
    if inst is None:
        return getattr(h, f"{name}_{mech_name}")
    else:
        return getattr(inst, f"{name}")


def make_callbacks(inst, name, mech_name):
    set_table = make_callable(inst, f"table_{name}", mech_name)
    eval_table = make_callable(inst, name, mech_name)

    return set_table, eval_table


def check_constant_1d(make_inst, mech_name):
    s = h.Section()
    s.insert("function_table")

    inst = make_inst(s)
    set_table, eval_table = make_callbacks(inst, "cnst1", mech_name)

    c = 42.0
    set_table(c)

    for vv in np.linspace(-10.0, 10.0, 14):
        np.testing.assert_equal(eval_table(vv), c)


def check_constant_2d(make_inst, mech_name):
    s = h.Section()
    s.insert("function_table")

    inst = make_inst(s)
    set_table, eval_table = make_callbacks(inst, "cnst2", mech_name)

    c = 42.0
    set_table(c)

    for vv in np.linspace(-10.0, 10.0, 7):
        for xx in np.linspace(-20.0, 10.0, 9):
            np.testing.assert_equal(eval_table(vv, xx), c)


def check_1d(make_inst, mech_name):
    s = h.Section()
    s.insert("function_table")

    inst = make_inst(s)
    set_table, eval_table = make_callbacks(inst, "tau1", mech_name)

    v = np.array([0.0, 1.0])
    tau1 = np.array([1.0, 2.0])

    set_table(h.Vector(tau1), h.Vector(v))

    for vv in np.linspace(v[0], v[-1], 20):
        expected = np.interp(vv, v, tau1)
        actual = eval_table(vv)

        np.testing.assert_approx_equal(actual, expected, significant=11)


def check_2d(make_inst, mech_name):
    s = h.Section()
    s.insert("function_table")

    inst = make_inst(s)
    set_table, eval_table = make_callbacks(inst, "tau2", mech_name)
    eval_use_table = make_callable(inst, "use_tau2", mech_name)

    if inst is None:
        setdata = getattr(h, f"setdata_{mech_name}")
        setdata(s(0.5))

    v = np.array([0.0, 1.0])
    x = np.array([1.0, 2.0, 3.0])

    tau2 = np.array([[1.0, 2.0, 3.0], [20.0, 30.0, 40.0]])

    # h.Matrix seems to be column major.
    hoc_tau2 = h.Matrix(*tau2.shape)
    hoc_tau2.from_vector(h.Vector(tau2.transpose().reshape(-1)))

    set_table(hoc_tau2._ref_x[0][0], v.size, v[0], v[-1], x.size, x[0], x[-1])

    for vv in np.linspace(v[0], v[-1], 20):
        for xx in np.linspace(x[0], x[-1], 20):
            expected = scipy.interpolate.interpn((v, x), tau2, (vv, xx))
            actual = eval_table(vv, xx)
            actual_indirect = eval_use_table(vv, xx)

            np.testing.assert_approx_equal(actual, expected, significant=11)
            np.testing.assert_approx_equal(actual_indirect, expected, significant=11)


def test_function_table():
    variations = [
        (lambda s: None, "function_table"),
        (lambda s: s(0.5).function_table, "function_table"),
        (lambda s: h.point_function_table(s(0.5)), "point_function_table"),
        (lambda s: h.art_function_table(s(0.5)), "art_function_table"),
    ]

    for make_instance, mech_name in variations:
        check_constant_1d(make_instance, mech_name)
        check_constant_2d(make_instance, mech_name)

        check_1d(make_instance, mech_name)
        check_2d(make_instance, mech_name)


if __name__ == "__main__":
    test_function_table()
