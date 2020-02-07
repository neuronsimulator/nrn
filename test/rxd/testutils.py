import inspect
import itertools
import os

import numpy


tol = 1e-10
dt_eps = 1e-20


def get_correct_data_for_test():
    """returns a path to the file with the correct data for a test."""

    curframe = inspect.currentframe()
    calframe = inspect.getouterframes(curframe, 3)
    testfunc_name = calframe[2][3]
    assert testfunc_name.startswith('test_')
    data_filename = testfunc_name[5:] + '.dat'
    basepath = os.path.dirname(os.path.abspath(__file__))
    return os.path.join(basepath, 'correct_data', data_filename)


def collect_data(h, rxd, data, num_record=10):
    """grabs the membrane potential data, h.t, and the rxd state values"""

    data['record_count'] += 1
    if data['record_count'] > num_record:
        h.stoprun = True
        return
    all_potentials = [seg.v for seg in itertools.chain.from_iterable(h.allsec())]
    rxd_1d = list(rxd.node._states)
    rxd_3d = []
    rxd_ecs = []
    for sp in rxd.species._all_species:
        s = sp()
        if s and hasattr(s, '_intracellular_instances'):
            for ics in s._intracellular_instances.values():
                rxd_3d += list(ics.states)
        if s and hasattr(s, '_extracellular_instances'):
            for ecs in s._extracellular_instances.values():
                rxd_ecs += list(ecs.states.flatten())
    all_rxd = rxd_1d + rxd_3d + rxd_ecs
    local_data = [h.t] + all_potentials + all_rxd

    # remove data before t=0
    if h.t == 0:
        data['data'] = []
        data['record_count'] = 1
    # remove previous record if h.t is the same
    if data['record_count'] > 1 and h.t == data['data'][-len(local_data)]:
        data['record_count'] -= 1
        del data['data'][-len(local_data) :]
    # add new data record
    data['data'].extend(local_data)
    if data['record_count'] == 2:
        data['rlen'] = len(local_data)


def compare_data(data):
    """compares the test data with the correct data"""

    rlen = data['rlen']
    corr_dat = numpy.fromfile(get_correct_data_for_test()).reshape(-1, rlen)
    tst_dat = numpy.array(data['data']).reshape(-1, rlen)
    t1 = corr_dat[:, 0]
    t2 = tst_dat[:, 0]
    # remove any initial t that are greter than the next t
    # (removes times before 0) in correct data
    c = 0
    while c < len(t1) - 1 and t1[c] > t1[c + 1]:
        c = c + 1
    t1 = numpy.delete(t1, range(c))
    corr_dat = numpy.delete(corr_dat, range(c), 0)
    # remove any initial t that are greter than the next t
    # (removes times before 0) in test data
    c = 0
    while c < len(t2) - 1 and t2[c] > t2[c + 1]:
        c = c + 1
    t2 = numpy.delete(t2, range(c))
    tst_dat = numpy.delete(tst_dat, range(c), 0)
    # get rid of repeating t in correct data (otherwise interpolation fails)
    c = 0
    while c < len(t1) - 1:
        c1 = c + 1
        while c1 < len(t1) and abs(t1[c] - t1[c1]) < dt_eps:
            c1 = c1 + 1
        t1 = numpy.delete(t1, range(c, c1 - 1))
        corr_dat = numpy.delete(corr_dat, range(c, c1 - 1), 0)
        c = c + 1
    # get rid of the test data outside of the correct data time interval
    t2_n = len(t2)
    t2_0 = 0
    while t2[t2_n - 1] > t1[-1]:
        t2_n = t2_n - 1
    while t2[t2_0] < t1[0]:
        t2_0 = t2_0 + 1
    # interpolate and compare
    corr_vals = numpy.array(
        [numpy.interp(t2[t2_0:t2_n], t1, corr_dat[:, i].T) for i in range(1, rlen)]
    )
    max_err = numpy.amax(abs(corr_vals.T - tst_dat[t2_0:t2_n, 1:]))
    return max_err
