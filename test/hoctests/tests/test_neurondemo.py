from neuron import h
from subprocess import Popen, PIPE, STDOUT
import hashlib


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

# Obtain raw and md5 results for all demos.
rawresult = {}
md5result = {}
for i in range(1, 8):
    data = neurondemo(prgraphs, input % i)
    data = "\n".join(data)
    key = "demo%d" % i
    rawresult[key] = data
    md5result[key] = hashlib.md5(data.encode("utf-8")).hexdigest()


# MD5 of the standard results of each demo
std = {
    "demo1": "52a75e06d60b4c2827c6587ab19a3161",
    "demo2": "a8cd6f2d40b3a97b2ea5683a59c88a3e",
    "demo3": "cd8614644f8953453434f106ab545738",
    "demo4": "c8d7c2dd850d0208658e0290b4b3db64",
    "demo5": "b8e6a7a8a58087900496ec2ca96f6155",
    "demo6": "71c2c5c6a44a66ce04633d59942704bf",
    "demo7": "02d2746c67bb6399fd6c02c7932aaafe",
}

# check the md5 results and print first different raw result.
for key, val in md5result.items():
    if val != std[key]:
        print(key, " DIFFERS FROM STANDARD")
        print(rawresult[key])
        assert val == std[key]
