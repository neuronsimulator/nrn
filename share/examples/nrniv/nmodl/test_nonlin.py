from neuron import h, gui


def test_nonlin():
    """
    test nmodl NONLINEAR statement.
    100 Artificial cells with different initial state values
    """
    ncell = 10_000
    csize = 40.0

    pc = h.ParallelContext()

    cells = [h.NonLinTest() for i in range(ncell)]

    r = h.Random()
    r.Random123(1, 2, 3)
    r.uniform(-csize, csize)

    def dif(tol):
        for cell in cells:
            residual = cell.dif()
            assert residual < tol, f"{residual = }, {tol = }"

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

    def rnd(x):
        return round(x, 10)

    solve(5, 10, 15)


if __name__ == "__main__":
    test_nonlin()
