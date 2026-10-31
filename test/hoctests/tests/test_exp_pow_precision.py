# With -DNRN_ENABLE_ARCH_INDEP_EXP_POW=ON, use_exp_pow_precision(istyle) must
# govern the exp computed inside NEURON as well as exp in mod files: hoc's exp(),
# the rate functions of KSChan (ChannelBuilder) channels and fsyn's alpha
# function (synapse.cpp). Each is compared with AlphaSynapse's FUNCTION alpha(x),
# x*exp(1 - x) from a mod file. istyle 2 rounds exp to 32 bits of mantissa,
# which the C library's exp does not, so an exp that bypasses the option fails
# here on every platform. Without the option there is nothing to check.

from neuron import h

# exp(1 - 0.85625) = exp(0.14375000000000004) is one ulp off in Apple's libm,
# exp(1 - 0.2598) one ulp off in glibc's.
xs = [0.85625, 0.2598, 3.7]


def test_exp_pow_precision():
    if h.use_exp_pow_precision(1) != 1:
        print("built without NRN_ENABLE_ARCH_INDEP_EXP_POW: nothing to check")
        return

    soma = h.Section(name="soma")
    syn = h.AlphaSynapse(soma(0.5))  # its alpha(x) is exp from a mod file

    ks = h.KSChan()
    ks.name("kexp")
    ks.add_hhstate("n")
    ks.trans(0).set_f(0, 2, h.Vector([1, 1, 0]))  # f(v) = 1*exp(1*(v - 0))

    gmax = 0.01
    h.fsyn(1)
    h.fsyn(0, 0.5, 0, 1, gmax, 0, sec=soma)  # onset 0, tau 1: alpha(t)
    gseg = 1e2 * gmax / h.area(0.5, sec=soma)

    dt = h.dt
    try:
        for style in [0, 1, 2]:
            h.use_exp_pow_precision(style)
            for x in xs:
                ref = syn.alpha(x)
                assert x * h.exp(1 - x) == ref, (style, x, "hoc exp")
                assert x * ks.trans(0).f(0, 1 - x) == ref, (style, x, "KSChan")
                h.dt = x  # fsyn uses the thread's t: one step to t = x
                h.finitialize(-65)
                h.fadvance()
                assert h.t == x
                assert h.fsyng(0) == gseg * ref * (gmax / gseg), (style, x, "fsyn")
    finally:
        h.use_exp_pow_precision(0)
        h.fsyn(0)
        h.dt = dt


if __name__ == "__main__":
    test_exp_pow_precision()
