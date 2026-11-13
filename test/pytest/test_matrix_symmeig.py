def test_matrix_symmeig_orthogonality():
    import numpy as np
    from neuron import h

    matrices = (
        4.0 * np.eye(4) - np.ones((4, 4)),
        np.array([[2.0, 1.0], [1.0, 3.0]]),
    )
    for a in matrices:
        n = len(a)
        source = h.Matrix(n, n)
        for i in range(n):
            for j in range(n):
                source.setval(i, j, float(a[i, j]))

        # In-tree callers use the source as the destination; that case overwrites it.
        for eigenvectors in (h.Matrix(n, n), source):
            eigenvalues = np.array(source.symmeig(eigenvectors).to_python())
            q = np.array(
                [[eigenvectors.getval(i, j) for j in range(n)] for i in range(n)]
            )

            np.testing.assert_allclose(q.T @ q, np.eye(n), rtol=0, atol=1e-12)
            np.testing.assert_allclose(
                a @ q, q @ np.diag(eigenvalues), rtol=0, atol=1e-12
            )
            np.testing.assert_allclose(
                q @ np.diag(eigenvalues) @ q.T, a, rtol=0, atol=1e-12
            )
            if n == 4:
                np.testing.assert_allclose(
                    np.sort(eigenvalues), [0.0, 4.0, 4.0, 4.0], rtol=0, atol=1e-12
                )
