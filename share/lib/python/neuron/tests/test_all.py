"""
Collection of all unittests for the neuron module

$Id$
"""


# import your specific test here
# and add it to the "suite" function below
from neuron.tests import test_vector, test_neuron

import unittest


def suite():
    suite = unittest.TestSuite()
    suite.addTest(test_vector.suite())
    suite.addTest(test_neuron.suite())
    # add additional test cases here
    return suite


if __name__ == "__main__":
    # unittest.main()
    runner = unittest.TextTestRunner(verbosity=2)
    runner.run(suite())
