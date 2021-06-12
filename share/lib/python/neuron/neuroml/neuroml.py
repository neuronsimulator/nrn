from .xml2nrn import *


def biophysics(self, node):
    # following is worthless. need units for Ra, cm, gmax, e
    self.is_physiological_units_ = True
    val = node.get("units")
    if val == "Physiological Units":
        self.is_physiological_units_ = True


def cell(self, node):
    self.biomechs_ = []
    self.cellname = ""
    val = node.get("name")
    if val:
        self.cellname = str(val)
    if debug:
        print("cell")
        self.prattrs(node)


def cell_end(self, node):
    if debug:
        print("endcell")
        for i in range(len(self.cables_)):
            c = self.cables_[i]
            print("Cable ", i, ": ", c.id_, c.parent_cable_id_, c.name_, c.px_, c.pcnt_)

        for bm in self.biomechs_:
            print(bm)

        for cg in self.cablegroups_:
            print(cg)

        for g2i in self.groupname2index_:
            print("groupname2index: ", g2i, " - ", str(self.groupname2index_[g2i]))


def cells(self, node):
    pass


def neuroml(self, node):
    pass
