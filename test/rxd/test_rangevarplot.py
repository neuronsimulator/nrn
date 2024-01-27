import pytest
import gc


@pytest.fixture
def model_rangevarplot(neuron_instance):
    h, rxd, data, save_path = neuron_instance

    def make_rvp(with_sec=True, with_species=True):
        dend = h.Section(name="dend")
        dend.L = 3
        dend.diam = 2
        dend.nseg = 3

        def my_initial(node):
            if h.distance(node, dend(0)) < 2:
                return 1
            else:
                return 0

        cyt = rxd.Region([dend], name="cyt")
        c = rxd.Species(cyt, name="c", initial=my_initial)

        g = h.RangeVarPlot(c, dend(0), dend(1))
        c = c if with_species else None
        dend = dend if with_sec else None
        gc.collect()
        return (dend, cyt, c, g)

    yield (h, rxd, make_rvp)


def test_rangevarplot(model_rangevarplot):
    h, rxd, make_rvp = model_rangevarplot
    dend, cyt, c, g = make_rvp()
    vec = h.Vector()
    g.to_vector(vec)
    assert vec.to_python() == [1, 1, 1, 0, 0]


def test_rangevarplot_no_species(model_rangevarplot):
    h, rxd, make_rvp = model_rangevarplot
    dend, cyt, c, g = make_rvp(with_species=False)
    vec = h.Vector()
    g.to_vector(vec)
    assert vec.to_python() == [0, 0, 0, 0, 0]


def test_rangevarplot_no_section(model_rangevarplot):
    h, rxd, make_rvp = model_rangevarplot
    dend, cyt, c, g = make_rvp(with_sec=False)
    vec = h.Vector()
    g.to_vector(vec)
    for sec in h.allsec():
        assert False
    assert vec.to_python() == [0, 0, 0, 0, 0]
