import math

import numpy as np
from neuron import gui
from neuron import h


def simulate():
    s = h.Section(name="soma")
    s.L = 10
    s.diam = 10
    s.insert("recursion")
    fact = s(0.5).recursion.myfactorial

    return fact


def check_solution(f, reference):
    for n in range(10):
        exact, expected = reference(n), f(n)
        assert np.isclose(exact, expected), f"{expected} != {exact}"


if __name__ == "__main__":
    fact = simulate()
    check_solution(fact, math.factorial)
