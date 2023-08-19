from neuron import h
from neuron.expect_hocerr import expect_err

expect_err("h.A_sdata(5,6,7)")  # No data for A_sdata.
expect_err("h.setdata_sdata()")  # not enough args
expect_err("h.setdata_sdata(0.5)")  # Section access unspecified

s = h.Section("s")
s.insert("sdata")
s.nseg = 5

for seg in s:
    h.setdata_sdata(seg)
    x = seg.x
    h.A_sdata(x, 10 * x, 100 * x)

for seg in s:
    x = seg.x
    assert seg.sdata.a == x
    assert seg.sdata.b == 10 * x
    assert seg.sdata.c[1] == 100 * x
