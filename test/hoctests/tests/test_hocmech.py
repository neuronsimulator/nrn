from neuron import h, gui
from neuron.expect_hocerr import expect_err, set_quiet

set_quiet(False)
import math

soma = h.Section(name="soma")
soma.L = soma.diam = math.sqrt(100 / math.pi)
soma.insert("hh")  # equivalently: soma.insert(h.hh)

stim = h.IClamp(soma(0.5))
stim.dur = 0.1
stim.amp = 0.3

# declare a density mechanism using HOC
h(
    """
    begintemplate Max
        public V, a, b
        a = 0
        double b[2]

        proc initial() {
            V = v($1)
        }

        proc after_step() {
            if (V < v($1)) {
                V = v($1)
            }
        }
    endtemplate Max
"""
)
h.make_mechanism("max", "Max", "a b")

# declare a POINT_PROCESS using HOC
h(
    """
    begintemplate PMax
        public V, c, d
        c = 0
        double d[2]

        proc initial() {
            V = v($1)
        }

        proc after_step() {
            if (V < v($1)) {
                V = v($1)
            }
        }
    endtemplate PMax
"""
)
pm = h.PMax()
expect_err('h.make_pointprocess("PMax", "c d")')
del pm
h.make_pointprocess("PMax", "c d")

soma.insert("max")
pm = h.PMax(0.5, sec=soma)
pm = h.PMax(soma(0.5))

h.run()

print("V_max = %g" % soma(0.5).V_max)
print("max.V = %g" % soma(0.5).max.V)
print("pm.V = %g" % pm.V)

expect_err('h.make_mechanism("max", "Max")')
assert pm.has_loc() == True
expect_err("pm.loc()")
pm.loc(soma(1.0))
print(pm.get_loc(), h.secname())

mt = h.MechanismType(1)
mt.select("PMax")
pmaxref = h.ref(None)
mt.make(pmaxref)  # eventually calls hoc_new_opoint
print(pmaxref[0])
expect_err("pmaxref[0].get_loc()")
del mt, pmaxref

expect_err('h.make_mechanism("xmax", "Max", "a 3")')
