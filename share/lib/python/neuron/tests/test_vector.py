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

    def testNegativeIndex(self):
        l = [i for i in range(10)]
        v = h.Vector(l)
        assert v[-3] == l[-3], "v[-3] Failed"
        v[-3] = 42
        l[-3] = 42
        assert v[-3] == l[-3], "Setting v[-3] Failed"

    def testSlicing(self):
        l = [i for i in range(10)]
        v = h.Vector(l)
        assert list(v[2:6]) == l[2:6], "v[2:6] Failed"
        assert list(v[-3:-1]) == l[-3:-1], "v[-3:-1] Failed"
        assert list(v[::2]) == l[::2], "v[::2] Failed"
        assert list(v[::-2]) == l[::-2], "v[::-2] Failed"
        assert list(v[3:6:2]) == l[3:6:2], "v[3:6] Failed"
        assert list(v[7:-9:-2]) == l[7:-9:-2], "v[7:-9:-2] Failed"
        assert list(v[-1::-3]) == l[-1::-3], "v[-1::-3] Failed"
        assert list(v[-2::-6]) == l[-2::-6], "v[-2::-6] Failed"
        assert list(v[7:-1:-2]) == l[7:-1:-2], "v[-2::-6] Failed"
        assert list(v[3:2:-2]) == l[3:2:-2], "v[3:2:-2] Failed"

    def testAssignmentSlicing(self):
        l = [i for i in range(10)]
        v = h.Vector(l)
        v[2:4] = [12, 13]
        l[2:4] = [12, 13]
        assert list(v[2:4]) == l[2:4], "v[2:4] Failed"
        v[-3:-1] = [-9, -9]
        l[-3:-1] = [-9, -9]
        assert list(v[-3:-1]) == l[-3:-1], "v[-3:-1] Failed"
        v[::2] = [_ for _ in range(277, 282)]
        l[::2] = [_ for _ in range(277, 282)]
        assert list(v[::2]) == l[::2], "v[::2] Failed"
        v[::-2] = [_ for _ in range(-377, -372)]
        l[::-2] = [_ for _ in range(-377, -372)]
        assert list(v[::-2]) == l[::-2], "v[::-2] Failed"
        v[3:6:2] = [-123, -456]
        l[3:6:2] = [-123, -456]
        assert list(v[3:6:2]) == l[3:6:2], "v[3:6] Failed"
        v2 = h.Vector(x for x in range(10, 20))
        v[3:8] = v2[3:8]
        assert list(v[3:8]) == list(v2[3:8]), "v[3:8] = v2[3:8] Failed"

    def testErrorHandling(self):
        l = [i for i in range(10)]
        v = h.Vector(l)
        # Input that is too short or long should raise an IndexError
        with self.assertRaises(IndexError):
            v[0:3] = [55]
        with self.assertRaises(IndexError):
            v[3:7] = (x for x in range(100, 120))


def suite():
    suite = unittest.makeSuite(VectorTestCase, "test")
    return suite


if __name__ == "__main__":

    # unittest.main()
    runner = unittest.TextTestRunner(verbosity=2)
    runner.run(suite())
