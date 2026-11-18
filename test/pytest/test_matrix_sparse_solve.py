def test_sparse_solve_unsymmetric_pattern():
    import numpy as np
    from neuron import h

    for row, col in ((1, 0), (0, 1)):
        a = 4.0 * np.eye(5)
        a[row, col] = -2.0
        matrix = h.Matrix(5, 5, 2)
        for i in range(5):
            for j in range(5):
                if a[i, j] != 0:
                    matrix.setval(i, j, a[i, j])

        expected = np.full(5, 0.25)
        expected[row] = 0.375
        result = np.array(matrix.solv(h.Vector(np.ones(5))).to_python())
        np.testing.assert_allclose(result, expected, rtol=0, atol=1e-14)
        np.testing.assert_allclose(a @ result, np.ones(5), rtol=0, atol=1e-14)

        reused = np.array(matrix.solv(h.Vector(np.full(5, 2.0)), 1).to_python())
        np.testing.assert_allclose(reused, 2 * expected, rtol=0, atol=1e-14)
        for i in range(5):
            for j in range(5):
                assert matrix.getval(i, j) == a[i, j]
