import neuron


def test_builtin_templates():
    assert isinstance(neuron.hoc.Vector, type), "Type instance expected for hoc.Vector"
    assert isinstance(neuron.hoc.CVode, type), "Type instance expected for hoc.CVode"
    assert isinstance(neuron.hoc.List, type), "Type instance expected for hoc.List"
    assert isinstance(neuron.hoc.Deck, type), "Type instance expected for hoc.Deck"

    assert neuron.h.Vector is neuron.hoc.Vector, "Redirect to hoc.Vector failed"
    assert neuron.h.Deck is neuron.hoc.Deck, "Redirect to hoc.Deck failed"
    assert neuron.h.List is neuron.hoc.List, "Redirect to hoc.List failed"


def test_inheritance_builtin():
    v = neuron.h.Vector()
    assert isinstance(v, neuron.hoc.HocObject), "hoc.HocObject should be parent."
    assert isinstance(v, neuron.hoc.Vector), "Should be instance of its class"
    assert not isinstance(v, neuron.hoc.Deck), "Should not be instance of another class"
    assert type(v) is neuron.hoc.Vector, "Type should be class"
    assert type(v) is not neuron.hoc.Deck, "Type should not be another class"
