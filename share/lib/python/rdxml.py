#
# See http://neuroml.svn.sourceforge.net/viewvc/neuroml/NeuroML2nrn/
# for the latest version of this file.
#


import xml
import xml.sax

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
      return "Point: "+ str(self.id_)+ ", ("+str(self.x_)+ ", "+str(self.y_)+ ", "+str(self.z_)+ " | "+str(self.d_)+ ") , par: "+str(self.pid_)+", cable id: "+str(self.cid_)
    
class Cable:
  def __init__(self, id, pid, ipnt):
    self.first_ = ipnt
    self.pcnt_ = 0
    self.id_ = id
    self.pid_ = pid
    self.px_ = -1.
    self.parent_cable_id_ = -1
    self.name_ = ""
    if debug: print " New ", self.__str__()

  def __str__(self):
      return "Cable: id="+str(self.id_)+" name="+self.name_+" pid="+str(self.parent_cable_id_)+ " first="+str(self.first_)

class CableGroup:
  def __init__(self, name):
    self.name_ = name
    self.cable_indices_ = []
    
  def __str__(self):
      info = "CableGroup: "+ self.name_
      for ci in self.cable_indices_:
        info = info+"\n" + "  "+str(ci)
      return info

class BioParm:
  def __init__(self, name, value):
    self.name_ = name
    self.value_ = value
    self.group_index_ = -1
    
  def __str__(self):
      return "BioParm: "+ self.name_+ ", val: "+str(self.value_)+", grp id: "+str(self.group_index_)

class BioMech:
  def __init__(self, name):
    self.name_ = name
    self.parms_ = []
    
  def __str__(self):
      info = "BioMech: "+ self.name_
      for p in self.parms_:
        info = info+"\n" + "  "+str(p)
      return info

class MyContentHandler(xml.sax.ContentHandler):

  def __init__(self, ho):
    if debug: print "Starting parsing of NeuroML file... "
    self.i3d = ho
    self.in_biogroup_ = False
    self.in_paramgroup_ = False
    self.locator = None
    pass

  def setDocumentLocator(self, locator):
    self.locator = locator
    return

  def startDocument(self):
    if debug: print "startDocument"
    return

  def endDocument(self):
    if debug: print "endDocument"
    self.i3d.parsed(self)

  def prattrs(self, attrs):
    for i in attrs.getNames():
      print "  ", i, " ", attrs.getValue(i)

  def startElement(self, name, attrs):
    if self.elements.has_key(name):
      if debug: print "startElement: ", name
      self.elements[name](self, attrs)
    else :
      if debug: print "startElement unknown", name

  def endElement(self, name):
    if self.elements.has_key('end'+name):
      self.elements['end'+name](self)

  def cell(self, attrs):
    self.biomechs_ = []
    self.cellname = ""
    if attrs.has_key('name'):
        self.cellname = str(attrs['name'])
    if debug: 
        print "cell"
        self.prattrs(attrs)
    return

  def endcell(self):
    if debug:
        print 'endcell'
        for i in range(len(self.cables_)):
            c = self.cables_[i]
            print "Cable ",i,": ", c.id_, c.parent_cable_id_, c.name_, c.px_, c.pcnt_
            
        for bm in self.biomechs_:
            print bm
            
        for cg in self.cablegroups_:
            print cg
            
        for g2i in self.groupname2index_:
            print "groupname2index: ", g2i, " - ", str(self.groupname2index_[g2i])
    return

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
    if debug:
        print "\nEnd of segments element"
        ic=0
        ip=0
        for cab in self.cables_ :
            ic += 1
            for i in range(cab.first_, cab.first_ + cab.pcnt_):
                pt = self.points_[i]
                print ip, pt.id_, pt.pid_, pt.x_, pt.y_, pt.z_, pt.d_
                ip += 1
        print "ncable=", ic, "  npoint=", ip, "   nprox=", self.nprox,"\n"
        
    return

  def segment(self, attrs):
    self.id = int(attrs['id'])
    self.cid = int(attrs['cable'])
    parent_cable_id = -1
    if attrs.has_key('parent') :
      self.pid = int(attrs['parent'])
      parent_cable_id = self.ptid2pt_[self.pid].cid_
    else:
      self.pid = -1
      
    if debug:
        print "\nsegment id=", self.id , "  cable=", self.cid, " parent id=", self.pid, " parent_cable_id=", parent_cable_id
        
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
    if debug:
        print "Proximal: "+ str(pt)
        print "Cable ", self.cable_.id_, " has ", self.cable_.pcnt_, " points"
    return

  def distal(self, attrs):
    pt = Point(self.id, self.pid, self.cid, self.locator.getLineNumber())
    self.ptid2pt_[self.id] = pt
    pt.set(float(attrs['x']), float(attrs['y']), float(attrs['z']), float(attrs['diameter']))
    if (self.cable_.pcnt_==1):
        proxpt = self.points_[len(self.points_)-1]
        if (proxpt.x_==pt.x_ and proxpt.y_==pt.y_ and proxpt.z_==pt.z_ and proxpt.d_==pt.d_):
            if debug: print "Prox and distal same, assuming spherical segment"
            pt.y_ = pt.y_ + (pt.d_ / 2.0)
            self.points_[len(self.points_)-1].y_  = self.points_[len(self.points_)-1].y_ - (self.points_[len(self.points_)-1].d_ / 2.0)
            if debug: 
                print "New distal: "+ str(pt)
                print "New proximal: "+ str(self.points_[len(self.points_)-1])
    self.points_.append(pt)
    self.cable_.pcnt_ += 1
    if debug:
        print "Distal: "+ str(pt)
        print "Cable ", self.cable_.id_, " has ", self.cable_.pcnt_, " points"

  def cable(self, attrs):
    self.lastcabid_ =-1
    if self.in_cablegroup_:
      self.cablegroups_[-1].cable_indices_.append(self.cableid2index_[int(attrs['id'])])
    else:
      cab = self.cables_[self.cableid2index_[int(attrs['id'])]]
      self.lastcabid_ = int(attrs['id'])
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
    
    
  def group(self, attrs):
    self.in_paramgroup_ = True
        
  def endgroup(self):
    self.in_paramgroup_ = False
        

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
    name = attrs["name"].encode('ascii')
    
    if attrs.has_key('passiveConductance'):
      if attrs['passiveConductance'] == 'true' or attrs['passiveConductance'] == '1':
        print "Substituting passive conductance", name , " in file for inbuilt mechanism pas as attribute passiveConductance = true "
        name = 'pas'
    
    self.biomechs_.append(BioMech(name))
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
      groupName = content.strip().encode('ascii')
      self.biomechs_[-1].parms_[-1].group_index_ = self.groupname2index_[groupName]
      if debug:
        print "Content: ", groupName, " relevant to biogroup: ", self.biomechs_[-1]
        
    if self.in_paramgroup_:
      groupName = content.strip().encode('ascii')
      if debug:
        print "Content: ", groupName, " for cable: ", self.lastcabid_
      cab = None
      for c in self.cablegroups_:
        if c.name_ == groupName:
            cab = c
      if (cab == None):
        cab = CableGroup(groupName)
        self.cablegroups_.append(cab)
        self.groupname2index_[groupName] = len(self.cablegroups_) - 1
      cab.cable_indices_.append(self.lastcabid_)

    return

  elements = {
    'neuroml':nothing,
    'morphml':nothing,
    'meta:notes':nothing,
    'cells':nothing,
    'cell':cell,
    'endcell':endcell,
    'segments':segments,
    'mml:segments':segments,
    'endsegments':endsegments,
    'endmml:segments':endsegments,
    'segment':segment,
    'mml:segment':segment,
    'proximal':proximal,
    'mml:proximal':proximal,
    'distal':distal,
    'mml:distal':distal,
    'cables':nothing,
    'mml:cables':nothing,
    'cable':cable,
    'meta:group':group,
    'endmeta:group':endgroup,
    'mml:cable':cable,
    'biophysics':biophysics,
    'cablegroup':cablegroup,
    'mml:cablegroup':cablegroup,
    'endcablegroup':endcablegroup,
    'endmml:cablegroup':endcablegroup,
    'section':section,
    'bio:mechanism':biomech,
    'bio:parameter':bioparm,
    'bio:group':biogroup,
    'endbio:group':endbiogroup,
    'bio:specificAxialResistance':biora,
    'bio:specificCapacitance':biocm,
  }

def rdxml(fname, ho) :
  xml.sax.parse(fname, MyContentHandler(ho))
