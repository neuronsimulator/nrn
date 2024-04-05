import itertools
import os

import numpy as np

from neuron import h, gui
from neuron import coreneuron
from neuron.tests.utils.strtobool import strtobool

enable_coreneuron_option = bool(
    strtobool(os.environ.get("NRN_TEST_ENABLE_CORENEURON", "true"))
)
enable_gpu_option = bool(strtobool(os.environ.get("CORENEURON_ENABLE_GPU", "false")))
file_mode_option = bool(strtobool(os.environ.get("NRN_TEST_FILE_MODE", "false")))
run_mode_option = os.environ.get("NRN_TEST_RUN_MODE", 2)
create_plots_option = bool(strtobool(os.environ.get("NRN_TEST_CREATE_PLOTS", "false")))


def test_array_variable_transfer(
    enable_coreneuron=enable_coreneuron_option,
    enable_gpu=enable_gpu_option,
    run_mode=run_mode_option,
    file_mode=file_mode_option,
    create_plots=create_plots_option,
):

    h("""create soma""")
    h.soma.L = 5.6419
    h.soma.diam = 5.6419
    h.soma.insert("red")
    h.soma.insert("green")
    h.soma.nseg = 3

    h.tstop = 6.0
    h.dt = 0.001

    # TODO check replay.

    c_red, c_green = "red", "green"
    m = 16

    # `upsilon` is an array variable with at least `m` elements. It's value is
    # approximately:
    def upsilon(i, tau, c, t):
        if c == c_red:
            return np.sin((i + 1.0) * t)
        elif c == c_green:
            return 2.0 + 8 * tau + np.sin((i / 3.0 + 1) * t)

    cs = [c_green, c_red]
    tols = [0.02, 1e-10]

    record_index = [0, m - 1]
    taus = [0.25, 0.5, 0.75]

    i_tau_c_tol = list(itertools.product(record_index, taus, zip(cs, tols)))
    i_tau_c = [(i, tau, c) for (i, tau, (c, tol)) in i_tau_c_tol]
    tols = [tol for (_, _, (_, tol)) in i_tau_c_tol]

    t_vector = h.Vector().record(h._ref_t)

    for tau in taus:
        h.soma(tau).tau_green = tau

    def record(i, tau, c):
        if c == c_red:
            return h.Vector().record(h.soma(tau)._ref_upsilon_red[i])
        elif c == c_green:
            return h.Vector().record(h.soma(tau)._ref_upsilon_green[i])

    upsilon_vector = [record(i, tau, c) for (i, tau, c) in i_tau_c]

    if enable_coreneuron:
        pc = h.ParallelContext()

        # set gid for the cell (necessary for coreneuron data transfer)
        pc.set_gid2node(pc.id() + 1, pc.id())
        myobj = h.NetCon(h.soma(0.5)._ref_v, None, sec=h.soma)
        pc.cell(pc.id() + 1, myobj)

        with coreneuron(enable=enable_coreneuron, file_mode=file_mode):
            h.stdinit()

            if run_mode == 0:
                pc.psolve(h.tstop)
            elif run_mode == 1:
                while h.t < h.tstop:
                    pc.psolve(h.t + 1.0)
            else:
                while h.t < h.tstop:
                    h.continuerun(h.t + 0.5)
                    pc.psolve(h.t + 0.5)

    else:
        h.stdinit()
        h.run()

    t = np.array(t_vector.as_numpy())

    assert t.size > 0, "No time instances recorded."

    upsilon_approx = [np.array(u.as_numpy()) for u in upsilon_vector]
    upsilon_exact = [upsilon(i, tau, c, t) for (i, tau, c) in i_tau_c]

    if create_plots:
        # Only import `matplotlib` if needed, it's not available
        # during CI.
        import matplotlib.pyplot as plt

        for k, u in enumerate(upsilon_exact):
            legend_kwargs = {"label": "exact"} if k == 0 else dict()
            plt.plot(t, u, "-k", linewidth=2, **legend_kwargs)

        for u in upsilon_approx:
            plt.plot(t, u, "--", linewidth=2)

        plt.xlabel("time")
        plt.ylabel("upsilon")
        plt.title(f"Mode {run_mode}, File mode = {file_mode}, GPU = {enable_gpu}")
        plt.savefig(
            f"upsilon_traces-mode{run_mode}_filemode{file_mode}_gpu{enable_gpu}.png",
            dpi=300,
        )

    for k, (tol, u_exact, u_approx) in enumerate(
        zip(tols, upsilon_exact, upsilon_approx)
    ):
        abs_err_inf = np.max(np.abs(u_approx - u_exact))
        rel_err_inf = abs_err_inf / np.max(np.abs(u_exact))
        assert rel_err_inf < tol, f"{k}: {rel_err_inf} < {tol} failed."


if __name__ == "__main__":
    test_array_variable_transfer()
