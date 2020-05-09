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

    @classmethod
    def psection(cls):
        """Test neuron.psection(Section)"""

        s = h.Section(name='soma')
        neuron.psection(s)

    def testpsection(self):
        from multiprocessing import Process
        p = Process(target=NeuronTestCase.psection)
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

    def testIterators(self):
        """Test section, segment, mechanism, rangevar iterators."""
        # setup model
        sections = [h.Section(name='s%d'%i) for i in range(3)]
        iclamps = [h.IClamp(sec(.5)) for sec in sections]
        for i, sec in enumerate(sections):
            sec.nseg = 3
            sec.insert('pas')
            sec.insert('hh')
        #iterate
        import hashlib
        sha = hashlib.sha256()
        for sec in h.allsec():
            for seg in sec:
                for mech in seg:
                    for var in mech:
                        txt="%s(%g).%s.%s=%g" % (sec.name(), seg.x, mech.name(), var.name(), var[0])
                        sha.update(txt.encode('utf-8'))
        d = sha.hexdigest()
        d1 = 'ac49344c054bc9e56e165fa75423d8bcb7cce96c4527f259362b527ee05103d8'
        # in case NRN_ENABLE_MOD_COMPATIBILITY=ON
        # (set by -DNRN_ENABLE_CORENEURON=ON)
        d2 = '44366906aa94a50644bc734eb23afcc25d1206c0431c4e7908698eeb2597c385'
        assert d == d1 or d == d2

    @classmethod
    def ExtendedSection(cls):
        """test prsection (modified print statement)"""
        from neuron.sections import ExtendedSection
        s = ExtendedSection(name="test")
        s.psection()

    def testExtendedSection(self):
        from multiprocessing import Process
        p = Process(target=NeuronTestCase.ExtendedSection)
        p.start()
        p.join()

    @classmethod
    def RxDexistence(cls):
        """test import rxd and geometry3d if scipy"""
        error = 0
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
                error = 1
            else:
               try:
                   a = basicRxD3D()
                   print("    basicRxD3D() ran with no exception")
               except:
                   print("'basicRxD3D()' failed")
                   error = 1
        assert(error == 0)
        return 0
    
    def testHelp(self):
        error = False
        try:
            help(h.xpanel)
        except:
            print("'help(h.xpanel)' failed")
            error = True
        self.assertFalse(error)
        return 0


    def testRxDexistence(self):
        from multiprocessing import Process
        p = Process(target=NeuronTestCase.RxDexistence)
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




