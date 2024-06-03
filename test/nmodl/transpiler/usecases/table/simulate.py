import numpy as np
from neuron import gui
from neuron import h


def setup_sim():
    section = h.Section()
    section.insert("tbl")

    return section


def check_solution(y_no_table, y_table, rtol1=1e-2, rtol2=1e-8):
    assert np.allclose(y_no_table, y_table, rtol=rtol1), f"{y_no_table} != {y_table}"
    # if the table is not used, we should get identical results, but we don't,
    # hence the assert below
    assert not np.allclose(
        y_no_table, y_table, rtol=rtol2
    ), f"Broken test logic: {y_no_table} == {y_table}"


def test_function():
    section = setup_sim()

    x = np.linspace(-3, 5, 500)

    func = section(0.5).tbl.quadratic

    h.c1_tbl = 1
    h.c2_tbl = 2

    h.usetable_tbl = 0
    y_no_table = np.array([func(i) for i in x])

    h.usetable_tbl = 1
    y_table = np.array([func(i) for i in x])

    check_solution(y_table, y_no_table, rtol1=1e-4)

    # verify that the table just "clips" the values outside of the range
    assert func(x[0] - 10) == y_table[0]
    assert func(x[-1] + 10) == y_table[-1]

    # change parameters and verify
    h.c1_tbl = 3
    h.c2_tbl = 4

    h.usetable_tbl = 0
    y_params_no_table = np.array([func(i) for i in x])

    h.usetable_tbl = 1
    y_params_table = np.array([func(i) for i in x])

    check_solution(y_params_table, y_params_no_table, rtol1=1e-4)


def test_procedure():
    section = setup_sim()

    x = np.linspace(-4, 6, 300)

    proc = section(0.5).tbl.sinusoidal

    def call_proc_return_values(arg):
        proc(arg)
        return section(0.5).tbl.v1, section(0.5).tbl.v2

    def check_table(procedure, **kwargs):
        for key, value in kwargs.items():
            setattr(h, f"{key}_tbl", value)

        h.usetable_tbl = 0
        values_no_table = np.array([procedure(i) for i in x])

        h.usetable_tbl = 1
        values_table = np.array([procedure(i) for i in x])

        assert np.allclose(
            values_no_table,
            values_table,
            rtol=1e-3,
        ), f"{values_no_table} != {values_table}"

        assert not np.allclose(
            values_no_table,
            values_table,
            rtol=1e-8,
        ), f"Broken test logic: {values_no_table} == {values_table}"

    check_table(call_proc_return_values, c1=1, c2=2)
    check_table(call_proc_return_values, c1=0.1, c2=0.3)


def simulate():
    s = setup_sim()

    h.k_tbl = -0.1
    h.d_tbl = -40
    s(0.5).tbl.gmax = 0.001

    vvec = h.Vector().record(s(0.5)._ref_v)
    tvec = h.Vector().record(h._ref_t)

    # run without a table
    h.usetable_tbl = 0
    h.run()

    t = np.array(tvec.as_numpy())
    v_exact = np.array(vvec.as_numpy())

    # run with a table
    h.usetable_tbl = 1
    h.run()

    v_table = np.array(vvec.as_numpy())

    check_solution(v_table, v_exact, rtol1=1e-2)

    # run without a table, and changing params
    h.usetable_tbl = 0
    h.k_tbl = -0.05
    h.d_tbl = -45

    h.run()

    v_params = np.array(vvec.as_numpy())

    # run with a table (same params as above)
    h.usetable_tbl = 1
    h.run()

    v_params_table = np.array(vvec.as_numpy())

    check_solution(v_params, v_params_table, rtol1=1e-2)


def plot_solution(t, y_exact, y):
    import matplotlib.pyplot as plt

    plt.plot(t, y_exact, label="exact")
    plt.plot(t, y, label="table", ls="--")
    plt.show()


if __name__ == "__main__":
    test_function()
    test_procedure()
    simulate()
