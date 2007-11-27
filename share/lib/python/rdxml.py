
import xml
import xml.sax

i3d = 1

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
    
class Cable:
  def __init__(self, id, pid, ipnt):
    self.first_ = ipnt
    self.pcnt_ = 0
    self.id_ = id
    self.pid_ = pid
    self.px_ = -1.
    self.parent_cable_id_ = -1
    self.name_ = ""

class CableGroup:
  def __init__(self, name):
    self.name_ = name
    self.cable_indices_ = []

class BioParm:
  def __init__(self, name, value):
    self.name_ = name
    self.value_ = value
    self.group_index_ = -1

class BioMech:
  def __init__(self, name):
    self.name_ = name
    self.parms_ = []

class MyContentHandler(xml.sax.ContentHandler):

  def __init__(self):
    self.in_biogroup_ = False
    self.locator = None
    pass

  def setDocumentLocator(self, locator):
    self.locator = locator
    return

  def startDocument(self):
    return
    print "startDocument"

  def endDocument(self):
    #print "endDocument"
    i3d.parsed(self)

  def prattrs(self, attrs):
    for i in attrs.getNames():
      print "  ", i, " ", attrs.getValue(i)

  def startElement(self, name, attrs):
    if self.elements.has_key(name):
      self.elements[name](self, attrs)
    else :
      print "startElement unknown", name

  def endElement(self, name):
    if self.elements.has_key('end'+name):
      self.elements['end'+name](self)

  def cell(self, attrs):
    self.biomechs_ = []
    return
    print "cell"
    self.prattrs(attrs)

  def endcell(self):
    return
    print 'endcell'
    for i in range(len(self.cables_)):
      c = self.cables_[i]
      print i, c.id_, c.parent_cable_id_, c.name_, c.px_, c.pcnt_

  def segments(self, attrs):
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

  def endsegments(self):
    return
    ic=0
    ip=0
    for cab in self.cables_ :
      ic += 1
      for i in range(cab.first_, cab.first_ + cab.pcnt_):
        pt = self.points_[i]
        print ip, pt.id_, pt.pid_, pt.x_, pt.y_, pt.z_, pt.d_
        ip += 1
    print "ncable=", ic, "  npoint=", ip, "   nprox=", self.nprox

  def segment(self, attrs):
    #print "segment id=", int(attrs['id']), "  cable=", int(attrs['cable'])
    self.id = int(attrs['id'])
    self.cid = int(attrs['cable'])
    parent_cable_id = -1
    if attrs.has_key('parent') :
      self.pid = int(attrs['parent'])
      parent_cable_id = self.ptid2pt_[self.pid].cid_
    else:
      self.pid = -1
    if self.cable_ == None :
      self.cable_ = Cable(self.cid, self.pid, len(self.points_))
      self.cableid2index_[self.cid] = len(self.cables_)
      self.cables_.append(self.cable_)
      self.cable_.parent_cable_id_ = parent_cable_id
    if self.cable_.id_ != self.cid:
      self.cable_ = Cable(self.cid, self.pid, len(self.points_))
      self.cableid2index_[self.cid] = len(self.cables_)
      self.cables_.append(self.cable_)
      self.cable_.parent_cable_id_ = parent_cable_id

  def proximal(self, attrs):
    self.nprox += 1
    pt = Point(-1, self.pid, self.cid, self.locator.getLineNumber())
    pt.set(float(attrs['x']), float(attrs['y']), float(attrs['z']), float(attrs['diameter']))
    self.points_.append(pt)
    self.cable_.pcnt_ += 1
    return
    print self.cable_.id_, "   ", self.cable_.pcnt_

  def distal(self, attrs):
    pt = Point(self.id, self.pid, self.cid, self.locator.getLineNumber())
    self.ptid2pt_[self.id] = pt
    pt.set(float(attrs['x']), float(attrs['y']), float(attrs['z']), float(attrs['diameter']))
    self.points_.append(pt)
    self.cable_.pcnt_ += 1

  def cable(self, attrs):
    if self.in_cablegroup_:
      self.cablegroups_[-1].cable_indices_.append(self.cableid2index_[int(attrs['id'])])
    else:
      cab = self.cables_[self.cableid2index_[int(attrs['id'])]]
      if attrs.has_key('fractAlongParent'):
        cab.px_ = float(attrs['fractAlongParent'])
      else:
        cab.px_ = 1.
      cab.name_ = attrs['name'].encode('ascii')

  def cablegroup(self, attrs):
    self.in_cablegroup_ = True
    name = attrs['name'].encode('ascii')
    self.cablegroups_.append(CableGroup(name))
    self.groupname2index_[name] = len(self.cablegroups_) - 1

  def endcablegroup(self):
    self.in_cablegroup_ = False

  def section(self, attrs):
    return
    print 'section'
    self.prattrs(attrs)

  def biophysics(self, attrs):
    # following is worthless. need units for Ra, cm, gmax, e
    self.is_physiological_units_ = True
    if attrs.has_key('units'):
      if attrs['units'] == 'Physiological Units':
        self.is_physiological_units_ = True

  def biomech(self, attrs):
    self.biomechs_.append(BioMech(attrs["name"].encode('ascii')))
    return

  def bioparm(self, attrs):
    value = float(attrs['value'])
    if attrs.has_key("name"):
      # convert from physiological to NEURON units
      if self.is_physiological_units_:
        if attrs["name"] == 'gmax':
          value *= .001
      # NEURON ModelView converts g and gbar to gmax...
      # following only fixes the problem for g_pas
      name = attrs["name"].encode('ascii') + '_' + self.biomechs_[-1].name_
      if name == 'gmax_pas':
        name = 'g_pas'
      self.biomechs_[-1].parms_.append(BioParm(name, value))
    else:
      if self.biomechs_[-1].name_ == 'Ra':
        value *= 1000.
      self.biomechs_[-1].parms_.append(BioParm(self.biomechs_[-1].name_, value))
    return

  def biogroup(self, attrs):
    self.in_biogroup_ = True
    # the name is in the content
    return

  def endbiogroup(self):
	self.in_biogroup_ = False

  def biora(self, attrs):
    self.biomechs_.append(BioMech('Ra'))
    return

  def biocm(self, attrs):
    self.biomechs_.append(BioMech('cm'))
    return

  def nothing(self, attrs):
    pass

  def characters(self, content):
    if self.in_biogroup_:
      self.biomechs_[-1].parms_[-1].group_index_ = self.groupname2index_[content.strip().encode('ascii')]
    return

  elements = {
    'neuroml':nothing,
    'meta:notes':nothing,
    'cells':nothing,
    'cell':cell,
    'endcell':endcell,
    'segments':segments,
    'endsegments':endsegments,
    'segment':segment,
    'proximal':proximal,
    'distal':distal,
    'cables':nothing,
    'cable':cable,
    'biophysics':biophysics,
    'cablegroup':cablegroup,
    'endcablegroup':endcablegroup,
    'section':section,
    'bio:mechanism':biomech,
    'bio:parameter':bioparm,
    'bio:group':biogroup,
    'endbio:group':endbiogroup,
    'bio:specificAxialResistance':biora,
    'bio:specificCapacitance':biocm,
  }

def rdxml(fname) :
  xml.sax.parse(fname, MyContentHandler())

