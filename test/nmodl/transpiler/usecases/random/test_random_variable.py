import numpy as np
import scipy as sp
import hashlib

from neuron import h


def hash_array(x):
    return hashlib.sha256(" ".join([f"{xx:.10e}" for xx in x]).encode()).hexdigest()


def test_negexp():
    nseg = 10
    s = h.Section()

    s.nseg = nseg
    s.insert("random_variable")

    n_samples = 1000
    samples = [
        np.array([s(x).random_variable.negexp() for _ in range(n_samples)])
        for x in [0.34, 0.74]
    ]
    expected_hashes = [
        "3b66d7f83dc81ea485929aa8a4f347a3befee7170971dfb9c35a1a1d40ed0407",
        "896c59d129cc0a248e57c548c8bf4be7ae4d39fbd92113e37f59c332be61a2b7",
    ]

    assert hash_array(samples[0]) == expected_hashes[0]
    assert hash_array(samples[1]) == expected_hashes[1]


if __name__ == "__main__":
    test_negexp()
