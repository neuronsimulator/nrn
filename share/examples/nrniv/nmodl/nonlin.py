# test nmodl NONLINEAR statement.
# 100 Artificial cells with different initial state values

from neuron import h, gui

pc = h.ParallelContext()

ncell = 10000
csize = 40.0

cells = [h.NonLinTest() for i in range(ncell)]

r = h.Random()
r.Random123(1, 2, 3)
r.uniform(-csize, csize)


def dif(tol):
    for cell in cells:
        assert cell.dif() < tol


def solve(a, b, c):
    for cell in cells:
        cell.a = a
        cell.b = b
        cell.c = c
        cell.x0 = r.repick()
        cell.y0 = r.repick()
        cell.z0 = r.repick()
    h.finitialize()
    dif(1e-7)
    for cell in cells:
        cell.x0 = cell.x
        cell.y0 = cell.y
        cell.z0 = cell.z
    h.finitialize()
    dif(1e-13)


solve(5, 10, 15)


def rnd(x):
    return round(x, 10)


def distinct():
    sol = set([(rnd(c.x), rnd(c.y), rnd(c.z)) for c in cells])
    sol = [i for i in sol]
    sol.sort()
    print(
        "distinct solutions for {} trials with\n    random start in cube with edge (-{}, {}).".format(
            ncell, csize, csize
        )
    )
    for i in sol:
        print(i)


distinct()
