"""Two single-compartment hh + IClamp cells on pc.nthread(2).

Localizes ELECTRODE_CURRENT multi-thread behavior (IClamp / HalfGap family)
without gap junctions or ringtest topology.

Runs as:

- NEURON CPU nthread=2 reference (both cells must depolarize / spike)
- Backend under test via backend_helper:
  - CoreNEURON CPU  (CORENRN_ENABLE_GPU unset/false)
  - CoreNEURON GPU  (CORENRN_ENABLE_GPU=true)
  - NEURON native GPU (NRN_GPU_BACKEND_TEST=native)

Gates ELECTRODE multi-thread (IClamp). As of 2026-07 this is green for
NEURON CPU, CoreNEURON CPU/GPU, and NEURON native GPU nthread=2 with an
explicit one-cell-per-thread partition. Keep it as a regression for the
electrode path while S3 ringtest ``-gap -nt 2`` residual is chased separately
(HalfGap + setup_transfer; g=0 still half-silent on the full ring).
"""

from neuron import h, gui

from backend_helper import disable_test_backend, enable_test_backend


def _clear_model(pc):
    pc.gid_clear()
    # Delete all sections from previous subtest.
    for sec in list(h.allsec()):
        h.delete_section(sec=sec)


def _build_two_cells(pc, nthread):
    """Return (cells, vmax_recorders, ics). Real worker threads when nthread>1.

    With nthread==2, explicitly partition one soma root onto each thread so
    both ELECTRODE_CURRENT (IClamp) instances cannot land on the same thread
    by accident (that would mask multi-thread electrode bugs).
    """
    _clear_model(pc)
    # Second arg 1: real std::thread workers (matches ringtest -nt).
    pc.nthread(nthread, 1 if nthread > 1 else 0)

    cells = []
    ics = []
    vs = []
    for i in range(2):
        s = h.Section(name=f"soma{i}")
        s.L = 10.0
        s.diam = 10.0
        s.insert("hh")
        ic = h.IClamp(s(0.5))
        ic.delay = 0.5
        ic.dur = 0.5
        ic.amp = 0.15
        cells.append(s)
        ics.append(ic)
        v = h.Vector()
        v.record(s(0.5)._ref_v, sec=s)
        vs.append(v)

    if nthread == 2:
        parts = [h.SectionList() for _ in range(2)]
        parts[0].append(cells[0])
        parts[1].append(cells[1])
        for ith in range(2):
            pc.partition(ith, parts[ith])
        # Sanity: each thread owns exactly one root section.
        for ith in range(2):
            owned = [sec for sec in pc.get_partition(ith)]
            assert len(owned) == 1, f"thread {ith} partition={owned}"
            assert owned[0] is cells[ith] or owned[0].name() == cells[ith].name()

    return cells, vs, ics


def _run_psolve(pc, tstop=3.0):
    pc.set_maxstep(10)
    h.dt = 0.025
    h.finitialize(-65.0)
    pc.psolve(tstop)


def _vmax(vs):
    return [float(v.max()) for v in vs]


def _assert_both_cells_driven(vmaxs, label, rest=-65.0, spike_thresh=0.0):
    """Both cells must leave rest; at least one spike-like peak is expected."""
    assert len(vmaxs) == 2, f"{label}: expected 2 vmax values, got {vmaxs}"
    for i, vm in enumerate(vmaxs):
        # Fail loud if a cell never feels IClamp (stuck at rest).
        assert vm > rest + 1.0, (
            f"{label}: cell{i} vmax={vm:g} still near rest {rest}; "
            f"IClamp CURRENT likely not applied on this thread (all vmax={vmaxs})"
        )
    # With amp/dur above, both cells should cross 0 mV (action potential).
    for i, vm in enumerate(vmaxs):
        assert vm > spike_thresh, (
            f"{label}: cell{i} vmax={vm:g} did not cross {spike_thresh} mV "
            f"(all vmax={vmaxs})"
        )


def test_iclamp_two_cells_nthread():
    pc = h.ParallelContext()
    tstop = 3.0

    # --- NEURON CPU multi-thread reference (always) ---
    disable_test_backend()
    _cells, vs, _ics = _build_two_cells(pc, nthread=2)
    _run_psolve(pc, tstop)
    cpu_nt2 = _vmax(vs)
    print(f"NEURON CPU nthread=2 vmax={cpu_nt2}")
    _assert_both_cells_driven(cpu_nt2, "NEURON CPU nthread=2")

    # --- Backend (CoreNEURON CPU/GPU or NEURON native GPU) multi-thread ---
    backend = enable_test_backend()
    _cells, vs, _ics = _build_two_cells(pc, nthread=2)
    _run_psolve(pc, tstop)
    backend_nt2 = _vmax(vs)
    print(f"{backend} nthread=2 vmax={backend_nt2}")
    _assert_both_cells_driven(backend_nt2, f"{backend} nthread=2")

    # Parity with NEURON CPU multi-thread (loose: both spike; peaks close).
    for i in range(2):
        assert abs(backend_nt2[i] - cpu_nt2[i]) < 5.0, (
            f"{backend} nthread=2 cell{i}: vmax={backend_nt2[i]:g} vs "
            f"NEURON CPU {cpu_nt2[i]:g} (diff too large)"
        )

    # --- Single-thread control: electrode path itself works on this backend ---
    _cells, vs, _ics = _build_two_cells(pc, nthread=1)
    _run_psolve(pc, tstop)
    backend_nt1 = _vmax(vs)
    print(f"{backend} nthread=1 vmax={backend_nt1}")
    _assert_both_cells_driven(backend_nt1, f"{backend} nthread=1")

    disable_test_backend()
    pc.nthread(1)


if __name__ == "__main__":
    test_iclamp_two_cells_nthread()
    h.quit()
