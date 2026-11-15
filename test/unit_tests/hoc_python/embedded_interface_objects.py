"""Run with nrniv -python (ctest: embedded_interface_objects).

There the legacy hoc module still converts Python values for HOC (nrnpy_po2ho),
but the objects `h` makes come from the pure-Python interface. HOC must get the
HOC object such a wrapper holds, not a PythonObject around the wrapper, or a
NetCon target, MUSIC's index2target and the like reject it.
"""

from neuron import h


class Holder:
    pass


def hoc_name(expr):
    h("strdef name_")
    h(f'sprint(name_, "%s", {expr})')
    return h.name_


def main():
    soma = h.Section(name="soma")
    syn = h.ExpSyn(soma(0.5))
    holder = Holder()
    holder.syn = syn
    holder.nothing = None
    h("objref holder, got, nc, nil")
    h.holder = holder

    h("got = holder.syn")
    assert hoc_name("got") == hoc_name("holder.syn") == syn.hname(), hoc_name("got")
    assert h.got == syn

    # A NetCon target has to be a point process.
    h("nc = new NetCon(nil, holder.syn)")
    assert h.nc.syn() == syn

    # Back to Python through a method HOC calls: the legacy module passes the
    # ExpSyn as its own HocObject, which must compare equal to syn.
    class Checker:
        def check(self, obj):
            self.got = obj
            return 1

    checker = Checker()
    h("objref checker")
    h.checker = checker
    h("checker.check(holder.syn)")
    assert checker.got == syn and syn.same(checker.got), type(checker.got)
    del checker

    h("got = holder.nothing")
    assert hoc_name("got") == "NULLobject", hoc_name("got")

    # The conversion's reference goes with the HOC variable that took it.
    h("nc = nil")
    h("checker = nil")
    del holder.syn, syn
    assert h.List("ExpSyn").count() == 0, h.List("ExpSyn").count()


main()
h.quit()
