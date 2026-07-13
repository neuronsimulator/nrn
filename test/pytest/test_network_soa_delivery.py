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
    assert 1.0 <= t_gmax <= 1.2, f"expected peak near 1.025 ms window, got t_gmax={t_gmax}"


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
