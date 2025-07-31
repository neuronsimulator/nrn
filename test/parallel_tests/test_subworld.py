from neuron import h
import sys

h.nrnmpi_init()
pc = h.ParallelContext()

# each subworld has 3 ranks if nhost_world is multiple of subsize
# (if not, last will have nworld%subsize ranks)
subsize = 3
pc.subworlds(subsize)

ibbs = pc.id_world() // subsize
nbbs = pc.nhost_world() // subsize
x = 0 if pc.nhost_world() % subsize == 0 else 1

if pc.id_world() == 0:
    print("id_world nhost_world id_bbs nhost_bbs ibbs nbbs  id  nhost")

print(
    f"{pc.id_world():5d} {pc.nhost_world():9d} {pc.id_bbs():8d} {pc.nhost_bbs():8d} {ibbs:7d} {nbbs + x:3d} {pc.id():5d} {pc.nhost():4d}"
)

assert pc.nhost() == (subsize if ibbs < nbbs else (pc.nhost_world() - subsize * nbbs))
assert pc.id() == pc.id_world() % subsize

# id_bbs and nhost_bbs for non-zero id not -1.
assert pc.nhost_bbs() == ((nbbs + x) if pc.id() == 0 else -1)
assert pc.id_bbs() == (ibbs if pc.id() == 0 else -1)


def f(arg):
    print(f"f({str(arg)}) id_world={pc.id_world():d} id={pc.id():d}")
    return (arg, pc.id_world(), pc.id())


x = 0


def gcntxt(arg):
    global x
    x = int(arg)
    # print("gcntxt(%s) id_world=%d" % (str(int(arg)), pc.id_world()))


def gtest(arg):
    if x != int(arg):
        h.execerror(f"gtest {x:d} != {int(arg):d}")
        pass
    # print("gtest x=%d id_world=%d" % (x, pc.id_world()))


pc.runworker()

for i in range(5):
    pc.submit(f, f"submit {i:d}")
hs = h.ref("")
while pc.working():
    pc.upkstr(hs)
    print(f"working returned {str(pc.pyret())} (submit passed {hs[0]})")

pc.context(gcntxt, 6)

for i in range(3 * pc.nhost_world()):
    pc.submit(gtest, 6)


x = 6
while pc.working():
    pass

pc.done()

h.quit()
