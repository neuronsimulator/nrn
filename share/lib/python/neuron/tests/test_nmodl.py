import unittest

from neuron import h


class NmodlTestCase(unittest.TestCase):
    """
    Tests for NMODL
    """

    def test_nmodl_ast(self):
        """
        Test for https://github.com/neuronsimulator/nrn/issues/3517
        """
        assert h.hh.ast


def suite():
    return unittest.defaultTestLoader.loadTestsFromTestCase(NmodlTestCase)


def test_nmodl():
    runner = unittest.TextTestRunner(verbosity=2)
    runner.run(suite())


if __name__ == "__main__":
    test_nmodl()
