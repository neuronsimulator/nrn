from neuron import h

pc = h.ParallelContext()


def f(a, b, c):
    print(f"f a={a} b={b} c={c} pc.id={pc.id()}")
    x = {}
    x[a] = h.Vector(100000 // 8)
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
        print(f"job id {id}  memory ", h.nrn_mallinfo(0))


comp()

pc.done()

z = h.List("PythonObject")
print(f"{z.count()} PythonObjects")

h.quit()
