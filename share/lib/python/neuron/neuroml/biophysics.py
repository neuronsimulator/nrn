from .xml2nrn import *


def group(self, node):
    groupName = str(node.text.strip())
    self.biomechs_[-1].parms_[-1].group_index_ = self.groupname2index_[groupName]
    if debug:
        print("Content: ", groupName, " relevant to biogroup: ", self.biomechs_[-1])


def mechanism(self, node):
    name = str(node.get("name"))
    pc = node.get("passiveConductance")
    if pc is not None:
        if pc == "true" or pc == "1":
            print(
                "Substituting passive conductance",
                name,
                " in file for inbuilt mechanism pas as attribute passiveConductance = true ",
            )
            name = "pas"
    self.biomechs_.append(BioMech(name))


def mechanism_end(self, node):
    m = self.biomechs_[-1]
    for p in m.parms_:
        cg = self.cablegroups_[p.group_index_]
        if len(cg.mechs_) == 0 or cg.mechs_[-1].name_ != m.name_:
            cg.mechs_.append(BioMech(m.name_))
        cg.mechs_[-1].parms_.append(p)


def parameter(self, node):
    value = float(node.get("value"))
    name = node.get("name")
    if name is not None:
        # convert from physiological to NEURON units
        if self.is_physiological_units_:
            if name == "gmax":
                value *= 0.001
        # NEURON ModelView converts g_pas to gmax.
        # following does only that one
        name = name.encode("ascii") + "_" + self.biomechs_[-1].name_
        if name == "gmax_pas":
            name = "g_pas"
        self.biomechs_[-1].parms_.append(BioParm(name, value))
    else:
        if self.biomechs_[-1].name_ == "Ra":
            value *= 1000.0
        self.biomechs_[-1].parms_.append(BioParm(self.biomechs_[-1].name_, value))


def spec_axial_resistance(self, node):
    self.biomechs_.append(BioMech("Ra"))


def spec_capacitance(self, node):
    self.biomechs_.append(BioMech("cm"))
