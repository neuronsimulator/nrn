# cvode and ida should interpolate correctly
# Consider a simulation with linearly increasing solution (charging capacitor)
# If during a free running large time step, one requests the value of
# the voltage in the middle of that time step, the voltage should be
# the average of the voltages at the beginning and end of the time step.
# Prior to this PR, IDA failed this test

from neuron import h

h.load_file("stdrun.hoc")
import math


def model():
    s = h.Section(name="s")
    s.L = s.diam = h.sqrt(100.0 / h.PI)
    ic = h.IClamp(s(0.5))
    ic.dur = 10
    ic.amp = 0.01
    return s, ic


def run(ida):
    h.cvode_active(1)
    h.cvode.use_daspk(ida)
    h.v_init = 0.0
    h.run()


def points(vecs):
    return [x for x in zip(vecs[0], vecs[1])]


def test():
    m = model()
    vref = m[0](0.5)._ref_v
    freerun = [h.Vector().record(h._ref_t), h.Vector().record(vref)]
    for ida in [0, 1]:
        run(ida)
        n = len(freerun[0])
        std = [v.c() for v in freerun]
        pts = points(freerun)[-3:-1]
        midpnt = [(pts[0][i] + pts[1][i]) / 2 for i in range(2)]
        trec = h.Vector([midpnt[0]])
        rec = [h.Vector().record(h._ref_t, trec), h.Vector().record(vref, trec)]
        run(ida)
        print(midpnt)
        print(points(rec))

        assert len(freerun[0]) == n  # rec does not add to original freerun
        for i, s in enumerate(std):  # and rec did not affect freerun.
            for j, v in enumerate(s):
                assert math.isclose(v, freerun[i][j])

        for i in range(2):
            assert math.isclose(midpnt[i], rec[i][0])


if __name__ == "__main__":
    test()
