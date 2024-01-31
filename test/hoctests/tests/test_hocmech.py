from neuron import h, gui
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
        public V

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
h.make_mechanism("max", "Max")

# declare a POINT_PROCESS using HOC
h(
    """
    begintemplate PMax
        public V

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
h.make_pointprocess("PMax")

soma.insert("max")
pm = h.PMax(0.5, sec=soma)
pm = h.PMax(soma(0.5))

h.run()

print("V_max = %g" % soma(0.5).V_max)
print("max.V = %g" % soma(0.5).max.V)
print("pm.V = %g" % pm.V)
