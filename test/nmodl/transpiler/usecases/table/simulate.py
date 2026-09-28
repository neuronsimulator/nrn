import numpy as np
from neuron import gui
from neuron import h


def check_solution(y_no_table, y_table, rtol):
    assert np.allclose(
        y_no_table, y_table, rtol=rtol
    ), f"{y_no_table} != {y_table}, delta: {y_table - y_no_table}"

    # The approximation by a linear lookup table can't be exact.
    assert not np.allclose(
        y_no_table, y_table, rtol=1e-10
    ), f"{y_no_table} == {y_table}"


def check_table(c1, c2, x, mech_name, evaluate_table):
    setattr(h, f"c1_{mech_name}", 1)
    setattr(h, f"c2_{mech_name}", 2)

    setattr(h, f"usetable_{mech_name}", 0)
    y_no_table = np.array([evaluate_table(i) for i in x])

    setattr(h, f"usetable_{mech_name}", 1)
    y_table = np.array([evaluate_table(i) for i in x])

    check_solution(y_table, y_no_table, rtol=1e-4)

    # verify that the table just "clips" the values outside of the range
    assert np.all(evaluate_table(x[0] - 10) == y_table[0])
    assert np.all(evaluate_table(x[-1] + 10) == y_table[-1])


def check_function(mech_name, make_instance):
    s = h.Section()
    obj = make_instance(s)

    x = np.linspace(-3, 5, 18)
    assert x[0] == -3.0
    assert x[-1] == 5.0

    check_table(1, 2, x, mech_name, obj.quadratic)
    check_table(2, 2, x, mech_name, obj.quadratic)
    check_table(2, 3, x, mech_name, obj.quadratic)


def make_density_instance(s):
    s.insert("tbl")
    return s(0.5).tbl


def make_point_instance(s):
    return h.tbl_point_process(s(0.5))


def test_function():
    check_function("tbl", make_density_instance)
    check_function("tbl_point_process", make_point_instance)


def check_procedure(mech_name, make_instance):
    s = h.Section()
    obj = make_instance(s)

    def evaluate_table(x):
        obj.sinusoidal(x)
        return np.array((obj.v1, obj.v2))

    x = np.linspace(-4, 6, 18)
    assert x[0] == -4.0
    assert x[-1] == 6.0

    check_table(1, 2, x, mech_name, evaluate_table)
    check_table(2, 2, x, mech_name, evaluate_table)
    check_table(2, 3, x, mech_name, evaluate_table)


def test_procedure():
    check_procedure("tbl", make_density_instance)


def simulate():
    s = h.Section()
    s.insert("tbl")

    vvec = h.Vector().record(s(0.5)._ref_v)

    # run without a table
    h.usetable_tbl = 0
    h.run()
    v_exact = np.array(vvec.as_numpy())

    # run with a table
    h.usetable_tbl = 1
    h.run()
    v_table = np.array(vvec.as_numpy())

    check_solution(v_table, v_exact, rtol=1e-2)


if __name__ == "__main__":
    test_function()
    test_procedure()

    simulate()
