# Test when large number of ion mechanisms exist.

# Strategy is to create a large number of ion mechanisms before loading
# the neurondemo mod file shared library which will create the ca ion
# along with several mechanisms that write to cai.

from neuron import h, load_mechanisms
from platform import machine
import sys
import io


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
# Following Aborts prior to PR#3055 with
# eion.cpp:431: void nrn_check_conc_write(Prop*, Prop*, int): Assertion `k < sizeof(long) * 8' failed.
nrnmechlibpath = "%s/demo/release" % h.neuronhome()
print(nrnmechlibpath)
quit()
assert load_mechanisms(nrnmechlibpath)

# ca_ion now exists and has a mechanism index > nion
assert exists("ca_ion")
mt = h.MechanismType(0)  # new mechanisms do not appear in old MechanismType
mt.select("ca_ion")
assert mt.selected() > nion


# return stderr output resulting from sec.insert(mechname)
def mech_insert(sec, mechname):
    # capture stderr
    old_stderr = sys.stderr
    sys.stderr = mystderr = io.StringIO()

    sec.insert(mechname)

    sys.stderr = old_stderr
    return mystderr.getvalue()


# test if can use the high mechanism index cadifpmp (with USEION ... WRITE cai) along with the high mechanims index ca_ion
s = h.Section()
s.insert("cadifpmp")
h.finitialize(-65)
# The ion_style tells whether cai or cao is being written
istyle = int(h.ion_style("ca_ion", sec=s))
assert istyle & 0o200  # writing cai
assert (istyle & 0o400) == 0  # not writing ca0
# unfortunately, at present, uninserting does not turn off that bit
s.uninsert("cadifpmp")
assert s.has_membrane("cadifpmp") is False
assert int(h.ion_style("ca_ion", sec=s)) & 0o200
# nevertheless, on reinsertion, there is no warning, so can't test the
# warning without a second mechanism that WRITE cai
assert mech_insert(s, "cadifpmp") == ""
assert s.has_membrane("cadifpmp")
# uninsert again and insert another mechanism that writes.
s.uninsert("cadifpmp")
assert mech_insert(s, "capmpr") == ""  # no warning
assert mech_insert(s, "cadifpmp") != ""  # now there is a warning.

# more serious test
# several sections with alternating cadifpmp and capmpr
del s
secs = [h.Section() for i in range(5)]
for i, s in enumerate(secs):
    if i % 2:
        assert mech_insert(s, "capmpr") == ""
    else:
        assert mech_insert(s, "cadifpmp") == ""

# warnings due to multiple mechanisms at same location that WRITE cai
assert mech_insert(secs[2], "capmpr") != ""
assert mech_insert(secs[3], "cadifpmp") != ""
