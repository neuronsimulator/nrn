def test_species_del(neuron_instance):
    """Test deleting an uninitialized species does not raise any error or
    exceptions.
    """

    h, rxd, data, save_path = neuron_instance
    soma = h.Section(name="soma")
    cyt = rxd.Region([soma])
    c = rxd.Species(cyt)
    try:
        c.__del__()
    except:
        assert False
