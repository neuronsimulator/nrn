"""
CPU delivery gate for network SoA dual-write (local/cpu-network-soa).

NetStim -> NetCon -> ExpSyn exercises:
  - Weight SoA dual-write and HOC weight[]
  - nrn_pnt_receive_by_weight_index around pnt_receive
  - PreSyn fanout order rebuild at init_events
"""

from neuron import h


def test_netstim_expsyn_delivery_near_1_025_ms():
    """Spike at t=1, delay 0.025 → receive ~1.025; g should peak soon after."""
    s = h.Section(name="soma")
    s.insert("pas")
    s.L = s.diam = 10

    syn = h.ExpSyn(s(0.5))
    syn.tau = 1
    syn.e = 0

    ns = h.NetStim()
    ns.start = 1.0
    ns.number = 1
    ns.noise = 0

    nc = h.NetCon(ns, syn)
    # Write weight through HOC (Weight SoA dual-write path when available).
    nc.weight[0] = 0.05
    nc.delay = 0.025

    h.dt = 0.025
    h.finitialize(-65)

    g_max = 0.0
    t_gmax = 0.0
    while h.t < 3.0 - h.dt / 2:
        h.fadvance()
        g = float(syn.g)
        if g > g_max:
            g_max = g
            t_gmax = float(h.t)

    assert g_max > 0.04, f"expected ExpSyn conductance peak, got g_max={g_max}"
    assert (
        1.0 <= t_gmax <= 1.2
    ), f"expected peak near 1.025 ms window, got t_gmax={t_gmax}"


def test_netcon_weight_hoc_roundtrip():
    """HOC weight set is visible to delivery (SoA dual-write + materialize)."""
    s = h.Section(name="soma2")
    s.insert("pas")
    s.L = s.diam = 10
    syn = h.ExpSyn(s(0.5))
    ns = h.NetStim()
    ns.start = 0.5
    ns.number = 1
    ns.noise = 0
    nc = h.NetCon(ns, syn)
    nc.weight[0] = 0.12
    nc.delay = 0.0
    assert abs(nc.weight[0] - 0.12) < 1e-12

    h.dt = 0.025
    h.finitialize(-65)
    g_max = 0.0
    while h.t < 2.0 - h.dt / 2:
        h.fadvance()
        g_max = max(g_max, float(syn.g))
    # Larger weight → larger g peak than default-zero would give
    assert g_max > 0.1, f"weight 0.12 should produce larger g, g_max={g_max}"


def test_savestate_weight_dualwrite_roundtrip():
    """SaveState restore must update Weight SoA, not only the weight_ heap."""
    s = h.Section(name="soma_ss")
    s.insert("pas")
    s.L = s.diam = 10
    syn = h.ExpSyn(s(0.5))
    syn.tau = 1
    syn.e = 0
    ns = h.NetStim()
    ns.start = 1.0
    ns.number = 1
    ns.noise = 0
    nc = h.NetCon(ns, syn)
    nc.weight[0] = 0.05
    nc.delay = 0.025

    h.dt = 0.025
    h.finitialize(-65)
    ss = h.SaveState()
    ss.save()

    # Mutate after save (SoA-primary HOC path).
    nc.weight[0] = 0.99
    assert abs(nc.weight[0] - 0.99) < 1e-12

    ss.restore()
    assert (
        abs(nc.weight[0] - 0.05) < 1e-12
    ), f"SaveState restore should restore HOC/SoA weight, got {nc.weight[0]}"

    # Delivery after restore must use restored weight (SoA materialize path).
    g_max = 0.0
    while h.t < 3.0 - h.dt / 2:
        h.fadvance()
        g_max = max(g_max, float(syn.g))
    assert (
        g_max > 0.04
    ), f"restored weight 0.05 should produce ExpSyn g peak, g_max={g_max}"
    assert (
        g_max < 0.5
    ), f"mutated weight 0.99 must not leak into delivery, g_max={g_max}"


def test_bbsavestate_weight_dualwrite_roundtrip(tmp_path):
    """BBSaveState restore must update Weight SoA (NetCon on a gid cell)."""

    # BBSaveState requires a real cell with a gid (section parented to a cell object).
    class Cell:
        def __init__(self):
            self.s = h.Section("soma", self)
            self.s.insert("pas")
            self.s.L = self.s.diam = 10
            self.syn = h.ExpSyn(self.s(0.5))
            self.syn.tau = 1
            self.syn.e = 0

    pc = h.ParallelContext()
    cell = Cell()
    ns = h.NetStim()
    ns.start = 1.0
    ns.number = 1
    ns.noise = 0
    nc = h.NetCon(ns, cell.syn)
    nc.weight[0] = 0.05
    nc.delay = 0.025

    gid = 7
    pc.set_gid2node(gid, pc.id())
    pc.cell(gid, h.NetCon(cell.s(0.5)._ref_v, None, sec=cell.s))

    h.dt = 0.025
    h.finitialize(-65)
    path = str(tmp_path / "bbss_weights.bbss")
    bbss = h.BBSaveState()
    bbss.save(path)

    nc.weight[0] = 0.99
    assert abs(nc.weight[0] - 0.99) < 1e-12

    bbss.restore(path)
    assert (
        abs(nc.weight[0] - 0.05) < 1e-12
    ), f"BBSaveState restore should restore HOC/SoA weight, got {nc.weight[0]}"

    pc.gid_clear()
