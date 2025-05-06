def test_species_exception(neuron_nosave_instance):
    """A test that an exception is raised when accessing a species on a region where it has not been defined"""

    h, rxd, save_path = neuron_nosave_instance
    soma = h.Section("soma")
    cyt = rxd.Region(soma.wholetree(), nrn_region="i", name="cyt")
    ecs = rxd.Extracellular(-10, -10, -10, 10, 10, 10, dx=20)
    ca = rxd.Species(cyt, name="ca", charge=2)
    k = rxd.Species(ecs, name="k", charge=1)

    try:
        species_on_region = ca[cyt]
    except:
        assert False
    try:
        species_on_region = k[cyt]
        assert False
    except rxd.RxDException as e:
        assert repr(k) in e.args[0]
        assert repr(cyt) in e.args[0]
    except:
        assert False

    try:
        species_on_ecs = k[ecs]
    except:
        assert False
    try:
        species_on_ecs = ca[ecs]
        assert False
    except rxd.RxDException as e:
        assert repr(ca) in e.args[0]
        assert repr(ecs) in e.args[0]
    except:
        assert False


def test_parameter_exception(neuron_nosave_instance):
    """A test that an exception is raised when accessing a parameter on a region where it has not been defined"""
    h, rxd, save_path = neuron_nosave_instance
    soma = h.Section("soma")
    cyt = rxd.Region(soma.wholetree(), nrn_region="i", name="cyt")
    ecs = rxd.Extracellular(-10, -10, -10, 10, 10, 10, dx=20)
    ca = rxd.Parameter(cyt, name="ca", charge=2)
    k = rxd.Parameter(ecs, name="k", charge=1)
    try:
        species_on_region = ca[cyt]
    except:
        assert False
    try:
        species_on_region = k[cyt]
        assert False
    except rxd.RxDException as e:
        assert repr(k) in e.args[0]
        assert repr(cyt) in e.args[0]
    except:
        assert False

    try:
        species_on_ecs = k[ecs]
    except:
        assert False
    try:
        species_on_ecs = ca[ecs]
        assert False
    except rxd.RxDException as e:
        assert repr(ca) in e.args[0]
        assert repr(ecs) in e.args[0]
    except:
        assert False


def test_state_exception(neuron_nosave_instance):
    """A test that an exception is raised when accessing a state on a region where it has not been defined"""
    h, rxd, save_path = neuron_nosave_instance
    soma = h.Section("soma")
    cyt = rxd.Region(soma.wholetree(), nrn_region="i", name="cyt")
    ecs = rxd.Extracellular(-10, -10, -10, 10, 10, 10, dx=20)
    ca = rxd.State(cyt, name="ca", charge=2)
    k = rxd.State(ecs, name="k", charge=1)

    try:
        species_on_region = ca[cyt]
    except:
        assert False
    try:
        species_on_region = k[cyt]
        assert False
    except rxd.RxDException as e:
        assert repr(k) in e.args[0]
        assert repr(cyt) in e.args[0]
    except:
        assert False

    try:
        species_on_ecs = k[ecs]
    except:
        assert False
    try:
        species_on_ecs = ca[ecs]
        assert False
    except rxd.RxDException as e:
        assert repr(ca) in e.args[0]
        assert repr(ecs) in e.args[0]
    except:
        assert False
