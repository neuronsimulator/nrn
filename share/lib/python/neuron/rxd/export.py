from neuron import rxd
from neuron.rxd.rxdException import RxDException
from xml.etree import ElementTree as ET
import xml.dom.minidom


# species declaration with arguments [name, compartment, and initial amount]
class species:
    def __init__(self, name, compartment, initial_amount):
        self.name = name
        self.compartment = compartment
        self.initial_amount = str(initial_amount)


# compartment declaration with arguments [name, size]
class compartment:
    def __init__(self, name, size):
        self.name = name
        self.size = str(size)


class parameter:
    def __init__(self, name, value):
        self.name = name
        self.value = value


# reactants and products are of the form [[name,stoichiometry],...]
# reaction declaration that takes name, reversible (boolean), reactants, products, and an element tree
# representing the kinetic law
class reaction:
    def __init__(self, name, reversible, reactants, products, tree, modifiers):
        self.name = name
        self.reversible = reversible
        self.reactants = reactants
        self.modifiers = modifiers
        self.products = products
        self.kinetic_law = tree


# define units here
# units are of the form [kind, exponent, scale]
class unit_definition:
    def __init__(self, name):
        self.name = name
        self.units = []


# structure defined as the middle man between NEURON and SBML file types
# it creates an smbl file for a single section or segment
class middle_man:
    # defining an empty structure to later add more things to list
    def __init__(self, name):
        self.name = name
        # dict of species in the simulation and their corresponding compartment, indexed by id
        # id = name + compartment
        self.species = {}
        # dict of compartments
        # id = compartment + index of compartment
        self.compartments = {}
        self.num_compartments = 0
        self.reactions = []
        self.parameters = {}
        # Of the form [kind,exponent,scale]
        self.unit_defs = {}

    # The parts that are required for a species are name and compartment, and optionally initial amount
    def add_species(self, name, compartment, initial_amount=0):
        if type(name) != str or len(name) < 1:
            return 1
        elif type(compartment) != str or len(compartment) < 1:
            return 2

        temp_species = species(name, compartment, initial_amount)
        id_name = name + "_" + compartment
        self.species[id_name] = temp_species

    # add compartment to structure
    def add_compartment(self, name, size):
        if type(name) != str or len(name) < 1:
            return 1
        if size is None:
            size = 0.1
        temp_compartment = compartment(name, size)
        id_name = name + str(self.num_compartments)
        self.compartments[id_name] = temp_compartment
        self.num_compartments += 1
        return 0

    def add_parameter(self, name, value):
        self.parameters[name] = parameter(name, value)

    # add reaction to structure
    def add_reaction(
        self, name, reversible, reactants, products, kinetic_law, modifiers
    ):
        self.reactions.append(
            reaction(name, reversible, reactants, products, kinetic_law, modifiers)
        )

    # add a unit to be defined later
    def add_unit_def(self, name):
        temp_unitdef = unit_definition(name)
        self.unit_defs[name] = temp_unitdef

    # add a unit to an already defined unit
    def add_unit(self, unit_def, kind, exponent, scale):
        self.unit_defs[unit_def].units.append([kind, exponent, scale])

    # returns the SBML as an element tree
    def create_xml(self):
        sbml = ET.Element(
            "sbml",
            level="2",
            version="3",
            xmlns="http://www.sbml.org/sbml/level2/version3",
        )
        model = ET.SubElement(sbml, "model", name=self.name)
        if len(self.unit_defs) > 0:
            unitdefinitions = ET.SubElement(model, "listOfUnitDefinitions")
        for key, value in self.unit_defs.items():
            unitDefinition = ET.SubElement(
                unitdefinitions, "unitDefinition", id=value.name, name=value.name
            )
            listOfUnits = ET.SubElement(unitDefinition, "listOfUnits")
            for j in value.units:
                ET.SubElement(
                    listOfUnits, "unit", kind=j[0], exponent=str(j[1]), scale=str(j[2])
                )

        if len(self.compartments) > 0:
            listOfCompartments = ET.SubElement(model, "listOfCompartments")
        for key, value in self.compartments.items():
            ET.SubElement(
                listOfCompartments,
                "compartment",
                id=value.name,
                name=value.name,
                size=str(value.size),
            )

        if len(self.species) > 0:
            listOfSpecies = ET.SubElement(model, "listOfSpecies")
        for key, value in self.species.items():
            ET.SubElement(
                listOfSpecies,
                "species",
                compartment=value.compartment,
                id=key,
                name=value.name,
                initialConcentration=value.initial_amount,
            )

        if len(self.parameters) > 0:
            listOfParameters = ET.SubElement(model, "listOfParameters")
        for para, value in self.parameters.items():
            ET.SubElement(
                listOfParameters, "parameter", id=para, value=str(value.value)
            )

        if len(self.reactions) > 0:
            listOfReactions = ET.SubElement(model, "listOfReactions")
        for react in self.reactions:
            current_reaction = ET.SubElement(
                listOfReactions,
                "reaction",
                id=react.name,
                name=react.name,
                reversible=str(react.reversible).lower(),
            )
            if len(react.reactants) > 0:
                listOfReactants = ET.SubElement(current_reaction, "listOfReactants")
            for i in react.reactants:
                ET.SubElement(
                    listOfReactants,
                    "speciesReference",
                    species=i[0],
                    stoichiometry=str(i[1]),
                )

            if len(react.products) > 0:
                listOfProducts = ET.SubElement(current_reaction, "listOfProducts")
            for i in react.products:
                ET.SubElement(
                    listOfProducts,
                    "speciesReference",
                    species=i[0],
                    stoichiometry=str(i[1]),
                )

            if len(react.modifiers) > 0:
                listOfModifiers = ET.SubElement(current_reaction, "listOfModifiers")
            for i in react.modifiers:
                ET.SubElement(listOfModifiers, "modifierSpeciesReference", species=i)

            current_reaction.append(react.kinetic_law)

        return sbml

    # #This can be changed later to no longer pretty print
    # def write_to_file(self,file_name,pretty_print = True):
    # 	# sbml = self.create_xml()
    # 	# et = ET.tostring(sbml,encoding='utf-8')
    # 	# dom = xml.dom.minidom.parseString(et)
    # 	# dom_pretty = dom.toprettyxml(encoding='utf-8')
    # 	with open(file_name,"w") as f:
    # 		if pretty_print:
    # 			f.write(self.dump(pretty_print = pretty_print))
    # 		else:
    # 			f.write('<?xml version="1.0" encoding="utf-8"?>')
    # 			f.write(ET.tostring(self.create_xml()).decode('utf-8'))

    # print out xml file
    def dump(self, pretty_print=True):
        sbml = self.create_xml()
        et = ET.tostring(sbml)
        dom = xml.dom.minidom.parseString(et)
        dom_pretty = dom.toprettyxml(encoding="utf-8").decode("utf-8")
        if pretty_print:
            return dom_pretty
        else:
            return '<?xml version="1.0" encoding="utf-8"?>' + et.decode("utf-8")


def sbml(segment, filename=None, model_name=None, pretty=True):
    rxd.initializer._do_init()
    section = segment.sec
    if model_name is not None:
        output = middle_man(model_name)
    else:
        output = middle_man(section.name())
    regions = section.psection()["regions"]
    species = section.psection()["species"]

    for i in regions:
        node = None
        for j in species:
            if i in j.regions or j.regions == [None]:
                node = j.nodes(segment)[0]
        output.add_compartment(i.name, size=node.volume)

    for i in species:
        if isinstance(i, rxd.Parameter):
            node = i.nodes(segment)[0]
            tempInitial = i.initial
            if i.initial is None:
                tempInitial = 0.0
            elif not isinstance(tempInitial, float) and not isinstance(
                tempInitial, int
            ):
                tempInitial = i.initial(node)
            output.add_parameter(i.name, i.initial)
            continue
        for j in i.regions:
            node = i.nodes(segment)[0]
            tempInitial = i.initial
            if i.initial is None:
                tempInitial = 0.0
            elif not isinstance(tempInitial, float) and not isinstance(
                tempInitial, int
            ):
                tempInitial = i.initial(node)
            output.add_species(
                name=i.name, compartment=j.name, initial_amount=tempInitial
            )

    for i in rxd.rxd._all_reactions:
        if isinstance(i, rxd.MultiCompartmentReaction):
            raise RxDException("Cannot export MultiCompartmentReactions")

    # MIGHT BE A PROBLEM WHEN MULTI COMPARTMENT REACTIONS
    reactions = [
        r()
        for r in rxd.rxd._all_reactions
        if (
            any(
                [
                    ((reg in r()._active_regions) or (reg is None))
                    for reg in r()._regions
                ]
            )
        )
        and isinstance(r(), rxd.Reaction)
    ]

    rates = [
        r()
        for r in rxd.rxd._all_reactions
        if (
            any(
                [
                    ((reg in r()._active_regions) or (reg is None))
                    for reg in r()._regions
                ]
            )
        )
        and isinstance(r(), rxd.Rate)
    ]

    num = 0
    for react in reactions:
        lhs = react._scheme._lhs._items
        rhs = react._scheme._rhs._items
        reacting_regions = []
        if react._regions == [None]:
            reacting_regions = regions
        else:
            reacting_regions = react._regions
        for reg in reacting_regions:
            reactants = [
                [f"{key.name}_{reg.name}", value]
                for key, value in zip(list(lhs.keys()), list(lhs.values()))
            ]
            products = [
                [f"{key.name}_{reg.name}", value]
                for key, value in zip(list(rhs.keys()), list(rhs.values()))
            ]

            kineticLaw = ET.Element("kineticLaw")
            math = ET.SubElement(
                kineticLaw, "math", xmlns="http://www.w3.org/1998/Math/MathML"
            )

            # adds the region to multiply it by
            region_multiple = ET.SubElement(math, "apply")
            ET.SubElement(region_multiple, "times")
            ET.SubElement(region_multiple, "ci").text = reg.name

            parameters_in_react = []

            recursive_search(
                react._rate_arithmeticed, region_multiple, parameters_in_react, reg.name
            )
            listOfParameters = ET.Element("listOfParameters")

            for i in parameters_in_react:
                if i[0] in output.parameters:
                    continue
                ET.SubElement(
                    listOfParameters, "parameter", id=i[0], name=i[1], value=str(i[2])
                )

            if list(listOfParameters):
                kineticLaw.append(listOfParameters)
            output.add_reaction(
                "Reaction_" + str(num),
                reversible=(react._dir == "<>"),
                products=products,
                reactants=reactants,
                kinetic_law=kineticLaw,
                modifiers=[],
            )
            num += 1

    num = 0
    for rate in rates:
        list_regions = []
        if rate._regions == [None]:
            list_regions = regions
        else:
            list_regions = rate._regions
        for r in list_regions:
            # math = ET.Element("apply")
            # ET.SubElement(math,"plus")
            # ET.SubElement(math,"cn").text = "0"
            # parameters = []
            # math_law = ex.recursive_search(rate._original_rate,math,parameters,r.name)
            # output.add_rate(rate._species().name + "_" + r.name,math)
            products = [[rate._species().name + "_" + r.name, 1]]
            reactants = []
            modifiers = [
                s().name + "_" + r.name
                for s in rate._involved_species
                if isinstance(s(), rxd.Parameter) == False
                and rate._species().name != s().name
            ]
            kineticLaw = ET.Element("kineticLaw")
            math = ET.SubElement(
                kineticLaw, "math", xmlns="http://www.w3.org/1998/Math/MathML"
            )
            region_multiple = ET.SubElement(math, "apply")
            ET.SubElement(region_multiple, "times")
            ET.SubElement(region_multiple, "ci").text = reg.name
            parameters = []
            recursive_search(rate._original_rate, region_multiple, parameters, r.name)
            output.add_reaction(
                "Rate_" + str(num),
                reversible=True,
                products=products,
                reactants=reactants,
                kinetic_law=kineticLaw,
                modifiers=modifiers,
            )
            num += 1

    output.add_unit_def("substance")
    output.add_unit("substance", "mole", 1, -3)
    output.add_unit_def("time")
    output.add_unit("time", "second", 1, -3)
    # output.add_unit_def("volume")
    # output.add_unit("volume","litre",1,1)

    final = output.dump(pretty_print=pretty)
    if filename:
        with open(filename, "w") as f:
            f.write(final)
    return final


function_names = {
    "acos": "arccos",
    "acosh": "arccosh",
    "asin": "arcsin",
    "asinh": "arcsinh",
    "atan": "arctanh",
    "atan2": "exception",  # FUNCTION 2
    "ceil": "ceiling",
    "copysign": "ERROR",
    "cos": "cos",
    "cosh": "cosh",
    "degrees": "ERROR",
    "erf": "ERROR",  # what do it mean
    "erfc": "ERROR",  # what do it mean
    "exp": "exp",
    "expm1": "exception",
    "fabs": "abs",
    "factorial": "ERROR",
    "floor": "floor",
    "fmod": "ERROR",
    "gamma": "eulergamma",
    "hypot": "exception",  # COULD IMPLEMENT FUNCTION 2
    "ldexp": "ERROR",
    "lgamma": "exception",  # what do it mean
    "log": "ln",
    "log10": "log",
    "log1p": "exception",
    "pow": "power",  # FUNCTION 2
    "radians": "ERROR",
    "sin": "sin",
    "sinh": "sinh",
    "sqrt": "exception",
    "tan": "tan",
    "tanh": "tanh",
    "trunc": "ERROR",
    "-": "exception",
}


def recursive_search(arth_obj, kineticLaw, parameters, compartment_name):
    items = []
    counts = []
    node = ET.SubElement(kineticLaw, "apply")

    for item, count in zip(
        list(arth_obj._items.keys()), list(arth_obj._items.values())
    ):
        if count:
            if isinstance(item, rxd.Parameter):
                parameters.append([item.name, item.name, item.initial])
                # ET.SubElement(parameters,"parameter", id=item.name,name=item.name,value=str(item.initial))
                temp = ET.Element("ci")
                temp.text = item.name
                items.append(temp)
                counts.append(count)
            elif isinstance(item, rxd.species._SpeciesMathable):
                temp = ET.Element("ci")
                if compartment_name == "":
                    temp.text = item.name
                else:
                    temp.text = f"{item.name}_{compartment_name}"
                items.append(temp)
                counts.append(count)
            elif isinstance(item, int):
                temp = ET.Element("cn")
                temp.text = str(item)
                items.append(temp)
                counts.append(count)
            else:
                recursive_node = determine_type(item, parameters, compartment_name)
                items.append(recursive_node)
                counts.append(count)

    ET.SubElement(node, "plus")

    for i, c in zip(items, counts):
        tempNode = ET.SubElement(node, "apply")
        ET.SubElement(tempNode, "times")

        tempNode.append(i)
        ET.SubElement(tempNode, "cn").text = str(c)

    if len(items) == 1:
        ET.SubElement(node, "cn").text = "0"
    return node


def determine_type(obj, parameters, comp_name):
    tree = ET.Element("apply")
    if isinstance(obj, rxd.rxdmath._Product):
        ET.SubElement(tree, "times")
        recursive_search(obj._a, tree, parameters, comp_name)  # arithmeticed
        recursive_search(obj._b, tree, parameters, comp_name)  # arithmeticed
    elif isinstance(obj, rxd.rxdmath._Quotient):
        ET.SubElement(tree, "divide")
        recursive_search(obj._a, tree, parameters, comp_name)  # arithmeticed
        recursive_search(obj._b, tree, parameters, comp_name)  # arithmeticed
    elif isinstance(obj, rxd.rxdmath._Arithmeticed):
        recursive_search(obj, tree, parameters, comp_name)
    elif isinstance(obj, rxd.rxdmath._Function):
        if function_names[obj._fname] == "exception":
            # Do Something bro
            if obj._fname == "-":
                ET.SubElement(tree, "times")
                recursive_search(obj._obj, tree, parameters, comp_name)
                ET.SubElement(tree, "cn").text = str(-1)
        elif function_names[obj._fname] == "ERROR":
            RxDException(obj._fname + " is not supported in this context")
        else:
            ET.SubElement(tree, function_names[obj._fname])
            recursive_search(obj._obj, tree, parameters, comp_name)
    elif isinstance(obj, rxd.rxdmath._Function2):
        if function_names[obj._fname] == "exception":
            if obj._fname == "atan2":
                temp = ET.SubElement(tree, function_names["atan"])
                applytemp = ET.SubElement(temp, "apply")
                recursive_search(obj._obj1, applytemp, parameters, comp_name)
                recursive_search(obj._obj2, applytemp, parameters, comp_name)
        elif function_names[obj._fname] == "ERROR":
            RxDException(obj._fname + " is not supported in this context")
        else:
            ET.SubElement(tree, function_names[obj._fname])
            recursive_search(obj._obj1, tree, parameters, comp_name)
            recursive_search(obj._obj2, tree, parameters, comp_name)

    return tree
