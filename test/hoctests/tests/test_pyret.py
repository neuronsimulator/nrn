# See PR #3563
# HOC PythonObject memory leak due to refcount not going to 0 for
# upkpyobj, pyret, and wrapped PyObject args when a submit job was executed
# during working. Basically anything that used pickle2po.  Since one of these 4
# cases involved subworlds, test/parallel_tests/test_subworld.py was
# also modified to cover a code fragment in subworld_worker_execute.
# The substantive assertion is that the count of PythonObject hoc
# objects is 0 after all submit jobs have been executed and results returned
# to rank 0.

from neuron import h

pc = h.ParallelContext()


def pyobjcnt():
    return h.List("PythonObject").count()


def f(a, b, c):
    id = h.hoc_ac_
    # print(f"f a={a} b={b} c={c} id={id} pc.id={pc.id()}")
    assert int(b) == len(c)
    x = {}
    x[a] = h.Vector(100000 // 8)
    pc.post(id, [a, b, c])
    return x


def submit(num):
    for i in range(num):
        pc.submit(f, i, str(i), [j for j in range(i)])


def comp():
    while True:
        id = pc.working()
        if id == 0:
            break
        r = pc.pyret()
        pc.take(id)
        bb = pc.upkpyobj()
        assert int(bb[0]) == len(bb[2])
        # print(f"key {id}: {bb}")
        # print(f"job id {id}  memory ", h.nrn_mallinfo(0))


def test_pyobjleak():
    pc.runworker()
    submit(10)
    comp()
    pc.done()
    # print(f"{pyobjcnt()} PythonObjects")
    assert pyobjcnt() == 0


if __name__ == "__main__":
    test_pyobjleak()
    h.quit()
