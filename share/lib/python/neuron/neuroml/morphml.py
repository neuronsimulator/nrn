from .xml2nrn import *


def cable(self, node):
    self.lastcabid_ = -1
    if self.in_cablegroup_:
        self.cablegroups_[-1].cable_indices_.append(
            self.cableid2index_[int(node.get("id"))]
        )
    else:
        cab = self.cables_[self.cableid2index_[int(node.get("id"))]]
        self.lastcabid_ = int(node.get("id"))
        val = node.get("fractAlongParent")
        if val is not None:
            cab.px_ = float(val)
        else:
            cab.px_ = 1.0
        cab.name_ = str(node.get("name"))


def cablegroup(self, node):
    self.in_cablegroup_ = True
    name = str(node.get("name"))
    self.cablegroups_.append(CableGroup(name))
    self.groupname2index_[name] = len(self.cablegroups_) - 1


def cables(self, node):
    pass


def distal(self, node):
    pt = Point(self.id, self.pid, self.cid, self.locator.getLineNumber())
    self.ptid2pt_[self.id] = pt
    pt.set(
        float(node.get("x")),
        float(node.get("y")),
        float(node.get("z")),
        float(node.get("diameter")),
    )
    if self.cable_.pcnt_ == 1:
        proxpt = self.points_[len(self.points_) - 1]
        if (
            proxpt.x_ == pt.x_
            and proxpt.y_ == pt.y_
            and proxpt.z_ == pt.z_
            and proxpt.d_ == pt.d_
        ):
            if debug:
                print("Prox and distal same, assuming spherical segment")
            pt.y_ = pt.y_ + (pt.d_ / 2.0)
            self.points_[len(self.points_) - 1].y_ = self.points_[
                len(self.points_) - 1
            ].y_ - (self.points_[len(self.points_) - 1].d_ / 2.0)
            if debug:
                print("New distal: " + str(pt))
                print("New proximal: " + str(self.points_[len(self.points_) - 1]))
    self.points_.append(pt)
    self.cable_.pcnt_ += 1
    if debug:
        print("Distal: " + str(pt))
        print("Cable ", self.cable_.id_, " has ", self.cable_.pcnt_, " points")


def proximal(self, node):
    self.nprox += 1
    pt = Point(-1, self.pid, self.cid, self.locator.getLineNumber())
    pt.set(
        float(node.get("x")),
        float(node.get("y")),
        float(node.get("z")),
        float(node.get("diameter")),
    )
    self.points_.append(pt)
    self.cable_.pcnt_ += 1
    if debug:
        print("Proximal: " + str(pt))
        print("Cable ", self.cable_.id_, " has ", self.cable_.pcnt_, " points")


def segment(self, node):
    self.id = int(node.get("id"))
    self.cid = int(node.get("cable"))
    parent_cable_id = -1
    p = node.get("parent")
    if p is not None:
        self.pid = int(p)
        parent_cable_id = self.ptid2pt_[self.pid].cid_
    else:
        self.pid = -1

    if debug:
        print(
            "\nsegment id=",
            self.id,
            "  cable=",
            self.cid,
            " parent id=",
            self.pid,
            " parent_cable_id=",
            parent_cable_id,
        )

    if self.cable_ is None:
        self.cable_ = Cable(self.cid, self.pid, len(self.points_))
        self.cableid2index_[self.cid] = len(self.cables_)
        self.cables_.append(self.cable_)
        self.cable_.parent_cable_id_ = parent_cable_id
    if self.cable_.id_ != self.cid:
        self.cable_ = Cable(self.cid, self.pid, len(self.points_))
        self.cableid2index_[self.cid] = len(self.cables_)
        self.cables_.append(self.cable_)
        self.cable_.parent_cable_id_ = parent_cable_id


def segments(self, node):
    self.in_cablegroup_ = False
    self.points_ = []
    self.cables_ = []
    self.cable_ = None
    self.id = -1
    self.cid = -1
    self.pid = -1
    self.nprox = 0
    self.cableid2index_ = {}
    self.ptid2pt_ = {}
    self.cablegroups_ = []
    self.groupname2index_ = {}


def segments_end(self, node):
    if debug:
        print("\nEnd of segments element")
        ic = 0
        ip = 0
        for cab in self.cables_:
            ic += 1
            for i in range(cab.first_, cab.first_ + cab.pcnt_):
                pt = self.points_[i]
                print(ip, pt.id_, pt.pid_, pt.x_, pt.y_, pt.z_, pt.d_)
                ip += 1
        print("ncable=", ic, "  npoint=", ip, "   nprox=", self.nprox, "\n")

    return
