"""
UnitTests of the python interface to the hoc.Vector class.

$Id$
"""

import unittest
from neuron import h


class Bench(object):
    def __init__(self, g, l):
        self.g = g
        self.l = l

    def __call__(self, cmd, repeat=5):
        from time import time

        t = 0
        for i in range(repeat):
            t1 = time()
            exec(cmd, self.g, self.l)
            t2 = time()
            t += t2 - t1

        print('Executed "%s".  Elapsed = %f s' % (cmd, t / repeat))


class VectorTestCase(unittest.TestCase):
    """Tests of the hoc.Vector"""

    def testEndian(self):
        """Test that Vector.__array_interface__ returns the proper byteorder (endian)"""

        endian_map = {"little": "<", "big": ">"}

        import sys

        sys_endian = endian_map[sys.byteorder]
        v = h.Vector(10)
        assert sys_endian == v.__array_interface__["typestr"][0]

    def testBytesize(self):
        """Test that Vector.__array_interface__ returns the proper bytesize (of a double)"""

        try:
            import numpy

            v = h.Vector(10)
            a = numpy.array([], dtype=float)
            assert a.__array_interface__["typestr"] == v.__array_interface__["typestr"]
        except:
            pass

    def testPerformance(self):
        """Test performance of Vector<->list,array"""

        try:
            import numpy

            bench = Bench(globals(), locals())
            print("\n")
            bench("l = range(1000000)")
            bench("v = h.Vector(l)")
            print("inplace:")
            bench("v.from_python(l)")
            bench("a = numpy.array(v)")
            print("inplace:")
            bench("v.to_python(a)")
            bench("v2 = h.Vector(a)")
            print("inplace:")
            bench("v2.from_python(a)")
            bench("l2 = list(v2)")
            print("inplace:")
            bench("v.to_python(l2)")
            bench("v2 = h.Vector(a[::-1])")
            bench("a2 = numpy.array(v2)")
        except:
            pass

    def testNumpyInteraction(self):
        """Testing numpy.array <=> hoc.Vector interaction"""

        try:
            import numpy
            from numpy import alltrue, array

            a = numpy.random.normal(size=10000)
            # b = numpy.random.normal(size=10000)
            v = h.Vector(a)
            a1 = array(v)
            assert alltrue(a == a1), 'numpy array "a" not equal to array(Vector(a))'
            v = h.Vector(a[::-1])
            assert alltrue(
                array(v)[::-1] == a
            ), "Vector(a) malfuctions when a is a sliced array"

            # inplace operations

            # todo
        except:
            pass


def suite():

    suite = unittest.makeSuite(VectorTestCase, "test")
    return suite


if __name__ == "__main__":

    # unittest.main()
    runner = unittest.TextTestRunner(verbosity=2)
    runner.run(suite())
