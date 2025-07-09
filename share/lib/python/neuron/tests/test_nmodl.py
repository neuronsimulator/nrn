import unittest

from neuron import h


def _test_nmodl_ast():
    """
    Test for https://github.com/neuronsimulator/nrn/issues/3517
    """
    assert h.hh.ast


def test_nmodl():
    """
    Controller for all NMODL tests
    """
    _test_nmodl_ast()


if __name__ == "__main__":
    test_nmodl()
