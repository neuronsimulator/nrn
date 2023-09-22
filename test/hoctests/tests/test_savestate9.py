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


def init(sec, v):
    for i, seg in enumerate(sec):
        seg.v = v + i
    h.finitialize()


def vars(sec):
    return [(seg.v, seg.m_hh, seg.h_hh, seg.n_hh) for seg in sec]


class Cell:  # needed for BBSaveState which is cell based.
    def __init__(self):
        sec = h.Section(name="dend", cell=self)
        sec.insert("hh")
        sec.nseg = 3
        self.sec = sec


cell = Cell()
sec = cell.sec

init(sec, -70)
savvars = vars(sec)

# save
svst = h.SaveState()
svst.save()
f = h.File(filename)
svst.fwrite(f)

# restore
fih = h.FInitializeHandler(1, restore)
h.finitialize(-60)
assert vars(sec) == savvars

# similar for BBSaveState

# clean up previous and prepare
fih = None
import subprocess

subprocess.run("rm -f " + filename, shell=True)
subprocess.run("rm -r -f bbss_out", shell=True)
subprocess.run("rm -r -f in", shell=True)

out2in_sh = r"""
#!/usr/bin/env bash
out=bbss_out
rm -f in/*
mkdir -p in
cat $out/tmp > in/tmp
for f in $out/tmp.*.* ; do
  i=`echo "$f" | sed 's/.*tmp\.\([0-9]*\)\..*/\1/'`
  if test ! -f in/tmp.$i ; then
    cnt=`ls $out/tmp.$i.* | wc -l`
    echo $cnt > in/tmp.$i
    cat $out/tmp.$i.* >> in/tmp.$i
  fi
done
"""

pc = h.ParallelContext()
pc.set_gid2node(1, pc.id())
pc.cell(1, h.NetCon(sec(0.5)._ref_v, None))


def cp_out_to_in():
    if pc.id() == 0:
        import tempfile

        with tempfile.NamedTemporaryFile("w") as scriptfile:
            scriptfile.write(out2in_sh)
            scriptfile.flush()
            subprocess.check_call(["/bin/bash", scriptfile.name])

    pc.barrier()


def restore():
    cp_out_to_in()
    svst = h.BBSaveState()
    svst.restore_test()


h.finitialize(-60)
assert vars(sec) != savvars
init(sec, -70)
assert vars(sec) == savvars

# save
svst = h.BBSaveState()
svst.save_test()
del svst

# restore
fih = h.FInitializeHandler(1, restore)
h.finitialize(-60)

# because of textual decimal storage of doubles in an ascii file,
# the binary mantissa can differ in the 14 or 15th decimal place.


def cmp_equal(a, b, res):
    for i, a1 in enumerate(a):
        for j, a2 in enumerate(a1):
            b2 = b[i][j]
            if abs(a2 - b2) / abs(a2 + b2) > res:
                return False
    return True


assert cmp_equal(vars(sec), savvars, 1e-15)
