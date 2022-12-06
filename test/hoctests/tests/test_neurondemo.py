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
    result = run("neurondemo -nobanner", input)
    result = result[0].splitlines()
    result = result[result.index("ZZZbegin") + 1 : result.index("ZZZend")]
    return result


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
    chk(key, rich_data)

chk.save()
