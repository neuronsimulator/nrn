def test_matrix_bcopy_destination_growth():
    import numpy as np
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

    original = np.array([[10.0 * i + j + 1 for j in range(6)] for i in range(4)])
    source = make(original)
    expected = np.zeros((4, 6))
    expected[2:4, 3:6] = original[1:3, 2:5]

    result = source.bcopy(1, 2, 2, 3, 2, 3)
    np.testing.assert_array_equal(values(result), expected)
    np.testing.assert_array_equal(values(source), original)

    small = make(np.array([[99.0]]))
    source.bcopy(1, 2, 2, 3, 2, 3, small)
    expected[0, 0] = 99.0
    np.testing.assert_array_equal(values(small), expected)

    large_values = np.full((6, 8), -7.0)
    large = make(large_values)
    source.bcopy(1, 2, 2, 3, 2, 3, large)
    large_values[2:4, 3:6] = original[1:3, 2:5]
    np.testing.assert_array_equal(values(large), large_values)

    source.bcopy(0, 0, 2, 2, 1, 1, source)
    expected_alias = original.copy()
    expected_alias[1:3, 1:3] = original[:2, :2]
    np.testing.assert_array_equal(values(source), expected_alias)
