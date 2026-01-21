from neuron import h, gui


def test_mview():
    """
    see PR #3075
    Don't look for KSChan channel __doc__ string with ModelView.
    """

    h.load_file(h.neuronhome() + "/demo/dynclamp.hoc")
    h.load_file("mview.hoc")
    h.mview()


if __name__ == "__main__":
    test_mview()
