from neuron import h, rxd
import time

h.load_file("stdrun.hoc")

mM = 1
msec = 1
nM = 1e-6 * mM
sec = 1e3 * msec
minute = 60 * sec
hour = 60 * minute
h.dt = 1 * minute


# def run_sim():
def declare_parameters(**kwargs):
    """enables clean declaration of parameters in top namespace"""
    for key in sorted(kwargs):
        globals()[key] = rxd.Parameter(r, name=key, value=kwargs[key])


def declare_species(**kwargs):
    """enables clean declaration of species in top namespace"""
    for key in sorted(kwargs):
        globals()[key] = rxd.Species(r, name=key, initial=kwargs[key], atolscale=1 * nM)


def recorder(ptr):
    """return a vector that records the pointer"""
    vec = h.Vector()
    vec.record(ptr)
    return vec


rxd.set_solve_type(dimension=3)
cell = h.Section(name="cell")
cell.diam = cell.L = 5
r = rxd.Region(h.allsec())
rxd.nthread(4)

declare_parameters(
    vsP=1.1 * nM / hour,
    vmP=1.0 * nM / hour,
    KmP=0.2 * nM,
    KIP=1.0 * nM,
    ksP=0.9 / hour,
    vdP=2.2 * nM / hour,
    KdP=0.2 * nM,
    vsT=1.0 * nM / hour,
    vmT=0.7 * nM / hour,
    KmT=0.2 * nM,
    KIT=1.0 * nM,
    ksT=0.9 / hour,
    vdT=3.0 * nM / hour,
    KdT=0.2 * nM,
    kdC=0.01 * nM / hour,
    kdN=0.01 * nM / hour,
    k1=0.8 / hour,
    k2=0.2 / hour,
    k3=1.2 / (nM * hour),
    k4=0.6 / hour,
    kd=0.01 * nM / hour,
    V1P=8.0 * nM / hour,
    V1T=8.0 * nM / hour,
    V2P=1.0 * nM / hour,
    V2T=1.0 * nM / hour,
    V3P=8.0 * nM / hour,
    V3T=8.0 * nM / hour,
    V4P=1.0 * nM / hour,
    V4T=1.0 * nM / hour,
    K1P=2.0 * nM,
    K1T=2.0 * nM,
    K2P=2.0 * nM,
    K2T=2.0 * nM,
    K3P=2.0 * nM,
    K3T=2.0 * nM,
    K4P=2.0 * nM,
    K4T=2.0 * nM,
    n=4,
)

declare_species(
    MP=0.0614368 * nM,
    P0=0.0169928 * nM,
    P1=0.0141356 * nM,
    P2=0.0614368 * nM,
    MT=0.0860342 * nM,
    T0=0.0217261 * nM,
    T1=0.0213384 * nM,
    T2=0.0145428 * nM,
    C=0.207614 * nM,
    CN=1.34728 * nM,
)

MTtranscription = rxd.Rate(MT, vsT * KIT**n / (KIT**n + CN**n))
MPtranscription = rxd.Rate(MP, vsP * KIP**n / (KIP**n + CN**n))
MTdegradation = rxd.Rate(MT, -(vmT * MT / (KmT + MT) + kd * MT))
MPdegradation = rxd.Rate(MP, -(vmP * MP / (KmP + MP) + kd * MP))
T0production = rxd.Rate(T0, ksT * MT)
T0degradation = rxd.Rate(T0, -kd * T0)
T1degradation = rxd.Rate(T1, -kd * T1)
T2degradation = rxd.Rate(T2, -kd * T2)
T2degradation_due_to_light = rxd.Rate(T2, -vdT * T2 / (KdT + T2))
T0toT1 = rxd.Reaction(
    T0, T1, V1T * T0 / (K1T + T0), V2T * T1 / (K2T + T1), custom_dynamics=True
)
T1toT2 = rxd.Reaction(
    T1, T2, V3T * T1 / (K3T + T1), V4T * T2 / (K4T + T2), custom_dynamics=True
)
P0production = rxd.Rate(P0, ksP * MP)
P0degradation = rxd.Rate(P0, -kd * P0)
P1degradation = rxd.Rate(P1, -kd * P1)
P2degradation = rxd.Rate(P2, -kd * P2 - vdP * P2 / (KdP + P2))
P0toP1 = rxd.Reaction(
    P0, P1, V1P * P0 / (K1P + P0), V2P * P1 / (K2P + P1), custom_dynamics=True
)
P1toP2 = rxd.Reaction(
    P1, P2, V3P * P1 / (K3P + P1), V4P * P2 / (K4P + P2), custom_dynamics=True
)
P2T2toC = rxd.Reaction(
    P2 + T2, C, 0 + k3, 0 + k4
)  # the 0+ works around a bug prior to NEURON 7.7
CtoCN = rxd.Reaction(
    C, CN, 0 + k1, 0 + k2
)  # the 0+ works around a bug prior to NEURON 7.7
Cdegradation = rxd.Rate(C, -kdC * C)
CNdegradation = rxd.Rate(CN, -kdN * CN)
t = recorder(h._ref_t)
h.finitialize(-65)

mpvec = recorder(MP.nodes[0]._ref_concentration)
cnvec = recorder(CN.nodes[0]._ref_concentration)
p0vec = recorder(P0.nodes[0]._ref_concentration)
p1vec = recorder(P1.nodes[0]._ref_concentration)
p2vec = recorder(P2.nodes[0]._ref_concentration)
cvec = recorder(C.nodes[0]._ref_concentration)

h.finitialize(-65)
h.CVode().active(True)
print("starting simulation")
start = time.time()
h.continuerun(72 * hour)
finish = time.time() - start
print(finish)

pt = (p0vec.as_numpy() + p1vec + p2vec + cvec + cnvec) / nM
mp = mpvec.as_numpy() / nM
cn = cnvec.as_numpy() / nM
t_in_hours = t.as_numpy() / hour

if __name__ == "__main__":
    from matplotlib import pyplot as plt

    plt.figure(figsize=(8, 6))
    plt.plot(t_in_hours, mp, label="MP")
    plt.plot(t_in_hours, cn, label="CN")
    plt.plot(t_in_hours, pt, label="PT")
    plt.legend()
    plt.xlabel("t (hours)")
    plt.ylabel("concentration (nM)")
    plt.show()
