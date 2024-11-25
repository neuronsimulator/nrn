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
        a.s = "one"
        b.s = "two"
        assert a.s == "one"
        assert b.s == "two"
        assert h.A[0].s == "one"
        assert a.p() == 7.0
        assert b.p() == 5.0
        a.a = 2
        b.a = 3
        assert a.a == 2
        assert b.a == 3
        assert h.List("A").count() == 2
        a = 1
        b = 1
        assert h.List("A").count() == 0

    @classmethod
    def psection(cls):
        """Test neuron.psection(Section)"""

        s = h.Section(name="soma")
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
        sections = [h.Section(name="s%d" % i) for i in range(3)]
        iclamps = [h.IClamp(sec(0.5)) for sec in sections]
        for i, sec in enumerate(sections):
            sec.nseg = 3
            sec.insert("pas")
            sec.insert("hh")
        # iterate
        import hashlib

        sha = hashlib.sha256()
        for sec in h.allsec():
            for seg in sec:
                for mech in seg:
                    for var in mech:
                        txt = "%s(%g).%s.%s=%g" % (
                            sec.name(),
                            seg.x,
                            mech.name(),
                            var.name(),
                            var[0],
                        )
                        sha.update(txt.encode("utf-8"))
        d = sha.hexdigest()
        d1 = "ac49344c054bc9e56e165fa75423d8bcb7cce96c4527f259362b527ee05103d8"
        # in case NRN_ENABLE_MOD_COMPATIBILITY=ON
        # (set by -DNRN_ENABLE_CORENEURON=ON)
        d2 = "44366906aa94a50644bc734eb23afcc25d1206c0431c4e7908698eeb2597c385"
        assert d == d1 or d == d2
        sections[0](0.5).na_ion.ena = 40.0  # issue #651
        assert sections[0](0.5).na_ion.ena == 40.0

    def testSectionArgOrder(self):
        """First optional arg for Section is name (but name="name" is recommended)"""
        soma = h.Section("soma")
        assert soma.name() == "soma"

    def testSectionCell(self):
        """Section.cell() internally referenced as weakref."""
        err = -1
        try:
            soma = h.Section(cell="foo", name="soma")
            err = 1
        except:
            err = 0
        assert err == 0

        class Cell:
            def __str__(self):
                return "hello"

        c = Cell()
        soma = h.Section(cell=c, name="soma")
        assert soma.name() == "hello.soma"
        assert soma.cell() == c
        del c
        assert soma.cell() is None

    def testSectionListIterator(self):
        """As of v8.0, iteration over a SectionList does not change the cas"""
        # See issue 509. SectionList iterator bug requires change to
        # longstanding behavior
        soma = h.Section(name="soma")
        soma.push()
        sections = [h.Section(name="s%d" % i) for i in range(3)]
        assert len([s for s in h.allsec()]) == 4
        sl = h.SectionList(sections)
        # Iteration over s SectionList does not change the currently accessed section
        for s in sl:
            assert 1 and h.cas() == soma
        # If an iteration does not complete the section stack is still ok.
        assert sections[1] in sl
        assert 2 and h.cas() == soma

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
        """test import rxd and geometry3d"""
        error = 0
        try:
            from neuron import rxd
            from neuron.rxd import geometry

            print("has_geometry3d is " + str(geometry.has_geometry3d))
        except Exception as e:
            print("'from neuron import rxd' failed", e)
            error = 1
        else:
            try:
                a = basicRxD3D()
                print("    basicRxD3D() ran with no exception")
            except Exception as e:
                print("'basicRxD3D()' failed", e)
                error = 1
        assert error == 0
        return 0

    def testHelp(self):
        error = False
        try:
            from neuron import doc

            print(doc.get_docstring("xpanel", ""))
        except Exception as e:
            print("'doc.get_docstring('xpanel', '')' failed:", e)
            error = True
        self.assertFalse(error)
        return 0

    def testRxDexistence(self):
        from neuron import config

        enable_rx3d = config.arguments["NRN_ENABLE_RX3D"]
        if not enable_rx3d:
            print("Skipping because NRN_ENABLE_RX3D={}".format(enable_rx3d))
            return

        from multiprocessing import Process

        p = Process(target=NeuronTestCase.RxDexistence)
        p.start()
        p.join()
        assert p.exitcode == 0
        return 0

    def test_newobj_err(self):
        """Test deletion of incompletely constructed objects"""
        print()  # Error message not on above line
        h.load_file("stdlib.hoc")  # need hoc String
        h(
            """
begintemplate Foo
endtemplate Foo

begintemplate NewObj
objref this, ob, foo1, foo2
proc init() {localobj s
  foo1 = new Foo() // Constructed before error, even partial constructions fill this field.
  if ($1 == 0) {
    execerror("generate an error") // All NewObj instances undergoing construction
  } else if ($1 == $2) {
    // This and all NewObj instances prior to this will construct successfully.
    // All after this will be partially constructed.
    // The execerror should cause only the partially constructed NewObj to
    // be destroyed.
    s = new String()
    sprint(s.s, "ob = new NewObj(%d, %d)", $1-1, $2)
    execute1(s.s, this)
  } else {
    ob = new NewObj($1-1, $2)
  }
  foo2 = new Foo() // Only instances prior to execute1 reach here.
}
endtemplate NewObj
"""
        )
        # arg[0] recursion depth
        # arg[0] - arg[1] + 1 should be successfully constructed
        # arg[1] should be partially constructed and destroyed.
        args = (4, 2)
        a = h.NewObj(*args)
        b = h.List("NewObj")
        c = h.List("Foo")
        print("#NewObj and #Foo in existence", b.count(), c.count())
        z = args[0] - args[1] + 1
        assert b.count() == z
        assert c.count() == 2 * z

        del a
        del b
        del c
        b = h.List("NewObj")
        c = h.List("Foo")
        print("after del a #NewObj and #Foo in existence", b.count(), c.count())
        assert b.count() == 0
        assert c.count() == 0

        return 1


def basicRxD3D():
    from neuron import h, rxd

    s = h.Section(name="s")
    s.L = s.diam = 1
    cyt = rxd.Region([s])
    ca = rxd.Species(cyt)
    rxd.set_solve_type(dimension=3)
    h.finitialize(-65)
    h.fadvance()
    return 1


def suite():
    import os

    # For sanitizer runtimes that need to be preloaded on macOS, we need to
    # propagate this manually so multiprocessing.Process works
    try:
        os.environ[os.environ["NRN_SANITIZER_PRELOAD_VAR"]] = os.environ[
            "NRN_SANITIZER_PRELOAD_VAL"
        ]
    except:
        pass
    return unittest.defaultTestLoader.loadTestsFromTestCase(NeuronTestCase)


if __name__ == "__main__":

    # unittest.main()
    runner = unittest.TextTestRunner(verbosity=2)
    runner.run(suite())
