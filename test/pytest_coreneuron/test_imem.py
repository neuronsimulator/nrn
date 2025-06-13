# i_membrane + electrode current sums to 0 when electrode parameters are time dependent.
# This test revealed a fast_imem bug that needed fixing #2733

from neuron.tests.utils.coreneuron_available import coreneuron_available

from neuron import h, gui, config
from neuron import coreneuron
import math

pc = h.ParallelContext()


def chkvec(v, str, tol=1e-10):
    x = v.c().abs().max()
    # print(str, " ", x)
    assert math.isclose(x, 0.0, abs_tol=tol)


def seclamp(s):
    stim = h.SEClamp(s(0.5))
    stim.amp1 = -65
    stim.dur1 = 1
    stim.amp2 = -10
    stim.dur2 = 100
    stim.rs = 10
    return stim


def iclamp(s):
    stim = h.IClamp(s(0.5))
    stim.delay = 1.0
    stim.dur = 0.1
    stim.amp = 0.3
    return stim


def triangle_stim(stimref, step, min, max):
    tstim = h.Vector(20).indgen(step)
    istim = tstim.c().fill(min)
    for i in range(1, len(istim), 2):
        istim[i] = max
    istim.play(stimref, tstim, 1)
    return tstim, istim


def tst(xtrcell, secondorder, cvactive, corenrn, seclmp):
    # print(xtrcell, secondorder, cvactive, corenrn, seclmp)
    cv = h.cvode
    cv.use_fast_imem(1)
    h.secondorder = secondorder
    cv.active(cvactive)

    s = h.Section("s")
    s.L = 10
    s.diam = 10
    s.nseg = 1
    s.insert("hh")

    imvec = None
    if xtrcell:
        s.insert("extracellular")
        imvec = h.Vector().record(s(0.5)._ref_i_membrane)

    vstim = None
    rstim = None
    if seclmp:
        stim = seclamp(s)
        stim.dur1 = 100
        vstim = triangle_stim(stim._ref_amp1, 0.5, -65, -10)
        rstim = triangle_stim(stim._ref_rs, 1, 1000, 100)
    else:
        stim = iclamp(s)
        stim.dur = 100
        vstim = triangle_stim(stim._ref_amp, 0.5, 0, 0.001)

    tvec = h.Vector().record(h._ref_t)
    vvec = h.Vector().record(s(0.5)._ref_v)
    istimvec = h.Vector().record(stim._ref_i)
    imemvec = h.Vector().record(s(0.5)._ref_i_membrane_)
    if hasattr(stim, "vc"):
        vstimvec = h.Vector().record(stim._ref_vc)
        rsstimvec = h.Vector().record(stim._ref_rs)

    glist = []

    def fstim():
        if h.cvode.active():
            return istimvec.c()
        # current balance and imembrane calculation takes place at t + .5*dt
        # SEClamp conductance is evaluated (Vector.play) at t + .5*dt
        # But recording is at time t + dt.
        # So use rsstimvec to evaluate clamp resistance at midpoint
        # of each step. That is the resistance for the entire step.
        rs = rsstimvec.c().add(rsstimvec.c().rotate(1)).mul(0.5)
        rs[0] = rsstimvec[0]
        vc = vstimvec.c()
        v = vvec.c()
        if h.secondorder:
            v = vvec.c().add(vvec.c().rotate(1)).mul(0.5)
            v[0] = vvec[0]
        s = vc.sub(v).div(rs)
        return s

    def plt(vec, vecstr):
        return  # comment out if want to see plots
        g = h.Graph()
        glist.append(g)
        vec.label(vecstr)
        vec.line(g, tvec)
        g.exec_menu("View = plot")
        g.exec_menu("10% Zoom out")
        return g

    # run
    # h.steps_per_ms = 400
    h.tstop = 10
    pc.set_maxstep(10)
    h.finitialize(-65)
    if corenrn:
        coreneuron.enable = True
        coreneuron.verbose = 0
        pc.psolve(h.tstop)
        coreneuron.enable = False
        coreneuron.verbose = 2
    else:
        h.run()
    plt(vvec, "v")
    plt(imemvec, "i_membrane_")
    plt(istimvec, "stim.i")
    tmp = (imemvec.c().sub(istimvec), "i_membrane_ - stim_i")
    plt(*tmp)
    if cv.active() or not seclmp:
        chkvec(*tmp)
    if xtrcell:
        pass
        plt(imvec, "i_membrane")
        plt(imvec.c().mul(s(0.5).area() / 100).sub(istimvec), "i_membrane - stim_i")
        tmp = (
            imvec.c().mul(s(0.5).area() / 100).sub(imemvec),
            "i_membrane - i_membrane_",
        )
        plt(*tmp)
        chkvec(*tmp)
    if hasattr(stim, "vc"):
        pass
        g = plt(rsstimvec, "stim.rs")
        if g:
            rstim[1].line(g, rstim[0], 2, 1)
        plt(vstimvec, "stim.vc")
        tmp = (imemvec.c().sub(fstim()), "i_membrane_ - fstim()")
        plt(*tmp)
        chkvec(*tmp)

    cv.use_fast_imem(0)
    h.secondorder = 0
    return glist


def test_imem():
    # tst(xtrcell, secondorder, cvactive, corenrn, seclmp)
    cnlist = [0]
    if coreneuron_available():
        cnlist.append(1)
    for seclmp in [0, 1]:
        tst(1, 0, 0, 0, seclmp)
        tst(0, 0, 1, 0, seclmp)
        for secondorder in [0, 2]:
            for corenrn in cnlist:
                tst(0, secondorder, 0, corenrn, seclmp)


if __name__ == "__main__":
    test_imem()
