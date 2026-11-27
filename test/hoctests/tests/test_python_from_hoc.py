# Python used from HOC programs: what HOC sees, and how a program ends when Python fails.
import os
import re
import shutil
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from tempfile import TemporaryDirectory

import pytest

# In __main__, which is the PythonObject p of the programs.
PRELUDE = """
import enum, fractions, sys
class C: pass
class F(float): pass
class U:
    def __float__(self): return 1.5
class N:
    def __init__(self, x): self.x = x
    def __add__(self, o): return N(self.x + getattr(o, "x", o))
    def __mul__(self, o): return self.x * o
    __rmul__ = __mul__
class Drop:
    def __getitem__(self, i):
        from neuron import h
        h.p = None
        raise KeyError(i)
c, f, u, quarter, drop, half = C(), F(2.5), U(), fractions.Fraction(1, 4), Drop(), N(0.5)
four, n, nul, d, e, L = enum.IntEnum("E", "four").four, [], "a\\0b", [1, 2], {}, [1, 2, 3.5]
def types(*a): return " ".join(type(x).__name__ for x in a)
def show(x): return type(x).__name__ + str(list(map(float, x)))
def twice(x): return F(2 * x)
def event(): n.append(1)
def bad(*a): raise KeyError("k")
def bye(*a): sys.exit(5)
"""

# name: (HOC program, stdout)
HOC = {
    "values": (
        """strdef s
        objref l
        { s = "abc"  l = new List()  p.c.x = 3  p.c.s = "str"  p.d._[0] = 9 }
        { s = p.types(1, s, "lit", nil, l, p)  o = p.e.get("none") }
        print s, p.c.x, p.d._[0], p.d._[1], p.abs(-3), p.bool(2)
        { s = p.c.s }
        { print s, o, p.len("four")  s = p.nul  o = p.half + p.half }
        print s, p.four, p.f, p.u, p.quarter, p.sys.flags.utf8_mode, o.x, p.half * 3, 2 * p.half""",
        "float str str NoneType List module3 9 2 3 1 \nstrNULLobject 4 \n"
        f"a1 2.5 1.5 0.25 {int(os.name != 'nt')} 1 1.5 1",  # LC_ALL=C is UTF-8 mode
    ),
    "Vector": (
        """strdef s
        objref vec, w
        { vec = new Vector(p.L)  w = new Vector(p.four)  w.from_python(p.L) }
        print vec.eq(w), vec.apply(p.twice).sum()
        { o = vec.to_python()  s = p.show(o) }
        print s""",
        "1 13 \nlist[2.0, 4.0, 7.0]",
    ),
    "sections and segments": (
        """{nrnpython("from neuron import h; a = h.Section('a'); a.L = 100; a.nseg = 3")}
        {nrnpython("a.insert('hh'); b = h.Section('b'); seg = a(0.5); S = [a, b]")}
        objref ic, ms, sl, rvp, vec, nc, syn
        { ic = new IClamp(p.seg)  o = ic.get_segment() }
        print ic.get_loc(), o.x
        {pop_section() distance(0, p.seg)}
        { ms = new MechanismStandard("hh")  ms.in(p.seg)  sl = new SectionList(p.S)  sl.append(p.b) }
        print distance(p.a(1)), ms.get("gnabar_hh")
        forsec sl print secname()
        { rvp = new RangeVarPlot(p.twice)  rvp.begin(p.a(0))  rvp.end(p.a(1)) }
        { vec = new Vector()  rvp.to_vector(vec) }
        create s
        access s
        { syn = new ExpSyn(.5)  nc = new NetCon(&v(.5), syn)  o = nc.preseg() }
        print vec.size(), vec.x[4], o.x
        o = new RanArt()
        print o.get_segment()""",
        "0.5 0.5 \n50 0.12 \n_pysec.a\n_pysec.b\n_pysec.b\n5 2 0.5 \n78",
    ),
    "callbacks": (
        """load_file("stdrun.hoc")
        objref st, nc, fih, cv, pc
        create s
        { st = new NetStim()  st.number = 3  st.interval = 1  st.start = 1  nc = new NetCon(st, nil) }
        { nc.record(p.event)  fih = new FInitializeHandler(p.event)  cv = new CVode() }
        proc start() { cv.event(1, p.event) }
        { fih = new FInitializeHandler("start()")  tstop = 10  run()  pc = new ParallelContext() }
        {pc.submit(p.twice, 7)}
        while (pc.working() != 0) { o = pc.pyret()  print p.len(p.n), o._ }
        { pc.pack(p.n)  pc.post("k")  pc.take("k")  o = pc.upkpyobj() }
        { print p.len(o)  o = pc.py_allgather(p.n)  print p.len(o) }""",
        "4 14 \n4 \n1",
    ),
    "caught errors": (
        """print execute1("p.nosuch", 0), execute1("p.bad()", 0), execute1("print 5", 0)
        print nrnpython("1/0"), execute1("print p.drop._[2]", 0), p""",
        "0 0 5 \n1 \n0 0 NULLobject",
    ),
    "stdin": (
        """begintemplate T
        public go, f, y
        objref p
        proc init() { nrnpython("from neuron import h; h.y = h.x + 1")  y = 99  p = new PythonObject() }
        func f() { return p.len("four") }
        proc go() { p.bad() }
        endtemplate T
        objref tt
        { x = 42  y = 0  tt = new T() }
        tt.go()
        print y, tt.y, tt.f()""",
        "43 99 4",
    ),
    "quit": (
        """{nrnpython("from neuron import h; import atexit, threading, time")}
        {nrnpython("atexit.register(lambda: print('atexit'))")}
        {nrnpython("threading.Thread(target=lambda: (time.sleep(0.1), print('thread'))).start()")}
        { o = new List()  o.append(p.h.Vector(3))  quit(3) }""",
        "thread\natexit",
    ),
}
# statements: (part of stderr, exit status); print 1 is not reached
FAIL = {
    "p.bad()": (
        "PyObject method call failed: bad\n in {file} near line 6\n p.bad()",
        1,
    ),
    "proc a() { p.twice(new Vector()) }\na()": ("near line 7\n a()", 1),
    'nrnpython("import neuron")\nx = nosuch': ("nosuch\n in {file} near line 7", 1),
    "create s\nobjref f\nf = new FInitializeHandler(p.bad)\nfinitialize()": ("'k'", 1),
    "objref f\nf = new NetStim()\nf.start = 0\no = new NetCon(f, nil)\no.record(p.bad)\n"
    'load_file("stdrun.hoc")\ncreate s\nrun()': ("'k'", 1),
    "o = new Vector(3)\no.apply(p.bad)": ("Python function call raised exception", 1),
    "o = new IClamp(p.d)": ("not a Python nrn.Segment", 1),
    "create s\ndistance(0, p.c)": ("rxd.node, or other with a segment property", 1),
    "o = new SectionList(p.d)": ("iterable must contain only Section objects", 1),
    'nrnpython("sys.exit(7)")': ("", 7),
    "p.bye()": ("", 5),
    "create s\nobjref f\nf = new FInitializeHandler(p.bye)\nfinitialize()": ("", 5),
    'execute1("p.bye()")': ("", 5),
}
PYTHON = {
    "errors": (
        """print(h.execute1('p.bad()', 0))
h('''begintemplate T
proc unref() { execute1("nosuchproc()", 0) }
endtemplate T
proc outer() { localobj o
 o = new T()
  p.bad() }''')
try: h.outer()
except RuntimeError as e: print(e)
try: h.Vector(2).apply(bye)
except RuntimeError as e: print(repr(e.__cause__))""",
        "0.0\nhocobj_call error: hoc_execerror: PyObject method call failed: bad\nNone",
    ),
    "capi": (
        """from neuron._interface import api
hook, hoc, take = api.nrn_python_hook_set, api._nrn_hoc_call_nothrow, api._python_error_take
def nested():
    print(hoc(b'p.bad()'), repr(take()))
    raise IndexError
print(hoc(b'p.nested()'), repr(take()), take())
sys.excepthook = lambda *args: print(args[0].__name__)
print(hoc(b'p.missing'), hoc(b'{execute1("p.bad()", 0)}'), type(take()).__name__)
epoch = [1]
print(hook(b'nosuch', 1), hook(b'model_epoch', []), hook(b'model_epoch', epoch))
for change in b'p.types()', b'create s', b'access s', b's { delete_section() }':
    epoch[0] = 1
    print(hoc(change), epoch)""",
        "1 None\n1 None None\nAttributeError\n1 0 NoneType\n1 2 0\n"
        "0 [None]\n0 [None]\n0 [1]\n0 [None]",
    ),
    "garbage": (
        """import gc, nrn
def cycle(*things):  # the collector clears objects in the order they were made
    c = C()
    c.things, c.me = things, c
kept = []
class Del(nrn.Section):
    def __del__(self): pass
class Back(nrn.Section):
    def __del__(self): kept.append(self)
a = h.Section('a')
h('objref ic'); h('_pysec.a.L = 5')  # from here HOC keeps the names
cycle(h.IClamp(a(0.5)), h.Vector(3), h.NetCon(h.NetStim(), None), h.Section('b'), Del('c'))
Del('d'), Back('e')
gc.collect()
print([len(h.List(k)) for k in ('IClamp', 'Vector', 'NetCon', 'NetStim')], list(h.allsec()))
kept.clear()
d = Del('d'); h.ic = h.IClamp(d(0.5))
del d
print(h.ic.get_segment(), list(h.allsec()))""",
        "[0, 0, 0, 0] [a, e]\nNone [a]",
    ),
}
# name: (program, stdout, part of stderr, exit status)
ROWS = {
    **{k: (*row, "", 3 if k == "quit" else 0) for k, row in {**HOC, **PYTHON}.items()},
    **{k: (f"{k}\nprint 1", "", *row) for k, row in FAIL.items()},
}


def run(name):
    program = ROWS[name][0] + "\n"
    # A sanitizer's runtime is in nrniv, but must be preloaded into python. macOS SIP drops
    # DYLD_INSERT_LIBRARIES before this process sees it, so it is set again from the test's copy.
    dropped = ("DISPLAY",) if name in PYTHON else ("DISPLAY", "LD_PRELOAD")
    env = {k: v for k, v in os.environ.items() if k not in dropped}
    preload = env.get("NRN_SANITIZER_PRELOAD_VAR"), env.get("NRN_SANITIZER_PRELOAD_VAL")
    if name in PYTHON and all(preload):
        env[preload[0]] = preload[1]
    with TemporaryDirectory() as directory:
        directory = directory.replace(os.sep, "/")
        prelude, file = directory + "/prelude", directory + "/program"
        start = f"exec(open({prelude!r}).read())"
        if name in PYTHON:
            command = [os.environ.get("NRN_PYTHON_EXECUTABLE", sys.executable), file]
            start += (
                "\nfrom neuron import h; h('objref p'); h('p = new PythonObject()')\n"
            )
        else:
            command = [shutil.which("nrniv"), "-notatty", "-nobanner", "-nogui", file]
            start = f'{{nrnpython("{start}")}}\n\nobjref p, o, nil\np = new PythonObject()\n\n'
        if name == "stdin":
            command[-1] = "-"
        for path, text in (prelude, PRELUDE), (file, start + program):
            with open(path, "w") as f:
                f.write(text)
        r = subprocess.run(
            command,
            env={**env, "LC_ALL": "C"},
            input=start + program if name == "stdin" else "",
            capture_output=True,
            text=True,
            timeout=120,
        )
    # load_file and friends print their value. A wheel's nrniv script prints its INFO line
    # before it starts nrniv; it gets to the pipe only where the script waits for nrniv (Windows).
    stdout = "".join(re.findall(r"^(?!\t|INFO : Using ).*\n", r.stdout, re.M)).strip()
    return stdout, r.stderr.replace(file, "{file}"), r.returncode


POOL = ThreadPoolExecutor(4)
RESULTS = {name: POOL.submit(run, name) for name in ROWS}


@pytest.mark.parametrize("name", ROWS)
def test_program(name):
    if name == "stdin" and sys.platform == "win32":
        pytest.skip("after an error HOC skips the rest of stdin, if it can seek there")
    _, stdout, stderr, status = ROWS[name]
    result = RESULTS[name].result()
    assert result == (stdout, result[1], status), result
    assert stderr in result[1], result


if __name__ == "__main__":
    sys.exit(pytest.main([__file__, "-q"]))
