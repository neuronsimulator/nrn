from neuron import h, gui
import sys

fname = sys.argv[-1]
print(fname)

f = open(fname, "r")
lines = f.readlines()
nline = len(lines)
iline = 0
data = {}
while iline < nline:
    z = lines[iline].split()
    i, total, cnt, ndetail = int(z[0]), float(z[1]), int(z[2]), int(z[3]) + 1
    print(i, total, cnt, ndetail)
    iline += 1
    data[i] = [float(lines[iline + j]) / 1000.0 for j in range(ndetail)]
    iline += ndetail

graphs = []


def plt(d, i, label):
    g = h.Graph()
    graphs.append(g)
    h.Vector(d).line(g)
    g.label(str(i) + " " + label)


for i, d in data.items():
    label = ["fadvance", "scatter/gather"]
    plt(d, label[i], "raw intervals")
    plt(sorted(d), label[i], "sorted intervals")
