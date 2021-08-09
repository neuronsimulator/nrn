#
# See http://neuroml.svn.sourceforge.net/viewvc/neuroml/NeuroML2nrn/
# for the latest version of this file.
#

# 1 for verbose debug mode
debug = 0


class Point:
    def __init__(self, id, parent_id, cable_id, lineno):
        self.id_ = id
        self.pid_ = parent_id
        self.cid_ = cable_id
        self.lineno_ = lineno

    def set(self, x, y, z, diam):
        self.x_ = x
        self.y_ = y
        self.z_ = z
        self.d_ = diam

    def __str__(self):
        return (
            "Point: "
            + str(self.id_)
            + ", ("
            + str(self.x_)
            + ", "
            + str(self.y_)
            + ", "
            + str(self.z_)
            + " | "
            + str(self.d_)
            + ") , par: "
            + str(self.pid_)
            + ", cable id: "
            + str(self.cid_)
        )


class Cable:
    def __init__(self, id, pid, ipnt):
        self.first_ = ipnt
        self.pcnt_ = 0
        self.id_ = id
        self.pid_ = pid
        self.px_ = -1.0
        self.parent_cable_id_ = -1
        self.name_ = ""
        if debug:
            print(" New ", self.__str__())

    def __str__(self):
        return (
            "Cable: id="
            + str(self.id_)
            + " name="
            + self.name_
            + " pid="
            + str(self.parent_cable_id_)
            + " first="
            + str(self.first_)
        )


class CableGroup:
    def __init__(self, name):
        self.name_ = name
        self.cable_indices_ = []
        self.mechs_ = []

    def __str__(self):
        info = "CableGroup: " + self.name_
        for ci in self.cable_indices_:
            info = info + "\n" + "  " + str(ci)
        return info


class BioParm:
    def __init__(self, name, value):
        self.name_ = name
        self.value_ = value
        self.group_index_ = -1

    def __str__(self):
        return (
            "BioParm: "
            + self.name_
            + ", val: "
            + str(self.value_)
            + ", grp id: "
            + str(self.group_index_)
        )


class BioMech:
    def __init__(self, name):
        self.name_ = name
        self.parms_ = []

    def __str__(self):
        info = "BioMech: " + self.name_
        for p in self.parms_:
            info = info + "\n" + "  " + str(p)
        return info


class LineNo:
    def __init__(self):
        self.lineno = 0

    def getLineNumber(self):
        return self.lineno


class XML2Nrn:
    def __init__(self):
        if debug:
            print("Starting parsing of NeuroML file... ")
        self.in_biogroup_ = False
        self.in_paramgroup_ = False
        self.locator = LineNo()

    def prattrs(self, node):
        for i in list(node.keys()):
            print("  ", i, " ", node.get(i))
