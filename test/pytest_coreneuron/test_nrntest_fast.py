"""
Tests that used to live in the fast/ subdirectory of the
https://github.com/neuronsimulator/nrntest repository
"""
import math
import numpy as np
import os
import pytest
from neuron import h
from neuron.tests.utils import (
    cvode_enabled,
    cvode_use_global_timestep,
    cvode_use_long_double,
    hh_table_disabled,
    num_threads,
    parallel_context,
)
from neuron.tests.utils.checkresult import Chk

h.load_file("stdrun.hoc")


@pytest.fixture(scope="module")
def chk():
    """Manage access to JSON reference data."""
    dir_path = os.path.dirname(os.path.realpath(__file__))
    checker = Chk(os.path.join(dir_path, "test_nrntest_fast.json"))
    yield checker
    # Save results to disk if they've changed; this is called after all tests
    # using chk have executed
    checker.save()


class Cell:
    """Cell class used for test_t13 and augmented for test_t14."""

    def __init__(self, id):
        self.id = id
        self.soma = h.Section(name="soma", cell=self)
        self.soma.L = 10
        self.soma.diam = 100 / (math.pi * self.soma.L)
        self.soma.insert("hh")
        self.s = h.IClamp(self.soma(0.5))
        self.s.dur = 0.1 + 0.1 * id
        self.s.amp = 0.2 + 0.1 * id
        self.vv = h.Vector()
        self.vv.record(self.soma(0.5)._ref_v)
        self.tv = h.Vector()
        self.tv.record(h._ref_t)

    def data(self):
        return {"t": list(self.tv), "v": list(self.vv)}

    def __str__(self):
        return "Cell[{:d}]".format(self.id)


# Run t13 and t13 with 1 thread and with 3 threads if MPI is disabled. With
# MPI enabled then multithreading is not supported, so just use 1 thread.
pc = h.ParallelContext()
this_rank, num_ranks = pc.id(), pc.nhost()
thread_values = [1] if num_ranks > 1 else [1, 3]
t13_methods = ["fixed", "cvode", "cvode_long_double"]


@pytest.fixture(scope="module", params=t13_methods)
def t13_model_data(request):
    """Run the simulation for the t13 tests and return the recorded data.

    Putting this in a separate fixture achieves two main things:

    - It allows us to explicitly clean up the model (`del cells`) before doing
      any result comparisons (which may throw). This helps ensure that we
      leave the global NEURON state as we found it, and that we do not trigger
      failures in later tests.
    - It allows us to put the assertions for different fields (time, voltage)
      and thread counts into different tests, which means that a single test
      execution reports results for fixed/cvode, 1/3 threads and time/voltage
      separately.

    Note: cvode_use_long_double (which is a slight misnomer now; it actually
    uses compensated/Kahan summation for portability reasons) aims to reduce
    the differences between 1- and 3-thread operation coming from N_VWrmsNorm.
    """
    method = request.param
    data = {"method": method}
    for threads in thread_values:
        cells = [Cell(i) for i in range(3) if i % num_ranks == this_rank]
        # cvode_use_global_timestep takes care of enabling global
        # synchronisation of the variable timestep method across ranks
        # even though the cells have no connectivity
        with hh_table_disabled(), parallel_context() as pc, num_threads(
            pc, threads=threads
        ), cvode_enabled(method.startswith("cvode")) as cv, cvode_use_global_timestep(
            cv, True
        ), cvode_use_long_double(
            cv, method == "cvode_long_double"
        ):
            h.run()
            data[threads] = {str(cell): cell.data() for cell in cells}
        del cells
    return data


def compare_time_and_voltage_trajectories(
    chk, model_data, field, threads, name, tolerance
):
    """Compare time and voltage trajectories for several cells.

    Arguments:
      chk: handle to JSON reference data loaded from disk
      model_data: the data to compare, this is a nested dict with structure
                  model_data[thread_count][cell_name][field] = list(values)
      field: which field (time or voltage) to compare
      threads: which thread count to compare
      name: name of the test (t13 or t14, for now), used to access chk
      tolerance: relative tolerance for the fuzzy comparison between values

    This is used to implement both test_t13 and test_t14. Different fields and
    numbers of threads are compared somewhat differently, as follows:

    - if threads == 1, the reference data are loaded from JSON via chk. If no
      data are loaded, no comparison is done, but the data from the current
      run are saved to disk (for use as a future reference run). This means
      that the comparisons with threads == 1 are sensitive to differences
      between different compilers, architectures, optimisation settings etc.
    - for threads != 1, the results with threads == 1 are used as a reference.
      This means that these comparisons are sensitive to differences in
      summation order and rounding due to different orderings of floating
      point operations etc.

    Furthermore, there is special handling for the dependent variable (v), to
    reduce the need for generous tolerances. Because, when cvode is used,
    there are small differences in the time values between the reference data
    and the new data, it is expected that (particularly when the voltage is
    changing rapidly) this will generate differences in the voltage values. To
    mitigate this, the new voltage values are interpolated to match the time
    values from the reference data before comparison."""
    method = model_data["method"]  # cvode or fixed

    # Determine which data we will use as a reference
    if threads == 1:
        # threads=1: compare to reference from JSON file on disk
        key = name + ":"
        if method.startswith("cvode"):
            key += "cvode"
        else:
            key += method
        ref_data = chk.get(key, None)
        if ref_data is None:
            # No comparison to be done; store the data as a new reference
            chk(key, model_data[threads])
            return
    else:
        # threads>1: compare to threads=1 from this test execution
        ref_data = model_data[1]

    # Compare `field` in `this_data` with `ref_data` and `tolerance`
    this_data = model_data[threads]

    if field == "v":
        # If the t values don't match then it is expected that the v values
        # won't either, particularly when the voltage is changing rapidly.
        # Interpolate the new v values to match the reference t values to
        # mitigate this.
        def interp(new_t, old_t, old_v):
            assert np.all(np.diff(old_t) > 0)
            return np.interp(new_t, old_t, old_v)

        new_data = {}
        for name, data in this_data.items():
            ref_t = ref_data[name]["t"]
            raw_t, raw_v = data["t"], data["v"]
            assert len(raw_t) == len(ref_t)
            assert len(raw_v) == len(ref_t)
            new_v = interp(ref_t, raw_t, raw_v)
            new_data[name] = {"v": new_v}
        this_data = new_data

    # Finally ready to compare
    assert this_data.keys() <= ref_data.keys()
    max_diff = 0.0
    for name in this_data:  # cell name
        # Pick out the field we're comparing
        these_vals = this_data[name][field]
        ref_vals = ref_data[name][field]
        assert len(these_vals) == len(ref_vals)
        for a, b in zip(these_vals, ref_vals):
            match = math.isclose(a, b, rel_tol=tolerance)
            if match:
                continue
            diff = abs(a - b) / max(abs(a), abs(b))
            max_diff = max(diff, max_diff)
    if max_diff > tolerance:
        raise Exception("max diff {} > {}".format(max_diff, tolerance))


# Whether to test time or voltage values
@pytest.mark.parametrize("field", ["t", "v"])
@pytest.mark.parametrize("threads", thread_values)
def test_t13(chk, t13_model_data, field, threads):
    """hh model, testing fixed step and cvode with threads.

    This used to be t13.hoc in nrntest/fast.

    See t13_model_data for the actual model and see
    compare_time_and_voltage_trajectories for explanation of how the results
    are validated, including why the thresholds for 1 and 3 threads below are
    probing different things."""

    method = t13_model_data["method"]  # cvode or fixed
    # Determine the relative tolerance we can accept
    tolerance = 0.0
    if method == "fixed" and field == "v" and threads == 1:
        tolerance = 1e-10
    elif method.startswith("cvode"):
        if field == "t":
            tolerance = 5e-8
        elif field == "v":
            tolerance = 6e-7

    compare_time_and_voltage_trajectories(
        chk, t13_model_data, field, threads, "t13", tolerance
    )


class CellWithGapJunction(Cell):
    """Cell class used for test_t14."""

    def __init__(self, id):
        super().__init__(id)
        self.gap = h.HalfGap(self.soma(0.5))
        self.gap.r = 200
        if id == 0:
            self.s.dur = 0.1
            self.s.amp = 0.3
        elif id == 1:
            self.s.dur = 0.0
            self.s.amp = 0.0


@pytest.fixture(scope="module", params=["fixed", "cvode", "cvode_long_double"])
def t14_model_data(request):
    """Run the simulation for the t14 tests and return the recorded data.

    See t13_model_data above for more rationale about why this is organised
    this way."""
    method = request.param
    data = {"method": method}
    n_cells = 2
    cells = {
        i: CellWithGapJunction(i) for i in range(n_cells) if i % num_ranks == this_rank
    }
    for threads in thread_values:
        with hh_table_disabled(), parallel_context() as pc, num_threads(
            pc, threads=threads
        ), cvode_enabled(method.startswith("cvode")) as cv, cvode_use_long_double(
            cv, method == "cvode_long_double"
        ):
            for i in range(n_cells):
                next_i = (i + 1) % n_cells
                if i in cells:
                    pc.source_var(cells[i].soma(0.5)._ref_v, i, sec=cells[i].soma)
                if next_i in cells:
                    pc.target_var(cells[next_i].gap, cells[next_i].gap._ref_vgap, i)

            pc.setup_transfer()
            h.run()
            data[threads] = {str(cell): cell.data() for cell in cells.values()}
    del cells
    return data


@pytest.mark.parametrize("field", ["t", "v"])
@pytest.mark.parametrize("threads", thread_values)
def test_t14(chk, t14_model_data, field, threads):
    """Gap junction with setup_transfer. Originally t14.hoc in nrntest/fast.

    See t14_model_data for the actual model and see
    compare_time_and_voltage_trajectories for explanation of how the results
    are validated, including why the thresholds for 1 and 3 threads below are
    probing different things."""
    tolerance = 0.0
    method = t14_model_data["method"]  # cvode or fixed
    if method == "fixed" and field == "v" and threads == 1:
        tolerance = 2e-9
    elif method.startswith("cvode"):
        if field == "t":
            if threads == 1:
                tolerance = 8e-10
            else:
                if "long_double" in method:
                    tolerance = 2e-10
                else:
                    tolerance = 8e-10
        elif field == "v":
            if threads == 1:
                tolerance = 2e-9
            else:
                if "long_double" in method:
                    tolerance = 4e-10
                else:
                    tolerance = 2e-9

    compare_time_and_voltage_trajectories(
        chk, t14_model_data, field, threads, "t14", tolerance
    )


if __name__ == "__main__":
    # python test_nrntest_fast.py will run all the tests in this file
    # e.g. __file__ --> __file__ + "::test_t13" would just run test_t13
    pytest.main([__file__])
