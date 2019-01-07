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

        from ._subclass import A1
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

    def psection(self):
        """Test neuron.psection(Section)"""

        s = h.Section(name='soma')
        neuron.psection(s)

    def testpsection(self):
        from multiprocessing import Process
        p = Process(target=self.psection)
        p.start()
        p.join()

    def testABI(self):
        """Test use of some  Py_LIMITED_API for python3."""

        # Py_nb_bool
        assert True if h else False
        assert True if h.List else False
        # ensure creating a List doesn't change the truth value
        l = h.List()
        assert True if h.List else False
        assert False if l else True
        v = h.Vector(1)
        l.append(v)
        assert True if l else False

        # Py_sq_length
        assert len(l) == 1
        # Py_sq_item
        assert l[0] == v
        # Py_sq_ass_item
        v.x[0] = 5
        assert v.x[0] == 5

    def ExtendedSection(self):
        """test prsection (modified print statement)"""
        from neuron.sections import ExtendedSection
        s = ExtendedSection(name="test")
        s.psection()

    def testExtendedSection(self):
        from multiprocessing import Process
        p = Process(target=self.ExtendedSection)
        p.start()
        p.join()

    
    def RxDexistence(self):
        """test import rxd and geometry3d if scipy"""
        a = 1
        try:
            import scipy
        except:
            print ("scipy not available")
        else:
            try:
                from neuron import rxd
                from neuron.rxd import geometry
                print("has_geometry3d is " + str(geometry.has_geometry3d))
            except:
                print("'from neuron import rxd' failed")
                a = 0
            else:
               try:
                   a = basicRxD3D(h, rxd)
                   print("    basicRxD3D() ran with no exception")
               except:
                   print("'basicRxD3D()' failed")
                   a = 0
        return a
        

    def testRxDexistence(self):
        from multiprocessing import Process
        p = Process(target=self.RxDexistence)
        p.start()
        p.join()
        assert(p.exitcode == 0)
        return 0


def basicRxD3D():
    from neuron import h, rxd
    s = h.Section(name='s')
    s.L = s.diam = 1
    cyt = rxd.Region([s])
    ca = rxd.Species(cyt)
    rxd.set_solve_type(dimension=3)
    h.finitialize(-65)
    h.fadvance()
    return 1

def suite():

    suite = unittest.makeSuite(NeuronTestCase,'test')
    return suite


if __name__ == "__main__":

    # unittest.main()
    runner = unittest.TextTestRunner(verbosity=2)
    runner.run(suite())




