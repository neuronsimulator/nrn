from neuron import h
from neuron.expect_hocerr import expect_err

expect_err("h.A_sdata(5,6,7)")
