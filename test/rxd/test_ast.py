import pytest
import json

try:
    from nmodl import to_json
    skip = False
except:
    skip = True
from testutils import compare_data, tol


@pytest.fixture
def setup_section(neuron_instance):
    """Setup a NEURON section for reactions"""
    h, rxd, data, save_path = neuron_instance
    sec = h.Section(name="soma")
    yield (neuron_instance, sec)


@pytest.mark.skipif(skip, reason="nmodl not installed")
def test_species_ast(setup_section):
    """Test that Species correctly generates an AST"""
    (h, rxd, data, save_path), sec = setup_section
    cyt = rxd.Region([sec], name="cyt", nrn_region="i")
    ca = rxd.Species(cyt, name="ca", charge=2)
    # Convert to AST and then to JSON
    node = json.loads(to_json(ca.ast(cyt)))
    ast = {"VarName": [{"Name": [{"String": [{"name": "ca@cyt[0]"}]}]}]}
    assert node == ast


@pytest.mark.skipif(skip, reason="nmodl not installed")
def test_rate_ast(setup_section):
    """Test that Rate equations correctly generate an AST"""
    (h, rxd, data, save_path), sec = setup_section

    cyt = rxd.Region([sec], name="cyt", nrn_region="i")
    ca = rxd.Species(cyt, name="ca", charge=2)
    rate = rxd.Rate(ca, -0.1 * ca)

    # create AST and convert to JSON
    nodes, species = rate.ast()
    node = json.loads(to_json(nodes[0]))
    ast = {
        "ExpressionStatement": [
            {
                "DiffEqExpression": [
                    {
                        "BinaryExpression": [
                            {
                                "PrimeName": [
                                    {"String": [{"name": "ca@cyt[0]"}]},
                                    {"Integer": [{"name": "1"}]},
                                ]
                            },
                            {"BinaryOperator": [{"name": "="}]},
                            {
                                "BinaryExpression": [
                                    {
                                        "VarName": [
                                            {
                                                "Name": [
                                                    {"String": [{"name": "ca@cyt[0]"}]}
                                                ]
                                            }
                                        ]
                                    },
                                    {"BinaryOperator": [{"name": "*"}]},
                                    {"Double": [{"name": "-0.1"}]},
                                ]
                            },
                        ]
                    }
                ]
            }
        ]
    }

    assert ast == node
    assert ca[cyt].ast().get_node_name() in species


@pytest.mark.skipif(skip, reason="nmodl not installed")
def test_reaction_ast(setup_section):
    """Test that Reaction correctly generates an AST"""
    (h, rxd, data, save_path), sec = setup_section
    cyt = rxd.Region([sec], name="cyt", nrn_region="i")
    ca = rxd.Species(cyt, name="ca", charge=2)
    buf = rxd.Parameter(cyt, name="buf")
    cabuf = rxd.Species(cyt, name="cabuf")

    reaction = rxd.Reaction(ca + buf, cabuf, 0.1, 0.05)

    rxd._ast_config["kinetic_block"] = "mass_action"
    react, species = reaction.ast()
    node = json.loads(to_json(react[0]))
    ast = {
        "ReactionStatement": [
            {
                "WrappedExpression": [
                    {
                        "BinaryExpression": [
                            {
                                "VarName": [
                                    {"Name": [{"String": [{"name": "ca@cyt[0]"}]}]}
                                ]
                            },
                            {"BinaryOperator": [{"name": "+"}]},
                            {
                                "VarName": [
                                    {"Name": [{"String": [{"name": "buf@cyt[0]"}]}]}
                                ]
                            },
                        ]
                    }
                ]
            },
            {"ReactionOperator": [{"name": "<->"}]},
            {"VarName": [{"Name": [{"String": [{"name": "cabuf@cyt[0]"}]}]}]},
            {"Double": [{"name": "0.1"}]},
            {"Double": [{"name": "0.05"}]},
        ]
    }
    assert node == ast
    myspecies = [ca[cyt].ast(), cabuf.ast(cyt)]
    for sp in myspecies:
        assert sp.get_node_name() in species
    # should not include parameters
    assert buf[cyt].ast().get_node_name() not in species

    rxd._ast_config["kinetic_block"] = "on"
    react, species = reaction.ast()
    node = json.loads(to_json(react[0]))
    assert node == ast

    rxd._ast_config["kinetic_block"] = "off"  # default with correct volume scaling

    rates, species = reaction.ast()

    ast = {
        "ExpressionStatement": [
            {
                "DiffEqExpression": [
                    {
                        "BinaryExpression": [
                            {
                                "PrimeName": [
                                    {"String": [{"name": "ca@cyt[0]"}]},
                                    {"Integer": [{"name": "1"}]},
                                ]
                            },
                            {"BinaryOperator": [{"name": "="}]},
                            {
                                "WrappedExpression": [
                                    {
                                        "BinaryExpression": [
                                            {
                                                "BinaryExpression": [
                                                    {
                                                        "BinaryExpression": [
                                                            {
                                                                "Double": [
                                                                    {"name": "0.1"}
                                                                ]
                                                            },
                                                            {
                                                                "BinaryOperator": [
                                                                    {"name": "*"}
                                                                ]
                                                            },
                                                            {
                                                                "VarName": [
                                                                    {
                                                                        "Name": [
                                                                            {
                                                                                "String": [
                                                                                    {
                                                                                        "name": "ca@cyt[0]"
                                                                                    }
                                                                                ]
                                                                            }
                                                                        ]
                                                                    }
                                                                ]
                                                            },
                                                        ]
                                                    },
                                                    {"BinaryOperator": [{"name": "*"}]},
                                                    {
                                                        "VarName": [
                                                            {
                                                                "Name": [
                                                                    {
                                                                        "String": [
                                                                            {
                                                                                "name": "buf@cyt[0]"
                                                                            }
                                                                        ]
                                                                    }
                                                                ]
                                                            }
                                                        ]
                                                    },
                                                ]
                                            },
                                            {"BinaryOperator": [{"name": "+"}]},
                                            {
                                                "BinaryExpression": [
                                                    {"Double": [{"name": "0.05"}]},
                                                    {"BinaryOperator": [{"name": "*"}]},
                                                    {
                                                        "VarName": [
                                                            {
                                                                "Name": [
                                                                    {
                                                                        "String": [
                                                                            {
                                                                                "name": "cabuf@cyt[0]"
                                                                            }
                                                                        ]
                                                                    }
                                                                ]
                                                            }
                                                        ]
                                                    },
                                                ]
                                            },
                                        ]
                                    }
                                ]
                            },
                        ]
                    }
                ]
            }
        ]
    }
    for rate in rates:
        node = json.loads(to_json(rate))
        if node == ast:
            break
    else:
        assert False  # given ast is not in rates

@pytest.mark.skipif(skip, reason="nmodl not installed")
def test_multicompartment_reaction_ast(setup_section):
    """Test that MultiCompartmentReaction generates an AST"""
    (h, rxd, data, save_path), sec = setup_section
    cyt = rxd.Region([sec], name="cyt", nrn_region="i")
    mem = rxd.Region([sec], name="mem", geometry=rxd.ScalableBorder(1))

    er = rxd.Region([sec], name="er")
    ca = rxd.Species([cyt, er], name="ca", charge=2)

    reaction = rxd.MultiCompartmentReaction(ca[cyt], ca[er], 0.1, 0.05, membrane=mem)

    rxd._ast_config["kinetic_block"] = "mass_action"
    react, species = reaction.ast()
    node = json.loads(to_json(react))

    ast = {
        "ReactionStatement": [
            {"VarName": [{"Name": [{"String": [{"name": "ca@cyt[0]"}]}]}]},
            {"ReactionOperator": [{"name": "<->"}]},
            {"VarName": [{"Name": [{"String": [{"name": "ca@er[2]"}]}]}]},
            {"Double": [{"name": "0.1"}]},
            {"Double": [{"name": "0.05"}]},
        ]
    }

    assert node == ast
    for sp in [ca[cyt], ca[er]]:
        assert sp.ast().get_node_name() in species

    rxd._ast_config["kinetic_block"] = "off"  # defualt
    rates, species = reaction.ast()

    ast = {
        "ExpressionStatement": [
            {
                "DiffEqExpression": [
                    {
                        "BinaryExpression": [
                            {
                                "PrimeName": [
                                    {"String": [{"name": "ca@cyt[0]"}]},
                                    {"Integer": [{"name": "1"}]},
                                ]
                            },
                            {"BinaryOperator": [{"name": "="}]},
                            {
                                "WrappedExpression": [
                                    {
                                        "BinaryExpression": [
                                            {
                                                "BinaryExpression": [
                                                    {
                                                        "VarName": [
                                                            {
                                                                "Name": [
                                                                    {
                                                                        "String": [
                                                                            {
                                                                                "name": "ca@cyt[0]"
                                                                            }
                                                                        ]
                                                                    }
                                                                ]
                                                            }
                                                        ]
                                                    },
                                                    {"BinaryOperator": [{"name": "*"}]},
                                                    {"Double": [{"name": "0.1"}]},
                                                ]
                                            },
                                            {"BinaryOperator": [{"name": "+"}]},
                                            {
                                                "BinaryExpression": [
                                                    {
                                                        "VarName": [
                                                            {
                                                                "Name": [
                                                                    {
                                                                        "String": [
                                                                            {
                                                                                "name": "ca@er[2]"
                                                                            }
                                                                        ]
                                                                    }
                                                                ]
                                                            }
                                                        ]
                                                    },
                                                    {"BinaryOperator": [{"name": "*"}]},
                                                    {"Double": [{"name": "0.05"}]},
                                                ]
                                            },
                                        ]
                                    }
                                ]
                            },
                        ]
                    }
                ]
            }
        ]
    }
    for rate in rates:
        node = json.loads(to_json(rate))
        if node == ast:
            break
    else:
        assert False  # given ast is not in rates
    for sp in [ca[cyt], ca[er]]:
        assert sp.ast().get_node_name() in species
