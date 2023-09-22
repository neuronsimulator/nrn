# SaveState no longer internally uses NrnProperty so can
# be restored from an FInitializeHandler
# after the model is locked in h.finitialize()
# See https://github.com/neuronsimulator/nrn/issues/2517

from neuron import h

filename = "save_test.dat"


def restore():
    svst = h.SaveState()
    f = h.File(filename)
    svst.fread(f)
    svst.restore()


sec = h.Section(name="dend")
h.finitialize(-70)
assert sec(0.5).v == -70.0
# save
svst = h.SaveState()
svst.save()
f = h.File(filename)
svst.fwrite(f)

# restore
fih = h.FInitializeHandler(1, restore)
h.finitialize(-60)
assert sec(0.5).v == -70.0
