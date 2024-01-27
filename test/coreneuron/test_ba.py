from neuron import h, coreneuron

pc = h.ParallelContext()


def model():
    s = h.Section()
    s.insert("ba0")
    s.insert("ba1")
    return s


def run(m):
    r = []
    pc.set_maxstep(10)
    h.finitialize()
    r.append(result(m))
    pc.psolve(h.dt)
    r.append(result(m))
    return r


def result(m):
    r = {}
    seg = m(0.5)
    for mech in [seg.ba0, seg.ba1]:
        for rv in mech:
            r[rv.name()] = rv[0]
    return r


std = [
    {
        "bi_ba0": 0.0,
        "i_ba0": 1.0,
        "ai_ba0": 2.0,
        "bb_ba0": 3.0,
        "b_ba0": -1.0,
        "pval_ba0": -1.0,
        "as_ba0": -1.0,
        "bs_ba0": 4.0,
        "bit_ba0": 0.0,
        "it_ba0": 0.0,
        "ait_ba0": 0.0,
        "bbt_ba0": 0.0,
        "bt_ba0": -1.0,
        "pvalt_ba0": -1.0,
        "ast_ba0": -1.0,
        "bst_ba0": 0.0,
        "icur_ba1": 0.0,
        "bi_ba1": 0.0,
        "i_ba1": 1.0,
        "ai_ba1": 2.0,
        "bb_ba1": 3.0,
        "b_ba1": 5.0,
        "pval_ba1": -1.0,
        "as_ba1": -1.0,
        "bs_ba1": 6.0,
        "bit_ba1": 0.0,
        "it_ba1": 0.0,
        "ait_ba1": 0.0,
        "bbt_ba1": 0.0,
        "bt_ba1": 0.0,
        "pvalt_ba1": -1.0,
        "ast_ba1": -1.0,
        "bst_ba1": 0.0,
        "s_ba1": 0.0,
    },
    {
        "bi_ba0": 0.0,
        "i_ba0": 1.0,
        "ai_ba0": 2.0,
        "bb_ba0": 5.0,
        "b_ba0": 7.0,
        "pval_ba0": 6.0,
        "as_ba0": 8.0,
        "bs_ba0": 9.0,
        "bit_ba0": 0.0,
        "it_ba0": 0.0,
        "ait_ba0": 0.0,
        "bbt_ba0": 0.0125,
        "bt_ba0": 0.025,
        "pvalt_ba0": 0.025,
        "ast_ba0": 0.025,
        "bst_ba0": 0.025,
        "icur_ba1": 0.0,
        "bi_ba1": 0.0,
        "i_ba1": 1.0,
        "ai_ba1": 2.0,
        "bb_ba1": 7.0,
        "b_ba1": 9.0,
        "pval_ba1": 10.0,
        "as_ba1": 11.0,
        "bs_ba1": 12.0,
        "bit_ba1": 0.0,
        "it_ba1": 0.0,
        "ait_ba1": 0.0,
        "bbt_ba1": 0.0125,
        "bt_ba1": 0.0125,
        "pvalt_ba1": 0.025,
        "ast_ba1": 0.025,
        "bst_ba1": 0.025,
        "s_ba1": 0.25,
    },
]


# to make a new std copy output of following to std = ... above and run black.
def mk_std():
    m = model()
    r = run(m)
    print(r)


def cmp(r, s):
    if r != s:
        print("standard", s)
        print("result", r)
    assert r == s


def test_ba():
    m = model()
    r = run(m)
    cmp(r, std)

    coreneuron.enable = True
    r = run(m)
    cmp(r, std)


if __name__ == "__main__":
    test_ba()
