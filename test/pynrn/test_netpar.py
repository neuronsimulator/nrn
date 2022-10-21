# Error tests for netpar.cpp
# Some minor coverage increase for netpar.cpp when nhost > 1

# Prepend the location of the current script to the search path, so we can
# import from test_hoc_po.
import os, sys

sys.path.insert(0, os.path.dirname(os.path.realpath(__file__)))

from neuron import config, h

pc = h.ParallelContext()

from neuron.expect_hocerr import expect_err
from test_hoc_po import Ring

cvode = h.CVode()


def run(tstop):
    pc.set_maxstep(1)
    h.finitialize(-65)
    pc.psolve(tstop)


def mpi_test1():  # pgvts_deliver and pc.spike_record
    dtsav = h.dt
    r = Ring(3, 2)
    for gid, cell in r.cells.items():
        if type(cell) == type(h):
            cell.axon.disconnect()
            pc.multisplit(cell.axon(0), gid)
            pc.multisplit(cell.soma(1), gid)

    pc.multisplit()
    gids = h.Vector([gid for gid in r.cells])
    pc.spike_record(gids, r.spiketime, r.spikegid)
    cvode.active(1)
    cvode.condition_order(1)  # investigate why condition_order(2) fails
    cvode.debug_event(1)
    run(3.0)

    cvode.queue_mode(1, 0)
    pc.spike_compress(3, 1)

    cvode.active(0)
    h.dt = dtsav
    pc.gid_clear()
    cvode.debug_event(0)
    cvode.condition_order(1)


def mpi_test2():
    return


def err_test1():
    r = Ring(3, 2)
    expect_err("pc.set_gid2node(0, pc.id())")
    nc = pc.gid_connect(10000, r.cells[0].syn)
    expect_err("pc.set_gid2node(10000, pc.id())")
    expect_err("pc.threshold(10000)")
    expect_err("pc.cell(10000)")
    expect_err("pc.cell(99999)")
    expect_err("pc.cell(0, None)")
    expect_err("pc.gid_connect(0, None)")
    pc.set_gid2node(99999, pc.id())
    expect_err("pc.gid_connect(99999, r.cells[0].syn)")
    expect_err("pc.gid_connect(1, r.cells[0].syn, h.Vector())")
    nc = h.NetCon(None, r.cells[1].syn)
    expect_err("pc.gid_connect(1, r.cells[0].syn, nc)")
    nc = h.NetCon(None, r.cells[0].syn)
    pc.gid_connect(1, r.cells[0].syn, nc)

    pc.gid_clear()
    del nc, r
    locals()


def err_test2():
    r = Ring(3, 2)
    r.ncs[0].delay = 0
    pc.nthread(2)
    pc.set_maxstep(1)
    expect_err("h.finitialize(-65)")
    pc.nthread(1)
    r.ncs[0].delay = h.dt
    pc.set_maxstep(h.dt)
    h.finitialize(-65)
    if config.arguments["NRN_ENABLE_MPI"]:
        # Fixed step method does not allow a mindelay < dt + 1e-10
        expect_err("pc.psolve(1.0)")  # wonder if this is too stringent an error
    else:
        pc.psolve(1.0)
    r.ncs[0].delay = 0
    assert pc.set_maxstep(1) == 1.0
    cvode.queue_mode(0, 1)
    pc.set_maxstep(1)
    h.finitialize(-65)
    pc.psolve(1)
    assert cvode.queue_mode() == 0
    v1 = h.Vector(10)
    pc.max_histogram(v1)
    v2 = h.Vector(10)
    pc.max_histogram()
    # cover nrn_gid2outpresyn
    pc.prcellstate(10000, "tmp")
    pc.prcellstate(0, "tmp")

    pc.gid_clear()
    del r
    locals()


def test_1():
    if pc.nhost() > 1:
        mpi_test1()
        mpi_test2()
    else:
        err_test1()
        err_test2()


if __name__ == "__main__":
    test_1()
    pc.barrier()
    h.quit()
