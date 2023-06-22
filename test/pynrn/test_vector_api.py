import sys
import numpy as np
from neuron import h, hoc, numpy_element_ref as npyref


def copy(src, result, *args, dest=None):
    if dest is None:
        dest = h.Vector()
    dest.copy(src, *args)
    assert dest.to_python() == result


def vwrite_type(src, vtype):
    f = h.File()
    fname = "vwrite.{}.tmp".format(str(vtype))
    f.wopen(fname)
    src.c().vwrite(f, vtype)
    f.close()
    f.ropen(fname)
    vr = h.Vector(vtype)
    vr.vread(f)
    f.close()
    f.unlink()
    assert src.to_python() == vr.to_python()


def test_vector_api():
    """
    Construction
    """
    vi = h.Vector((1, 2, 3))
    assert vi.to_python() == [1.0, 2.0, 3.0]
    assert vi.get(1) == 2.0
    vi.set(1, 2.1)
    assert vi.to_python() == [1.0, 2.1, 3.0]
    del vi

    v = h.Vector(np.array([5, 1, 6], "d"))
    assert v.to_python() == [5.0, 1.0, 6.0]
    v.clear()
    assert v.size() == 0
    del v

    v = h.Vector(3)
    assert v.to_python() == [0.0, 0.0, 0.0]
    del v

    v = h.Vector(3, 1)
    assert v.to_python() == [1.0, 1.0, 1.0]
    del v

    assert h.Vector().from_python((1, 2, 3)).to_python() == [1.0, 2.0, 3.0]

    v = h.Vector()
    v.append(3, 3)
    v.append(2)
    v.append(1)
    v.append(5)

    v3 = h.Vector([4, 2, 61, 17, 13])

    """
    Vector size & capacity
    """
    assert v.to_python() == [3.0, 3.0, 2.0, 1.0, 5.0]
    assert v.size() == 5
    v.buffer_size(v.size())
    assert v.buffer_size() >= v.size()
    v.buffer_size(6)
    assert v.buffer_size() >= 6
    assert v.to_python() == [3.0, 3.0, 2.0, 1.0, 5.0]
    assert v.eq(v.c())

    """
    Calculations
    """
    assert v.median() == 3.0
    assert v.mean() == 2.8
    assert v.mean(1, 3) == 2.0
    assert np.allclose(v.stdev(), 1.4832396974191326)
    assert np.allclose(v.stdev(0, 3), 0.9574271077563381)
    assert np.allclose(v.stderr(), 0.6633249580710799)
    assert np.allclose(v.stderr(1, 3), 0.5773502691896258)
    assert v.sum() == 14.0
    assert v.sum(1, 3) == 6.0
    assert np.allclose(
        v.sumgauss(-1, 1, 0.5, 1).to_python(),
        [
            0.05869048145253869,
            0.14879136924715222,
            0.30482687457572216,
            0.5166555071584352,
        ],
    )
    assert np.allclose(
        v.sumgauss(-1, 1, 0.5, 1, h.Vector((1, 3, 2, 5, 4))).to_python(),
        [
            0.2793538745964073,
            0.6861357408871805,
            1.3355688961479038,
            2.0895389620919826,
        ],
    )
    assert np.allclose(
        v.cl().smhist(v.cl(), 1, 3, 2, 1).to_python(),
        [0.9060003240550064, 0.9598574603424295, 0.5071918738793386],
    )
    assert np.allclose(
        v.cl().smhist(v.cl(), 1, 3, 2, 1, h.Vector((1, 3, 2, 5, 4))).to_python(),
        [3.009095149765841, 2.1896697532507994, 1.8126697992388372],
    )
    assert v.sumsq() == 48.0
    assert v.sumsq(2, 4) == 30.0
    assert v.var() == 2.2
    assert v.var(2, 3) == 0.5
    assert v.min() == 1.0
    assert v.min(0, 2) == 2.0
    assert h.Vector().min() == 0.0
    assert v.min_ind() == 3
    assert v.min_ind(0, 2) == 2
    assert h.Vector().min_ind() == -1.0
    assert v.max() == 5.0
    assert v.max(0, 2) == 3.0
    assert h.Vector().max() == 0.0
    assert v.max_ind() == 4
    assert v.max_ind(0, 2) == 0
    assert v3.max_ind(2, 4) == 2
    assert v3.max_ind(1, 2) == 2
    assert v3.max_ind(3, 4) == 3
    assert h.Vector().max_ind() == -1.0
    assert v.dot(h.Vector((1, 2, 3, 4, 5))) == 44.0
    assert np.allclose(v.mag(), 6.928203230275509)
    assert v.c().reverse().meansqerr(v) == 3.2
    assert v.c().reverse().meansqerr(v, h.Vector((1, 2, 5, 4, 3))) == 8.0
    assert (
        v.c().trigavg(h.Vector((1, 2, 3, 4, 5)), h.Vector((1, 2, 3, 4, 5)), 1, 2) == 2
    )

    """
    Copying
    """
    # vdest.copy(vsrc)
    copy(v, [3.0, 3.0, 2.0, 1.0, 5.0])
    # vdest.copy(vsrc, dest_start)
    copy(v, [0.0, 0.0, 3.0, 3.0, 2.0, 1.0, 5.0], 2)
    # vdest.copy(vsrc, src_start, src_end)
    copy(v, [2.0, 1.0], 2, 3)
    copy(v, [2.0, 1.0, 5.0], 2, -1)  # -1 for actual end
    # vdest.copy(vsrc, dest_start, src_start, src_end)
    copy(v, [0.0, 2.0, 1.0], 1, 2, 3)
    # vdest.copy(vsrc, dest_start, src_start, src_end, dest_inc, src_inc)
    copy(v, [0.0, 3.0, 1.0], 1, 1, -1, 1, 2)
    # vdest.copy(vsrc, vsrcdestindex)
    copy(v, [0.0, 3.0, 2.0], h.Vector((1, 2)), dest=h.Vector(3, 0.0))
    # vdest.copy(vsrc, vsrcindex, vdestindex)copy(v, [3.0, 3.0, 2.0, 1.0, 5.0])
    copy(v, [3.0, 2.0, 0.0], h.Vector((1, 2)), h.Vector((0, 1)), dest=h.Vector(3, 0.0))
    copy(v, [3.0], h.Vector((1, 2)), h.Vector((0, 1)), dest=h.Vector(1, 0.0))
    assert v.c().to_python() == v.to_python()
    assert v.c(1).to_python() == [3.0, 2.0, 1.0, 5.0]
    assert v.c(1, 3).to_python() == [3.0, 2.0, 1.0]
    assert v.at().to_python() == v.to_python()
    assert v.at(1).to_python() == [3.0, 2.0, 1.0, 5.0]
    assert v.at(1, 3).to_python() == [3.0, 2.0, 1.0]

    """
    Data morphing and operations
    """
    assert v.resize(4).size() == 4
    assert v.fill(1.0).to_python() == [1.0, 1.0, 1.0, 1.0]
    assert v.fill(1.1, 1, 2).to_python() == [1.0, 1.1, 1.1, 1.0]
    # obj = vsrcdest.indgen()
    assert v.indgen().to_python() == [0.0, 1.0, 2.0, 3.0]
    # obj = vsrcdest.indgen(stepsize)
    assert v.indgen(2).to_python() == [0.0, 2.0, 4.0, 6.0]
    # obj = vsrcdest.indgen(start,stepsize)
    assert v.indgen(2, 5).to_python() == [2.0, 7.0, 12.0, 17.0]
    # obj = vsrcdest.indgen(start,stop,stepsize)
    assert v.indgen(1, 20, 5).to_python() == [1.0, 6.0, 11.0, 16.0]
    assert v.append(h.Vector(1, 17.0), 18.0, 19.0).to_python() == [
        1.0,
        6.0,
        11.0,
        16.0,
        17.0,
        18.0,
        19.0,
    ]
    assert v.insrt(1, 3.0).to_python() == [1.0, 3.0, 6.0, 11.0, 16.0, 17.0, 18.0, 19.0]
    assert v.insrt(3, h.Vector(1, 9.0)).to_python() == [
        1.0,
        3.0,
        6.0,
        9.0,
        11.0,
        16.0,
        17.0,
        18.0,
        19.0,
    ]
    assert v.remove(4).to_python() == [1.0, 3.0, 6.0, 9.0, 16.0, 17.0, 18.0, 19.0]
    assert v.remove(1, 5).to_python() == [1.0, 18.0, 19.0]
    h("double px[3]")
    h.px[0] = 5
    h.px[2] = 2
    assert v.from_double(3, h._ref_px[0]).to_python() == [5.0, 0.0, 2.0]
    a = np.array([5, 1, 6], "d")
    assert v.from_double(3, npyref(a, 0)).to_python() == [5.0, 1.0, 6.0]
    v.indgen(1, 30, 5)
    assert v.to_python() == [1.0, 6.0, 11.0, 16.0, 21.0, 26.0]
    assert v.contains(6.0)
    assert not v.contains(7.0)
    assert h.Vector().where(v, ">=", 10).to_python() == [11.0, 16.0, 21.0, 26.0]
    assert h.Vector().where(v, "<=", 11).to_python() == [1.0, 6.0, 11.0]
    assert h.Vector().where(v, "!=", 11).to_python() == [1.0, 6.0, 16.0, 21.0, 26.0]
    assert h.Vector().where(v, "==", 11).to_python() == [11.0]
    assert h.Vector().where(v, "<", 11).to_python() == [1.0, 6.0]
    assert h.Vector().where(v, ">", 11).to_python() == [16.0, 21.0, 26.0]
    assert h.Vector().where(v, "[)", 9, 21).to_python() == [11.0, 16.0]
    assert h.Vector().where(v, "[]", 9, 21).to_python() == [11.0, 16.0, 21.0]
    assert h.Vector().where(v, "(]", 11, 21).to_python() == [16.0, 21.0]
    assert h.Vector().where(v, "()", 11, 21).to_python() == [16.0]
    assert v.where(">", 1.0).to_python() == [6.0, 11.0, 16.0, 21.0, 26.0]
    assert v.where("[)", 6.0, 26.0).to_python() == [6.0, 11.0, 16.0, 21.0]
    assert v.indwhere(">", 11.0) == 2
    assert v.indwhere("<", 11.0) == 0
    assert v.indwhere("!=", 11.0) == 0
    assert v.indwhere(">=", 11.0) == 1
    assert v.indwhere("<=", 11.0) == 0
    assert v.indwhere("[)", 11.1, 16.0) == -1
    assert v.indwhere("[)", 11.0, 16.0) == 1
    assert v.indwhere("(]", 11.0, 16.0) == 2
    assert v.indwhere("[]", 11.0, 16.0) == 1
    assert v.indwhere("()", 16.0, 11.0) == -1
    assert h.Vector().indvwhere(v, "()", 11, 21).to_python() == [2.0]
    assert h.Vector().indvwhere(v, "==", 11).to_python() == [1.0]
    assert h.Vector().indvwhere(v, "[]", 1, 17).to_python() == [0.0, 1.0, 2.0]
    assert h.Vector().indvwhere(v, "(]", 1, 16).to_python() == [0.0, 1.0, 2.0]
    assert h.Vector().indvwhere(v, "[)", 1, 16).to_python() == [0.0, 1.0]
    assert h.Vector().indvwhere(v, "!=", 11).to_python() == [0.0, 2.0, 3.0]
    assert h.Vector().indvwhere(v, "<", 11).to_python() == [0.0]
    assert h.Vector().indvwhere(v, "<=", 11).to_python() == [0.0, 1.0]
    assert h.Vector().indvwhere(v, ">", 16).to_python() == [3.0]
    assert h.Vector().indvwhere(v, ">=", 16).to_python() == [2.0, 3.0]

    assert v.histogram(1.0, 20.0, 10).to_python() == [0.0, 1.0, 2.0]
    assert h.Vector().hist(v, 1.0, 2.0, 10).to_python() == [1.0, 2.0]
    assert v.ind(h.Vector((1, 3))).to_python() == [11.0, 21.0]
    assert h.Vector().spikebin(v.c(), 12.0).to_python() == [0.0, 0.0, 1.0, 0.0]

    """
    Vector metadata
    """
    assert v.label() == ""
    v.label("v")
    assert v.label() == "v"
    assert v.cl().label() == "v"
    v.label("v2")
    assert v.label() == "v2"

    """
    Transformations
    """
    v = h.Vector((3, 2, 15, 16))
    assert np.allclose(
        v.c().apply("sin").to_python(),
        [
            0.1411200080598672,
            0.9092974268256817,
            0.6502878401571169,
            -0.2879033166650653,
        ],
    )
    assert np.allclose(
        v.c().apply("sin", 1, 2).to_python(),
        [3.0, 0.9092974268256817, 0.6502878401571169, 16.0],
    )
    h("func sq(){return $1*$1}")
    assert np.allclose(v.c().apply("sq").to_python(), [9.0, 4.0, 225.0, 256.0])
    assert v.reduce("sq", 100) == 594.0
    assert h.Vector().deriv(v, 0.1).to_python() == [-10.0, 60.0, 70.0, 10.0]
    assert h.Vector().deriv(v, 1, 1).to_python() == [-1.0, 13.0, 1.0]
    assert h.Vector().deriv(v, 1, 2).to_python() == [-1.0, 6.0, 7.0, 1.0]
    assert np.allclose(
        v.c().interpolate(v.c(), v.c().apply("sqrt")).to_python(),
        [10.384365150689874, 5.097168242109362, 16.0, 16.0],
    )
    assert np.allclose(
        v.c()
        .interpolate(v.c(), v.c().apply("sqrt"), h.Vector((1, 2, 3, 4)))
        .to_python(),
        [2.644951165437683, 2.2382437109314894, 4.0, 4.0],
    )
    assert h.Vector().integral(v).to_python() == [3.0, 5.0, 20.0, 36.0]
    assert np.allclose(
        h.Vector().integral(v, 0.1).to_python(), [3.0, 3.2, 4.7, 6.300000000000001]
    )
    assert v.c().medfltr().to_python() == [3.0, 3.0, 3.0, 3.0]
    assert v.c().medfltr(h.Vector((1, 2, 3, 4))).to_python() == [2.0, 2.0, 2.0, 2.0]
    assert v.c().sort().to_python() == [2.0, 3.0, 15.0, 16.0]
    assert v.sortindex().to_python() == [1.0, 0.0, 2.0, 3.0]
    assert v.c().reverse().to_python() == [16.0, 15.0, 2.0, 3.0]
    assert v.c().rotate(3).to_python() == [2.0, 15.0, 16.0, 3.0]
    assert v.c().rotate(3, 0).to_python() == [0.0, 0.0, 0.0, 3.0]
    assert h.Vector().rebin(v, 2).to_python() == [5.0, 31.0]
    assert v.c().rebin(2).to_python() == [5.0, 31.0]
    assert h.Vector().pow(v, 2).to_python() == [9.0, 4.0, 225.0, 256.0]
    assert np.allclose(v.c().pow(v, 0).to_python(), [1.0, 1.0, 1.0, 1.0])
    assert np.allclose(
        v.c().pow(v, 0.5).to_python(),
        [1.7320508075688772, 1.4142135623730951, 3.872983346207417, 4.0],
    )
    assert np.allclose(
        v.c().pow(v, -1).to_python(),
        [0.3333333333333333, 0.5, 0.06666666666666667, 0.0625],
    )
    assert np.allclose(v.c().pow(v, 1).to_python(), [3.0, 2.0, 15.0, 16.0])
    assert np.allclose(v.c().pow(v, 3).to_python(), [27.0, 8.0, 3375.0, 4096.0])
    assert v.c().pow(2).to_python() == [9.0, 4.0, 225.0, 256.0]
    assert np.allclose(
        h.Vector().sqrt(v).to_python(),
        [1.7320508075688772, 1.4142135623730951, 3.872983346207417, 4.0],
    )
    assert np.allclose(
        v.c().sqrt().to_python(),
        [1.7320508075688772, 1.4142135623730951, 3.872983346207417, 4.0],
    )
    assert np.allclose(
        h.Vector().log(v).to_python(),
        [1.0986122886681098, 0.6931471805599453, 2.70805020110221, 2.772588722239781],
    )
    assert np.allclose(
        v.c().log().to_python(),
        [1.0986122886681098, 0.6931471805599453, 2.70805020110221, 2.772588722239781],
    )
    assert np.allclose(
        h.Vector().log10(v).to_python(),
        [
            0.47712125471966244,
            0.3010299956639812,
            1.1760912590556813,
            1.2041199826559248,
        ],
    )
    assert np.allclose(
        v.c().log10().to_python(),
        [
            0.47712125471966244,
            0.3010299956639812,
            1.1760912590556813,
            1.2041199826559248,
        ],
    )
    assert np.allclose(
        h.Vector().tanh(v).to_python(),
        [
            0.9950547536867305,
            0.9640275800758169,
            0.9999999999998128,
            0.9999999999999747,
        ],
    )
    assert np.allclose(
        v.c().tanh().to_python(),
        [
            0.9950547536867305,
            0.9640275800758169,
            0.9999999999998128,
            0.9999999999999747,
        ],
    )
    assert h.Vector([1.2312414, 3.1231, 5.49554, 6.5000000001]).floor().to_python() == [
        1.0,
        3.0,
        5.0,
        6.0,
    ]
    assert h.Vector([-1.0, -3.0, -5.0, -6.0]).abs().to_python() == [1.0, 3.0, 5.0, 6.0]
    assert v.c().add(h.Vector((1.1, 2.2, 3.3, 4.4))).to_python() == [
        4.1,
        4.2,
        18.3,
        20.4,
    ]
    assert v.c().add(1.3).to_python() == [4.3, 3.3, 16.3, 17.3]
    assert v.c().sub(h.Vector((1.1, 2, 3.3, 4))).to_python() == [1.9, 0.0, 11.7, 12.0]
    assert v.c().sub(1.3).to_python() == [1.7, 0.7, 13.7, 14.7]
    assert v.c().mul(h.Vector((1.5, 2, 3, 4))).to_python() == [4.5, 4.0, 45.0, 64.0]
    assert v.c().mul(2.5).to_python() == [7.5, 5.0, 37.5, 40.0]
    assert v.c().div(h.Vector((1.5, 2, 3, 4))).to_python() == [2.0, 1.0, 5.0, 4.0]
    assert v.c().div(2.5).to_python() == [1.2, 0.8, 6.0, 6.4]
    vs = v.c()
    assert np.allclose(vs.scale(2, 5), 0.21428571428571427)
    assert np.allclose(
        vs.to_python(), [2.2142857142857144, 2.0, 4.785714285714286, 5.0]
    )
    assert np.allclose(
        v.c().sin(1, 1).to_python(),
        [0.8414709848078965, 0.844849172063764, 0.8481940061209319, 0.8515053549310787],
    )
    assert np.allclose(
        v.c().sin(1, 1, 2).to_python(),
        [
            0.8414709848078965,
            0.8481940061209319,
            0.8547830877678237,
            0.8612371892561972,
        ],
    )

    """
    Fourier
    """
    assert v.to_python() == [3.0, 2.0, 15.0, 16.0]
    assert h.Vector(v.size()).correl(v).to_python() == [494.0, 324.0, 154.0, 324.0]
    assert h.Vector(v.size()).convlv(v, v.c().reverse()).to_python() == [
        294.0,
        122.0,
        318.0,
        490.0,
    ]
    assert np.allclose(
        h.Vector(v.size()).convlv(v, v.c().reverse(), -1).to_python(),
        [305.9999866504336, 306.0, 306.0000133495664, 306.0],
    )
    assert v.c().spctrm(v).to_python() == [60.625, 2.0, 15.0, 16.0]
    assert h.Vector(v.size()).filter(v, v.c().reverse()).to_python() == [
        308.0,
        -66.0,
        376.0,
        750.0,
    ]
    assert h.Vector(v.size()).fft(v, -1).to_python() == [17.5, 16.5, -12.5, -15.5]
    assert v.c().fft(-1).to_python() == h.Vector(v.size()).fft(v, -1).to_python()

    """
    I/O
    """
    assert v.to_python() == [3.0, 2.0, 15.0, 16.0]
    f = h.File()
    f.wopen("temp.tmp")
    v.vwrite(f)
    f.close()
    assert v.to_python() == [3.0, 2.0, 15.0, 16.0]

    vr = h.Vector()
    f.ropen("temp.tmp")
    vr.vread(f)
    assert vr.to_python() == v.to_python()
    f.close()
    f.unlink()

    f.wopen("temp.tmp")
    f.printf("%d %d %d %d\n", 3, 2, 15, 16)
    f.close()
    f.ropen("temp.tmp")

    vr.resize(0)
    vr.scanf(f)
    assert vr.to_python() == v.to_python()
    f.seek(0)
    vr2 = h.Vector(4)
    vr2.scanf(f)
    assert vr.to_python() == vr2.to_python()
    f.seek(0)
    vr.resize(0)
    vr.scanf(f, 1)
    assert vr.to_python() == [3.0]
    vr.scanf(f, 1)
    assert vr.to_python() == [2.0]
    vr.resize(0)
    f.seek(0)
    vr.scantil(f, 15.0)
    assert vr.to_python() == [3.0, 2.0]
    f.close()
    f.unlink()

    # Columns
    f.wopen("col.tmp")
    f.printf("%d %d %d %d\n", 3, 2, 15, 16)
    f.printf("%d %d %d %d\n", 6, 9, 7, 21)
    f.printf("%d %d %d %d\n", 1, 4, 5, 22)
    f.printf("%d %d %d %d\n", 3, 8, 14, 23)
    f.close()
    f.ropen("col.tmp")
    vc = h.Vector()
    vc.scanf(f, 3, 2, 4)
    assert vc.to_python() == [2.0, 9.0, 4.0]
    vc.scanf(f, 3, 2, 4)
    assert vc.to_python() == [8.0]
    f.close()
    f.ropen("col.tmp")
    vc = h.Vector()
    vc.scanf(f, 3, 4)
    assert vc.to_python() == [15.0, 7.0, 5.0, 14.0]
    f.seek(0)
    vc.scantil(f, 5.0, 3, 4)
    assert vc.to_python() == [15.0, 7.0]
    vc.printf()  # code cov
    vc.printf("%8.4f ")
    vc.printf("%8.4f ", 0, 1)
    f.close()
    f.unlink()

    # Vwrite types
    vwrite_type(h.Vector([1, 2, 3, 4]), 1)
    vwrite_type(h.Vector([4, 3, 2, 1]), 2)
    vwrite_type(h.Vector([4, 5, 6, 7]), 3)
    vwrite_type(h.Vector([7, 8, 9, 10]), 4)
    vwrite_type(h.Vector([0, 1, 2, 33]), 5)

    """
    Random 
    """
    vrand = h.Vector((1, 2, 3))
    r = h.Random()
    r.poisson(12)
    assert vrand.cl().setrand(r).to_python() == [10.0, 16.0, 11.0]
    assert vrand.cl().setrand(r, 1, 2).to_python() == [1.0, 9.0, 18.0]
    assert vrand.cl().addrand(r).to_python() == [9.0, 9.0, 16.0]
    assert vrand.cl().addrand(r, 0, 1).to_python() == [13.0, 16.0, 3.0]

    """
    Misc 
    """
    assert h.Vector().inf(h.Vector((3, 2, 4)), 2, 3, 4, 5, 6, 7).to_python() == [
        4.0,
        5.2,
        4.56,
    ]
    assert h.Vector().resample(h.Vector((3, 2, 4, 6, 7)), 2).to_python() == [
        3.0,
        3.0,
        2.0,
        2.0,
        4.0,
        4.0,
        6.0,
        6.0,
        7.0,
        7.0,
    ]
    assert h.Vector().psth(h.Vector((3, 2, 4, 6, 7, 6, 7, 8)), 1, 2, 3).to_python() == [
        1500.0,
        1500.0,
        2000.0,
        3000.0,
        3500.0,
        3000.0,
        3500.0,
        4000.0,
    ]

    h("func fun () { return ($1 - 0.5) * 2 + ($2 - 0.5) * 2 }")
    dvec = h.Vector(2)
    fvec = h.Vector(2)
    fvec.fill(1)
    ivec = h.Vector(2)
    ivec.indgen()
    a = h.ref(2)
    b = h.ref(1)
    error = dvec.fit(fvec, "fun", ivec, a, b)
    assert np.allclose([error], [1.0005759999999997])
    assert np.allclose(fvec.to_python(), [-0.976, 1.024])
    assert ivec.to_python() == [0.0, 1.0]
    assert dvec.to_python() == [0.0, 0.0]
    aftau = np.array([5, 1, 6, 8], "d")
    error = dvec.fit(
        fvec,
        "exp2",
        ivec,
        npyref(aftau, 0),
        npyref(aftau, 1),
        npyref(aftau, 2),
        npyref(aftau, 3),
    )
    assert np.allclose([error], [8.442756842706686])
    error = dvec.fit(
        fvec,
        "exp1",
        ivec,
        npyref(aftau, 0),
        npyref(aftau, 1),
        npyref(aftau, 2),
        npyref(aftau, 3),
    )
    assert np.allclose([error], [2.5653639385348724e-06])
    error = dvec.fit(
        fvec,
        "charging",
        ivec,
        npyref(aftau, 0),
        npyref(aftau, 1),
        npyref(aftau, 2),
        npyref(aftau, 3),
    )
    assert np.allclose([error], [7.288763182752445e-08])
    error = dvec.fit(
        fvec, "quad", ivec, npyref(aftau, 0), npyref(aftau, 1), npyref(aftau, 2)
    )
    assert np.allclose([error], [0.0010239221022848835])
    error = dvec.fit(fvec, "line", ivec, npyref(aftau, 0), npyref(aftau, 1))
    assert np.allclose([error], [6.593786238758728e-06])
