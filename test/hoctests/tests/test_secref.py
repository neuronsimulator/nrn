from neuron import h
from neuron.expect_hocerr import expect_err, set_quiet

set_quiet(0)

# SectionRef unname and rename are undocumented but used in implementation
# of CellBuild

h("create a")
sr = h.SectionRef(sec=h.a)
assert sr.sec.name() == "a"
assert sr.rename("b") == 0  # must first be unnamed
sr.unname()
expect_err("sr.unname()")  # already unnamed
h("foo = 1")
assert sr.rename("foo") == 0  # already exists
sr.rename("b")
assert sr.sec.name() == "b"
h.delete_section(sec=h.b)
assert sr.rename("c") == 0

h("create a, b[3]")
sr = h.SectionRef(sec=h.a)
sr.unname()
sr.rename("b")  # no longer an array and the old 3 are deleted
assert sr.sec.name() == "b"
h.delete_section(sec=h.b)

# rename a bunch as an array
h("create a, b, c, d")
sr = None  # let's start over at 0
sr = [h.SectionRef(sec=i) for i in h.allsec()]
assert len(sr) == 4

# the ones to rename into a new array
lst = h.List()
for i in [0, 2, 3]:
    lst.append(sr[i])
sr[0].unname()
assert sr[0].rename("x", lst) == 0  # have not unnamed c and d
for i in lst:
    i.unname()
assert sr[0].rename("x", lst) == 1
h.topology()
print("cover a deleted section message")
h.delete_section(sec=h.x[1])
sr[0].unname()
assert sr[0].rename("y", lst) == 0
h.topology()


# cleanup
def cleanup():
    for sec in h.allsec():
        h.delete_section(sec=sec)


cleanup()

# Python sections cannot be unnamed or renamed
a = h.Section("a")
sr = h.SectionRef(sec=a)
assert sr.unname() == 0
assert a.name() == "a"
assert sr.rename("b") == 0
assert a.name() == "a"
cleanup()


# remaining "documented" methods
sr = None
lst = None
h("create a, a1, a2, a3")
h.a1.connect(h.a(0))
h.a2.connect(h.a(0.5))
h.a3.connect(h.a(1))
h.topology()
sr = [h.SectionRef(sec=sec) for sec in h.allsec()]
assert sr[0].nchild() == 3
assert sr[0].has_parent() is False
assert sr[0].has_trueparent() is False
assert sr[1].has_parent() is True
assert sr[1].has_trueparent() is False
assert sr[2].has_parent() is True
assert sr[2].has_trueparent() is True

assert sr[0].is_cas(sec=sr[0].sec) is True
assert sr[1].is_cas(sec=sr[0].sec) is False

assert sr[1].parent == sr[0].sec
assert sr[2].trueparent == sr[0].sec
assert sr[2].root == sr[0].sec
for i in range(sr[0].nchild()):
    assert sr[0].child[i] == sr[i + 1].sec

# can only get to these lines via hoc? (when implementation uses nrn_inpython)
# May want to eliminate the nrn_inpython fragments for complete coverage
# I believe hoc_execerror would work also in the python context.
assert h("%s.child[3] print secname()" % sr[0]) == False
assert h("%s.child[1][1] print secname()" % sr[0]) == False
assert h("%s.child print secname()" % sr[0]) == False
assert h("%s.parent print secname()" % sr[0]) == False
assert h("%s.trueparent print secname()" % sr[0]) == False


h.topology()

h.delete_section(sec=sr[0].sec)
expect_err("sr[0].nchild()")
expect_err("sr[0].has_parent()")
expect_err("sr[0].has_trueparent()")
expect_err("sr[0].is_cas()")
expect_err("sr[1].parent")
expect_err("sr[2].trueparent")
assert sr[2].root == sr[2].sec
expect_err("sr[1].child[5]")

h("create soma")
sr = h.SectionRef(sec=h.soma)
assert sr.exists() is True
h.delete_section(sec=h.soma)
assert sr.exists() is False

h(
    """
begintemplate Cell
public soma
create soma
endtemplate Cell
"""
)

cell = h.Cell()
sr = h.SectionRef(sec=cell.soma)
print(sr.sec)
sr.unname()

h("create c")  # fix segfault
