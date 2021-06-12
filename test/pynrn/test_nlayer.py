from neuron import h, gui


class Model:
    def __init__(self):
        nlayer = int(h.nlayer_extracellular())

        cable = [h.Section("cable%d" % i) for i in range(4)]
        cable[1].connect(cable[0](1))
        cable[2].connect(cable[1](1))
        cable[3].connect(cable[0](0))

        for sec in cable:
            sec.nseg = 3
            sec.L = 100
            sec.diam = 1
            sec.insert("hh")
            sec.insert("extracellular")  # default, virtually wire to ground
            for i in range(nlayer):
                sec.xg[i] = 1e7  # Gaussian elimination roundoff errors in 10 ms
                sec.xraxial[i] = 1e14  # accumulate if left at default 1e9
            sec.xg[nlayer - 1] = 0.001  # all voltage drop in last layer

        cable[3](1).e_extracellular = -20  # stimulate extracellularly

        self.cable = cable

    def run(self):
        h.tstop = 10
        h.run()
        res = [h.Vector(), h.Vector()]
        for sec in self.cable:
            for seg in sec.allseg():
                res[0].append(seg.v)
                res[1].append(seg.vext[0])
        return res


def test_nlayer():
    res = []
    for nlayer in [1, 2, 3]:
        h.nlayer_extracellular(nlayer)
        assert nlayer == int(h.nlayer_extracellular())
        m = Model()
        res.append(m.run())
        del m
    for r in res[1:]:
        for i in range(2):
            x = r[i].c().sub(res[0][i]).abs().sum()
            print(x)
            assert x < 0.01  # At 10ms. At 5ms would be 2e-5
    return res


if __name__ == "__main__":
    res = test_nlayer()
    for i in range(len(res[0][0])):
        for j in range(2):
            for k in range(3):
                print("%g" % res[k][j].x[i], end=" ")
        print()
