from neuron import h

h("create a, b")
for sec in h.allsec():
    sec.nseg = 5
    sec.insert("t2")

for seg in h.a:
    seg.t2.r = 10 + seg.x * 9
    h.b(seg.x).t2.r = 40 + seg.x * 9
    # h.setpointer(h.b(seg.x)._ref_r_t2, 'p', seg.t2)
    # h.setpointer(seg._ref_r_t2, 'p', h.b(seg.x).t2)
    seg.t2._ref_p = h.b(seg.x)._ref_r_t2
    h.b(seg.x)._ref_p_t2 = seg.t2._ref_r


def pr(sec):
    sec.push()
    for seg in sec:
        print(
            "%s   x=%g   r=%g   t2.p=%g   p_t2=%g"
            % (sec.name(), seg.x, seg.t2.r, seg.t2.p, seg.p_t2)
        )
        h.setdata_t2(seg.x)
        print("   f()=%g" % h.f_t2())
    print("\n")
    h.pop_section()


pr(h.a)
pr(h.b)

z = h.a(0.5).t2
print(z)
print(z._ref_p)
z._ref_p[0] = 5
print(h.b(0.5).t2.r)
z.p = 10
print(h.b(0.5).t2.r)
