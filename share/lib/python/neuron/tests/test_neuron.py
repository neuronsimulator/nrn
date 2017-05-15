"""
UnitTests of the python interface to the neuron class.
Items declared in neuron/__init__.py

$Id$
"""

import unittest
import neuron
from neuron import h

class NeuronTestCase(unittest.TestCase):
    """Tests of neuron"""

    def testHClass(self):
        """Test subclass of hoc class."""

        from _subclass import A1
        a = A1(5)
        assert a.x == 5.0
        assert a.p() == 6.0
        b = A1(4)
        a.s = 'one'
        b.s = 'two'
        assert a.s == 'one'
        assert b.s == 'two'
        assert h.A[0].s == 'one'
        assert a.p() == 7.0
        assert b.p() == 5.0
        a.a = 2
        b.a = 3
        assert a.a == 2
        assert b.a == 3
        assert h.List('A').count() == 2
        a = 1
        b = 1
        assert h.List('A').count() == 0

    def testpsection(self):
        """Test neuron.psection(Section)"""

        s = h.Section(name='soma')
        neuron.psection(s)

def suite():

    suite = unittest.makeSuite(NeuronTestCase,'test')
    return suite


if __name__ == "__main__":

    # unittest.main()
    runner = unittest.TextTestRunner(verbosity=2)
    runner.run(suite())




