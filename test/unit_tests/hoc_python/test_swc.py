from neuron import h


def test_swc_loader(tmp_path, capsys):
    """Test for regression noted in:
    https://github.com/neuronsimulator/nrn/issues/%20#issuecomment-3451674614
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
