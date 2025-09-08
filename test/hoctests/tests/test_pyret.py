from neuron import h

pc = h.ParallelContext()


def pyobjcnt():
    return h.List("PythonObject").count()


def f(a, b, c):
    id = h.hoc_ac_
    print(f"f a={a} b={b} c={c} id={id} pc.id={pc.id()}")
    x = {}
    x[a] = h.Vector(100000 // 8)
    pc.post(id, [a, b, c])
    return x


pc.runworker()


def submit(num):
    for i in range(num):
        pc.submit(f, i, str(i), [j for j in range(i)])


submit(10)


def comp():
    while True:
        id = pc.working()
        if id == 0:
            break
        r = pc.pyret()
        pc.take(id)
        bb = pc.upkpyobj()
        print(f"key {id}: {bb}")
        print(f"job id {id}  memory ", h.nrn_mallinfo(0))


comp()

pc.done()

print(f"{pyobjcnt()} PythonObjects")
assert pyobjcnt() == 0

h.quit()
