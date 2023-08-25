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

# this part tests POINT_PROCESS calls using pythonic syntax.
# no need for explicit setdata call.
sds = [h.SData(seg) for seg in s]
for i, sd in enumerate(sds):
    sd.A(i, 10 * i, 100 * i)

for i, sd in enumerate(sds):
    x = float(i)
    assert sd.a == x
    assert sd.b == 10 * x
    assert sd.c[1] == 100 * x

# this part tests SUFFIX calls using pythonic syntax.
# no need for explicit setdata call.
for seg in s:
    x = seg.x
    seg.sdata.A(x + 10, x + 20, x + 30)
for seg in s:
    x = seg.x
    assert seg.sdata.a == x + 10
    assert seg.sdata.b == x + 20
    assert seg.sdata.c[1] == x + 30
