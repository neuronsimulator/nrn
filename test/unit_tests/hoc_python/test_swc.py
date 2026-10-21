from neuron import h


def _count_dend_sections(swc_text, tmp_path):
    """Instantiate an SWC morphology and return the number of dendrite sections."""
    path = tmp_path / "morph.swc"
    path.write_text(swc_text)

    h.load_file("import3d.hoc")
    reader = h.Import3d_SWC_read()
    reader.quiet = 1
    reader.input(str(path))

    i3d = h.Import3d_GUI(reader, 0)
    i3d.instantiate(None)

    n_dend = sum(1 for s in h.allsec() if "soma" not in s.name())

    # Clean up all sections so tests don't interfere with each other.
    for sec in list(h.allsec()):
        h.delete_section(sec=sec)

    return n_dend


def test_swc_soma_adjacent_branch_first_child(tmp_path):
    """Regression test: first soma child that is a branch point must not create a stub section.

    Before the fix (p > 1 in mark_branch), a spurious 1-point stub section was
    created when the first non-soma SWC point (index 1) was itself a branch point.
    All later soma-adjacent branch points (index > 1) were already handled correctly
    by the connect2prox mechanism.  The fix changes the guard to p >= 1.

    Morphology m1: soma + first dend child (ID=2) that immediately branches into
    two children (IDs 3,4) + one linear stem (IDs 5,6).
    Expected: 3 dend sections (two branches + one linear stem).
    With the old bug: 4 dend sections (stub + two branches + linear stem).
    """
    swc = """\
1 1  0  0 0 5.0 -1
2 3 10  0 0 2.7  1
3 3 20  0 0 2.7  2
4 3 20  0 0 2.7  2
5 3  5  0 0 2.7  1
6 3 10  0 0 2.7  5
"""
    assert _count_dend_sections(swc, tmp_path) == 3


def test_swc_soma_adjacent_branch_later_child(tmp_path):
    """Regression test: a later soma child that is a branch point must not create a stub section.

    This case was already handled correctly before the fix (p > 1 fires for index > 1).
    Included here to guard against regressions.

    Morphology m2: soma + one linear stem (IDs 2,3) + a second soma child (ID=4)
    that branches into two children (IDs 5,6).
    Expected: 3 dend sections (linear stem + two branches from the later soma child).
    """
    swc = """\
1 1  0  0 0 5.0 -1
2 3  5  0 0 2.7  1
3 3 10  0 0 2.7  2
4 3 10  0 0 2.7  1
5 3 20  0 0 2.7  4
6 3 20  0 0 2.7  4
"""
    assert _count_dend_sections(swc, tmp_path) == 3


def test_swc_loader(tmp_path, capsys):
    """Test for regression noted in:
    https://github.com/neuronsimulator/nrn/issues/3647
    """
    swc = """\
# index     type         X            Y            Z       radius       parent
1           1  1.049119830 -8.248093605  0.500801444  1.359369397          -1
2           1  0.945353746 -7.414265633  0.450333387  2.455268145           1
"""
    path = tmp_path / "test.swc"
    path.write_text(swc)

    h.load_file("import3d.hoc")
    reader = h.Import3d_SWC_read()
    reader.quiet = 1
    reader.input(str(path))

    captured = capsys.readouterr()
    assert not captured.out
    assert not captured.err
