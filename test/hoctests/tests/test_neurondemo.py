import os
from neuron import config

# skip test if no InterViews GUI
if not config.arguments["NRN_ENABLE_INTERVIEWS"] or os.getenv("DISPLAY") is None:
    print("No GUI for running neurondemo. Skip this test.")
    quit()

from neuron.tests.utils.checkresult import Chk
from subprocess import Popen, PIPE

# Create a helper for managing reference results
dir_path = os.path.dirname(os.path.realpath(__file__))
chk = Chk(os.path.join(dir_path, "test_neurondemo.json"))


# Run a command with input into stdin
def run(cmd, input):
    p = Popen(cmd, shell=True, stdin=PIPE, stdout=PIPE, stderr=PIPE, text=True)
    return p.communicate(input=input)


# Attempting to run multiple demos under stdin control with one launch
# of neurondemo with address sanitizer reveals a long-standing issue with
# regard to accessing freed memory when destroying the gui of the previous
# demo. This generally happens when switching away from demos that use
# a demo specific ```destroy()``` function that closes windows with
# the ```PWManager[0].close(i)``` method.
# This generally does not occur during circumstances where one
# mouses over the demos using the selection panel.
# For this reason we run neurondemo separately while selecting only a
# single demo. It would be nice if we could run neurondemo once and
# cycle several times through each demo. Though that works manually for
# version 8.2, With the current version of the master one can
# cycle through once but not twice with the mouse.

# HOC: select demo, run, and print all lines on all Graphs
input = r"""
proc dodemo() {
  usetable_hh = 0 // Compatible with -DNRN_ENABLE_CORENEURON=ON
  demo(%d)
  run()
  printf("\nZZZbegin\n")
  prgraphs()
  printf("ZZZend\n")
}
dodemo()
quit()
"""


# run neurondemo and return the stdout lines between ZZZbegin and ZZZend
def neurondemo(extra, input):
    input = extra + input
    stdout, stderr = run("neurondemo -nobanner", input)
    stdout = stdout.splitlines()
    stderr = stderr.splitlines()
    if len(stderr) != 0:
        for line in stderr:
            print("stderr:", line.strip())
        raise Exception("Unexpected stderr")
    begin_index = stdout.index("ZZZbegin")
    end_index = stdout.index("ZZZend")
    prefix = stdout[:begin_index]
    suffix = stdout[end_index + 1 :]
    if len(prefix):
        print("stdout prefix, excluded from comparison:")
        for line in prefix:
            print("stdout:", line.strip())
    if len(suffix):
        print("stdout suffix, excluded from comparison:")
        for line in suffix:
            print("stdout:", line.strip())
    return stdout[begin_index + 1 : end_index]


# HOC: procedure to print all lines of all Graphs
prgraphs = r"""
proc prgraphs() {local i, j, k  localobj xvec, yvec, glist
  xvec = new Vector()
  yvec = new Vector()
  glist = new List("Graph")
  printf("Graphs %d\n", glist.count)
  for i=0, glist.count-1 {
    printf("%s\n", glist.o(i))
    k = 0
    for (j=-1; (j=glist.o(i).line_info(j, xvec)) != -1; ){
      k += 1
    }
    printf("lines %d\n", k)
    for (j=-1; (j=glist.object(i).getline(j, xvec, yvec)) != -1; ){
      printf("points %d\n", xvec.size)
      printf("xvec%d\n", j)
      xvec.printf("%g\n")
      printf("yvec%d\n", j)
      yvec.printf("%g\n")
    }
  }
}
"""

# Run all the demos and compare their results to the reference
for i in range(1, 8):
    data = neurondemo(prgraphs, input % i)
    data.reverse()
    # parse the prgraphs output back into a rich structure
    rich_data = []
    graphs, num_graphs = data.pop().split(maxsplit=1)
    assert graphs == "Graphs"
    num_graphs = int(num_graphs)
    for i_graph in range(num_graphs):
        graph_name = data.pop()
        lines, num_lines = data.pop().split(maxsplit=1)
        assert lines == "lines"
        num_lines = int(num_lines)
        lines = []
        for i_line in range(num_lines):
            points, num_points = data.pop().split(maxsplit=1)
            assert points == "points"
            num_points = int(num_points)
            # not clear if the number after xvec is useful
            assert data.pop().startswith("xvec")
            xvals = [data.pop() for _ in range(num_points)]
            # not clear if the number after yvec is useful
            assert data.pop().startswith("yvec")
            yvals = [data.pop() for _ in range(num_points)]
            lines.append({"x": xvals, "y": yvals})
        rich_data.append([graph_name, lines])
    # we should have munched everything
    assert len(data) == 0
    key = "demo%d" % i

    if os.uname().sysname == "Darwin":
        # Sometimes a Graph y value str differs in last digit by 1
        # Perhaps a locale issue? But float32->float64->str can differ
        # between machines. For this reason, if a number str is not
        # identical, demand the relative difference < 1e-5 (float32 accuracy)
        err = 0
        try:
            err = 0
            chk(key, rich_data)
        except AssertionError:
            err = 1
        if err:
            from math import isclose

            reltol = 1e-5
            std = chk.d[key]

            for ig, gstd in enumerate(std):
                gname = gstd[0]
                for iline, line in enumerate(gstd[1]):
                    for coord in line:
                        vstd = [float(a) for a in line[coord]]
                        vd = [float(a) for a in rich_data[ig][1][iline][coord]]
                        if vstd != vd:
                            for i, sval in enumerate(vstd):
                                if not isclose(sval, vd[i], rel_tol=reltol):
                                    print(
                                        gname,
                                        iline,
                                        coord,
                                        i,
                                        ": %g %g" % (sval, vd[i]),
                                    )
                                    assert isclose(sval, vd[i], rel_tol=reltol)
                            print(
                                gname,
                                iline,
                                coord,
                                " float32 not identical but within rel_tol=%g" % reltol,
                            )
    else:
        chk(key, rich_data)

chk.save()

################################################
# test_many_ions.py copied below and removed to fix a CI coverage test failure:
# libgcov profiling error:/home/...demo/release/x86_64/mod_func.gcda:overwriting
# an existing profile data with a different timestamp.
# I was unable to reproduce the coverage test failure on my linux desktop by
# experimenting with parallel runs of the distinct tests. Nevertheless,
# CI coverage is successful when the use of the neurondemo shared library is
# serialized into this file.
# I'd rather keep the file and run from here via something like
# run("python test_many_ions.py", "")
# but cmake installs each hoctest file in a separate folder and "python" may not
# be the name of the program we need to run.
###########################
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

############################################
