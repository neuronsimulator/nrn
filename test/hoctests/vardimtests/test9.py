# test of fix for hoc statements of form obj.sec.rangvar[i]
from neuron import h

# test name1.name2.name3 where
# name1 can be objref, objref array, section, section array, obfunc (args)
# name2 can be objref, objref array, section, section array, obfunc (args)
# name3 can be objref, objref array, section, section array,
#    rangevar, rangevar[], rangevar(x), rangevar[](x), func (args)

h.load_file("expect_err.hoc")

h(
    r"""
begintemplate Foo
public soma, dend, f1
create soma, dend[3]
proc init() { local x, i, val
    create soma, dend[3]
    forall insert atst
    forall nseg = 3
    val = $1
    forall for (x, 0) {
        val += 1
        scalarrng_atst(x) = val
        for i = 0, 3 {
            val += 1
            arrayrng_atst[i](x) = val
        }
    }
}
func f1() {
    return 3 * $1
}
endtemplate Foo

objref foo, fooarray[3]
foo = new Foo(0)
for i = 0, 2 {
    fooarray[i] = new Foo((i+1)*100)
}
"""
)


def tryhoc(stmt):
    print("checking: ", stmt)
    return h.execute1(stmt)


# illustrates the original error
tryhoc("print foo.soma.arrayrng_atst[2](.7)")


def generate():
    name1 = ["foo", "fooarray[1]"]
    name2 = ["soma", "dend[2]"]
    name3 = ["scalarrng_atst", "arrayrng_atst[3]"]
    loc = ["", "(.7)"]  # Hoc: at end of range var. Python: at end of section
    for i in name1:
        for j in name2:
            for k in name3:
                for l in loc:
                    hexpr = ".".join([i, j, k]) + l
                    pexpr = ".".join([i, j + ("(.5)" if l == "" else l), k])
                    tryhoc("x = " + hexpr)
                    assert h.x == eval("h." + pexpr)


generate()

# error if more than one arclen param for array
tryhoc("foo.soma.scalarrng_atst(.7, .1)")
assert tryhoc("foo.soma.arrayrng_atst[2](.7, .1)") == 0
# subscript out of range
assert tryhoc("foo.soma.arrayrng_atst[50]") == 0
