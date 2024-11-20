from neuron import h, gui

somas = [h.Section(name="soma" + str(i)) for i in range(2)]
for sec in somas:
    sec.L = 10
    sec.diam = 10
    sec.insert("pas")
    sec.e_pas = -65
    sec.g_pas = 0.0001

syns = [h.ExpSynSpine(somas[0](0.5)), h.ExpSyn(somas[1](0.5))]
syns[0].rsp = 0

stim = h.NetStim()
stim.interval = 1
stim.number = 10
stim.start = 1

netcons = [h.NetCon(stim, syn, 0, 1, 0.1) for syn in syns]

h.load_file("expsynspine.ses")
