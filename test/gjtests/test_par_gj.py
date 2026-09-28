#!/usr/bin/env python

## Test of ParallelTransfer-based gap junctions. Assumes the presence
## of a conductance-based half-gap junction model ggap.mod

import sys, os, itertools, argparse, time
import numpy as np
from neuron import h


cells = []
gjlist = []
vrecs = []
stims = []


class MyCell:
    _ids = itertools.count(0)

    def __repr__(self):
        return "MyCell[%d]" % self.id

    def __init__(self):
        self.id = next(self._ids)
        # create the morphology and connect it
        self.soma = h.Section(name="soma", cell=self)
        self.dend = h.Section(name="dend", cell=self)
        self.dend.connect(self.soma(0.5))
        self.soma.insert("pas")
        self.dend.insert("pas")
        self.dend(0.5).pas.e = -65
        self.soma(0.5).pas.e = -65


## Creates half-gap junction mechanism
def mkgap(pc, sec, gid, secpos, sgid, dgid, w, gjlist):
    myrank = int(pc.id())

    seg = sec(secpos)
    gj = h.ggap(seg)
    gj.g = w

    pc.source_var(seg._ref_v, sgid, sec=sec)
    pc.target_var(gj, gj._ref_vgap, dgid)

    if myrank == 0:
        print(
            "mkgap: gid %i: sec=%s sgid=%i dgid=%i w=%f"
            % (gid, str(sec), sgid, dgid, w)
        )

    gjlist.append(gj)

    return gj


def mkcells(pc, ngids):
    nranks = int(pc.nhost())
    myrank = int(pc.id())

    assert nranks <= ngids

    for gid in range(ngids):
        if gid % nranks == myrank:
            cell = MyCell()
            nc = h.NetCon(cell.soma(0.5)._ref_v, None, sec=cell.soma)
            pc.set_gid2node(gid, myrank)
            pc.cell(gid, nc, 1)
            cells.append(cell)

            # Current injection into section
            stim = h.IClamp(cell.soma(0.5))
            if gid % 2 == 0:
                stim.delay = 10
            else:
                stim.delay = 20
            stim.dur = 20
            stim.amp = 10
            stims.append(stim)

            # Record membrane potential
            v = h.Vector()
            v.record(cell.dend(0.5)._ref_v)
            vrecs.append(v)

            if myrank == 0:
                print(
                    "Rank %i: created gid %i; stim delay = %.02f"
                    % (myrank, gid, stim.delay)
                )


## Creates gap junctional connections:
## The first halfgap is created on even gids and gid 0 and the second
## halfgap is created on odd gids.
def mkgjs(pc, ngids):
    nranks = int(pc.nhost())
    myrank = int(pc.id())

    ggid = 2e6  ## gap junction id range is intended to not overlap with gid range
    for gid in range(0, ngids, 2):
        # source gid: all even gids
        src = gid
        # destination gid: all odd gids
        dst = gid + 1

        # source gap junction gid
        sgid = ggid
        # destination gap junction gid
        dgid = ggid + 1

        # if the source gid exists on this rank, create the half gap from src to dst
        if pc.gid_exists(src) > 0:
            cell = pc.gid2cell(src)
            sec = cell.dend
            ggap = mkgap(pc, sec, gid, 0.5, sgid, dgid, 1.0, gjlist)

        # if the destination gid exists on this rank, create the half gap from dst to src
        if pc.gid_exists(dst) > 0:
            cell = pc.gid2cell(dst)
            sec = cell.dend
            ggap = mkgap(pc, sec, gid, 0.5, dgid, sgid, 1.0, gjlist)

        ggid += 2


def main():
    parser = argparse.ArgumentParser(description="Parallel transfer test.")
    parser.add_argument(
        "--sparse-partrans",
        dest="sparse_partrans",
        default=False,
        action="store_true",
        help="use sparse parallel transfer",
    )
    parser.add_argument(
        "--result-prefix",
        default=".",
        help="place output files in given directory (must exist before launch)",
    )
    parser.add_argument(
        "--ngids", default=2, type=int, help="number of gids to create (must be even)"
    )

    args, unknown = parser.parse_known_args()

    pc = h.ParallelContext()
    myrank = int(pc.id())

    mkcells(pc, args.ngids)
    mkgjs(pc, args.ngids)

    pc.setup_transfer()

    if args.sparse_partrans:
        if hasattr(h, "nrn_sparse_partrans"):
            h.nrn_sparse_partrans = 1

    rec_t = h.Vector()
    rec_t.record(h._ref_t)

    wt = time.time()

    h.dt = 0.25
    pc.set_maxstep(10)
    h.finitialize(-65)
    pc.psolve(500)

    total_wt = time.time() - wt

    gjtime = pc.vtransfer_time()

    print("rank %d: parallel transfer time: %.02f" % (myrank, gjtime))
    print("rank %d: total compute time: %.02f" % (myrank, total_wt))

    output = itertools.chain(
        [np.asarray(rec_t.to_python())],
        [np.asarray(vrec.to_python()) for vrec in vrecs],
    )
    np.savetxt(
        "%s/ParGJ_%04i.dat" % (args.result_prefix, myrank),
        np.column_stack(tuple(output)),
    )

    pc.runworker()
    pc.done()

    h.quit()


main()
