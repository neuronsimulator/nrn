import neuron
import pytest


def test_builtin():
    with pytest.raises(TypeError):

        class MyList(neuron.HocBaseObject, hoc_type=neuron.h.List):
            pass


def test_hocbase():
    class MyStim(neuron.HocBaseObject, hoc_type=neuron.h.NetStim):
        pass

    assert issubclass(MyStim, neuron.hoc.HocObject)
    assert issubclass(MyStim, neuron.HocBaseObject)
    assert MyStim._hoc_type == neuron.h.NetStim


def test_hoc_template_hclass():
    neuron.h(
        """
            begintemplate A
            public x, s, o, xa, oa, f, p
            strdef s
            objref o, oa[2]
            double xa[3]
            proc init() { \
              x = $1 \
            }
            func f() { return $1*xa[$2] }
            proc p() { x += 1 }
            endtemplate A
        """
    )

    class A1(neuron.hclass(neuron.h.A)):
        def __new__(cls, arg):
            return super().__new__(cls, arg)

        def __init__(self, arg):
            self.bp = self.baseattr("p")

        def p(self):
            self.bp()
            return self.x

    a = A1(5)
    assert a.x == 5.0
    assert a.p() == 6.0
    b = A1(4)
    a.s = "one"
    b.s = "two"
    assert a.s == "one"
    assert b.s == "two"
    assert neuron.h.A[0].s == "one"
    assert a.p() == 7.0
    assert b.p() == 5.0
    a.a = 2
    b.a = 3
    assert a.a == 2
    assert b.a == 3
    assert neuron.h.List("A").count() == 2
    a = 1
    b = 1
    assert neuron.h.List("A").count() == 0


def test_pyobj_constructor():
    # Test that __new__ is required when __init__ is overridden
    with pytest.raises(TypeError):

        class PyObj(neuron.HocBaseObject, hoc_type=neuron.h.NetStim):
            def __init__(self, freq):
                super().__init__()
                self.append(first)

    class PyObj(neuron.HocBaseObject, hoc_type=neuron.h.NetStim):
        def __new__(cls, freq):
            return super().__new__(cls)

        def __init__(self, freq):
            super().__init__()
            self.interval = 1000 / freq

    p = PyObj(4)
    assert p.interval == 250


def test_pyobj_def():
    class PyObj(neuron.HocBaseObject, hoc_type=neuron.h.NetStim):
        def my_method(self, a):
            return a * 2

    p = PyObj()
    assert p.my_method(4) == 8


def test_pyobj_overloading():
    class PyObj(neuron.HocBaseObject, hoc_type=neuron.h.PatternStim):
        def play(self, i):
            self.played = True
            v = neuron.h.Vector([i])
            return self.baseattr("play")(v, v)

    p = PyObj()
    p.play(2)
    assert hasattr(p, "played")


@pytest.mark.xfail(reason="inf. recursion because baseattr finds Python attrs")
def test_bad_overload():
    class PyObj(neuron.HocBaseObject, hoc_type=neuron.h.PatternStim):
        def not_on_base(self):
            return p.baseattr("not_on_base")()

    p = PyObj()
    p.not_on_base()


def test_pyobj_inheritance():
    class PyObj(neuron.HocBaseObject, hoc_type=neuron.h.NetStim):
        pass

    class MyObj(PyObj):
        pass

    with pytest.raises(TypeError):

        class MyObj2(PyObj):
            def __init__(self, arg):
                pass

    class List(neuron.HocBaseObject, hoc_type=neuron.h.NetStim):
        def __new__(cls, *args, **kwargs):
            super().__new__(cls)

    class InitList(List):
        def __init__(self, *args):
            super().__init__()
            for arg in args:
                self.append(arg)

    l = InitList(neuron.h.NetStim(), neuron.h.NetStim())


def test_pyobj_composition():
    class A(neuron.HocBaseObject, hoc_type=neuron.h.NetStim):
        pass

    class B(neuron.HocBaseObject, hoc_type=neuron.h.NetStim):
        pass

    class C(neuron.HocBaseObject, hoc_type=neuron.h.ExpSyn):
        pass

    with pytest.raises(TypeError):
        # Composition of different HOC types is impossible.
        class D(A, C):
            pass

    class E(A, B):
        pass

    assert E._hoc_type == neuron.h.NetStim


class PickleTest(neuron.HocBaseObject, hoc_type=neuron.h.NetStim):
    def __new__(cls, *args, **kwargs):
        return super().__new__(cls)

    def __init__(self, start, number, interval, noise):
        self.start = start
        self.number = number
        self.interval = interval
        self.noise = noise

    def __reduce__(self):
        return (
            self.__class__,
            (self.start, self.number, self.interval, self.noise),
        )


def test_pyobj_pickle():
    import pickle

    p = pickle.loads(pickle.dumps(PickleTest(10, 100, 1, 0)))
    assert p.__class__ is PickleTest
    assert p.start == 10
