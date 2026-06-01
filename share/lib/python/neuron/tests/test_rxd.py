from neuron import n
import neuron
import unittest
import sys
import os
import json
import platform
from multiprocessing import Process, Lock

try:
    import multiprocessing as mp

    mp.set_start_method("spawn")
except:
    pass

# load the reference data from rxd_data.json
fdir = os.path.dirname(os.path.abspath(__file__))
test_data = json.load(open(os.path.join(fdir, "test_rxd.json"), "r"))


def scalar_bistable(lock, path=None):
    from neuron import rxd

    n.load_file("stdrun.hoc")
    s = n.Section(name="s")
    s.nseg = 101
    cyt = rxd.Region(n.allsec())
    c = rxd.Species(
        cyt, name="c", initial=lambda node: 1 if 0.4 < node.x < 0.6 else 0, d=1
    )
    r = rxd.Rate(c, -c * (1 - c) * (0.3 - c))
    n.finitialize()
    n.run()

    # check the results
    result = n.Vector(c.nodes.concentration)
    if path is not None:
        lock.acquire()
        if os.path.exists(path):
            data = json.load(open(path, "r"))
        else:
            data = {}
        data["scalar_bistable_data"] = list(result)
        json.dump(data, open(path, "w"), indent=4)
        lock.release()
    else:
        cmpV = n.Vector(test_data["scalar_bistable_data"])
        cmpV.sub(result)
        cmpV.abs()
        if cmpV.sum() >= 1e-6:
            sys.exit(-1)
    sys.exit(0)


def trivial_ecs(scale, lock, path=None):
    from neuron import h, crxd as rxd
    import numpy
    import warnings

    warnings.simplefilter("ignore", UserWarning)
    n.load_file("stdrun.hoc")
    tstop = 10
    if scale:  # variable step case
        n.CVode().active(True)
        n.CVode().event(tstop)
    else:  # fixed step case
        n.CVode().active(False)
        n.dt = 0.1

    sec = n.Section()  # NEURON requires at least 1 section

    # enable extracellular RxD
    rxd.options.enable.extracellular = True

    # simulation parameters
    dx = 1.0  # voxel size
    L = 9.0  # length of initial cube
    Lecs = 21.0  # lengths of ECS

    # define the extracellular region
    extracellular = rxd.Extracellular(
        -Lecs / 2.0,
        -Lecs / 2.0,
        -Lecs / 2.0,
        Lecs / 2.0,
        Lecs / 2.0,
        Lecs / 2.0,
        dx=dx,
        volume_fraction=0.2,
        tortuosity=1.6,
    )

    # define the extracellular species
    k_rxd = rxd.Species(
        extracellular,
        name="k",
        d=2.62,
        charge=1,
        atolscale=scale,
        initial=lambda nd: 1.0
        if abs(nd.x3d) <= L / 2.0 and abs(nd.y3d) <= L / 2.0 and abs(nd.z3d) <= L / 2.0
        else 0.0,
    )

    # record the concentration at (0,0,0)
    ecs_vec = n.Vector()
    ecs_vec.record(k_rxd[extracellular].node_by_location(0, 0, 0)._ref_value)
    n.finitialize()
    n.continuerun(tstop)  # run the simulation

    if path is not None:
        lock.acquire()
        if os.path.exists(path):
            data = json.load(open(path, "r"))
        else:
            data = {}
        if "trivial_ecs_data" not in data:
            data["trivial_ecs_data"] = {}
        data["trivial_ecs_data"][str(scale)] = list(ecs_vec)
        json.dump(data, open(path, "w"), indent=4)
        lock.release()
    else:
        # compare with previous solution
        ecs_vec.sub(n.Vector(test_data["trivial_ecs_data"][str(scale)]))
        ecs_vec.abs()
        if ecs_vec.sum() > 1e-9:
            sys.exit(-1)
    sys.exit(0)


class RxDTestCase(unittest.TestCase):
    """Tests of rxd"""

    @classmethod
    def setUpClass(cls):
        # Check for --save in command-line arguments
        cls.path = None
        cls.lock = Lock()
        if len(sys.argv) > 1:
            for arg1, arg2 in zip(sys.argv, sys.argv[1:]):
                if arg1 == "--save":
                    cls.path = arg2
                    break

    def test_rxd(self):
        p = Process(target=scalar_bistable, args=(self.lock, self.path))
        p.start()
        p.join()
        assert p.exitcode == 0
        return 0

    def test_ecs_diffusion_fixed_step(self):
        p = Process(target=trivial_ecs, args=(False, self.lock, self.path))
        p.start()
        p.join()
        assert p.exitcode == 0
        return 0

    @unittest.skipIf(
        platform.machine() in ["aarch64", "arm64"],
        "see discussion in #3143",
    )
    def test_ecs_diffusion_variable_step_coarse(self):
        p = Process(target=trivial_ecs, args=(1e-2, self.lock, self.path))
        p.start()
        p.join()
        assert p.exitcode == 0
        return 0

    @unittest.skipIf(
        platform.machine() in ["aarch64", "arm64"],
        "see discussion in #3143",
    )
    def test_ecs_diffusion_variable_step_fine(self):
        p = Process(target=trivial_ecs, args=(1e-5, self.lock, self.path))
        p.start()
        p.join()
        assert p.exitcode == 0
        return 0


def suite():
    return unittest.defaultTestLoader.loadTestsFromTestCase(RxDTestCase)


def test():
    runner = unittest.TextTestRunner(verbosity=2)
    runner.run(suite())


if __name__ == "__main__":
    """Run the rxd tests unless --save <path> is given, in which case save the
    test data to that path without comparing the results to the reference data.
    """
    test()
