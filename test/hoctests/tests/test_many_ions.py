# Test when large number of ion mechanisms exist.

# Strategy is to create a large number of ion mechanisms before loading
# the neurondemo mod file shared library which will create the ca ion
# along with several mechanisms that write to cai.

from neuron import h
from platform import machine
import sys


# return True if name exists in h
def exists(name):
    try:
        exec("h.%s" % name)
    except:
        return False
    return True


# use up a large number of mechanism indices for ions. And want to test beyond
# 1000 which requires change to max_ions in nrn/src/nrnoc/eion.cpp
nion = 250
ion_indices = [int(h.ion_register("ca%d" % i, 2)) for i in range(1, nion + 1)]
# gain some confidence they exist
assert exists("ca%d_ion" % nion)
assert (ion_indices[-1] - ion_indices[0]) == (nion - 1)
mt = h.MechanismType(0)
assert mt.count() > nion

# this test depends on the ca ion not existing at this point
assert exists("ca_ion") is False

# load neurondemo mod files. That will create ca_ion and provide two
# mod files that write cai
nrnmechlib = (
    "nrnmech.dll" if sys.platform == "win32" else "%s/libnrnmech.so" % machine()
)
# Following Aborts prior to PR#3055 with
# eion.cpp:431: void nrn_check_conc_write(Prop*, Prop*, int): Assertion `k < sizeof(long) * 8' failed.
nrnmechlibname = "%s/demo/release/%s" % (h.neuronhome(), nrnmechlib)
print(nrnmechlibname)
assert h.nrn_load_dll(nrnmechlibname)

# ca_ion now exists and has a mechanism index > nion
assert exists("ca_ion")
mt = h.MechanismType(0)  # new mechanisms do not appear in old MechanismType
mt.select("ca_ion")
assert mt.selected() > nion

# test if can use the high mechanism index cadifpmp (with USEION ... WRITE cai) along with the high mechanims index ca_ion
s = h.Section()
s.insert("cadifpmp")
h.finitialize(-65)
