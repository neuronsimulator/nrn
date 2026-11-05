def test_matrix_transpose():
    import numpy as np
    import pytest
    from neuron import h

    def make(values):
        matrix = h.Matrix(*values.shape)
        for i in range(values.shape[0]):
            for j in range(values.shape[1]):
                matrix.setval(i, j, float(values[i, j]))
        return matrix

    def values(matrix):
        return np.array(
            [
                [matrix.getval(i, j) for j in range(int(matrix.ncol()))]
                for i in range(int(matrix.nrow()))
            ]
        )

    original = np.arange(1.0, 13.0).reshape(3, 4)
    source = make(original)
    result = source.transpose()
    np.testing.assert_array_equal(values(result), original.T)
    np.testing.assert_array_equal(values(source), original)

    square = np.array([[1.0, 2.0], [3.0, 4.0]])
    source = make(square)
    destination = make(square)
    source.transpose(destination)
    np.testing.assert_array_equal(values(source), square)
    np.testing.assert_array_equal(values(destination), square.T)

    source.transpose(source)
    np.testing.assert_array_equal(values(source), square.T)

    source = make(original)
    with pytest.raises(RuntimeError, match="non-square matrix"):
        source.transpose(source)
    np.testing.assert_array_equal(values(source), original)
