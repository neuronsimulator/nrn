from neuron import h
import itertools

def parent(sec):
    """Return the parent of sec or None if sec is a root"""
    sref = h.SectionRef(sec=sec)
    return sref.trueparent().sec if sref.has_trueparent() else None

def parent_loc(sec):
    """Return the position on the (true) parent where sec is connected
    
    Note that h.section_orientation(sec=sec) is which end of the section is
    connected.
    """
    # TODO: would I ever have a parent but not a trueparent (or vice versa)
    sref = h.SectionRef(sec=sec)
    if not sref.has_trueparent():
        return None
    trueparent = sref.trueparent().sec
    parent = sref.parent().sec
    while parent.name() != trueparent.name():
        sec, parent = parent, h.SectionRef(sec=sec).parent().sec
    return h.parent_connection(sec=sec)


class SectionDict(dict):
    def __setitem__(self, key, value):
        if not isinstance(key, str):
            key = key.name()
        dict.__setitem__(self, key, value)
    def __getitem__(self, key):
        if not isinstance(key, str):
            key = key.name()
        return dict.__getitem__(self, key)
    def __contains__(self, item):
        if not isinstance(item, str):
            item = item.name()
        return dict.__contains__(self, item)
        
class SectionPtDict(dict):
    def __setitem__(self, key, value):
        sec, pt = key
        if not isinstance(sec, str):
            sec = sec.name()
        dict.__setitem__(self, (sec, pt), value)
    def __getitem__(self, key):
        sec, pt = key
        if not isinstance(sec, str):
            sec = sec.name()
        return dict.__getitem__(self, (sec, pt))
    def __contains__(self, item):
        sec, pt = item
        if not isinstance(sec, str):
            sec = sec.name()
        return dict.__contains__(self, (sec, pt))



class MorphologyDB:
    """
        MorphologyDB preserves the Neuron morphology at the time it was created.
        
        Subsequent changes will be ignored.
    """
    def __init__(self):
        """Create a MorphologyDB with the current NEURON morphology"""
        self._children = SectionDict()
        self._parents = SectionDict()
        self._connection_pts = SectionPtDict()
        for sec in h.allsec():
            self._children[sec] = []
        for sec in h.allsec():
            parent_sec = parent(sec)
            if parent_sec is not None:
                self._children[parent_sec].append(sec)
                pt = (parent_sec.name(), parent_loc(sec))
                local_pt = (sec.name(), h.section_orientation(sec=sec))
                if pt in self._connection_pts:
                    self._connection_pts[pt].append(local_pt)
                else:
                    self._connection_pts[pt] = [pt, local_pt]
            self._parents[sec] = parent_sec
    
    def parent(self, sec):
        """Return the (true)parent section of sec"""
        return self._parents[sec]
    
    def children(self, sec):
        """Return a list of the children of sec"""
        return list(self._children[sec])
    
    def nchild(self, sec):
        """Return the number of children of sec"""
        return len(self._children[sec])
    
    def connections(self, secs):
        """Return a list of pairwise connections within secs"""
        result = []
        for pts in self._connection_pts.values():
            for pt1, pt2 in itertools.combinations(pts, 2):
                if pt1[0] in secs and pt2[0] in secs:
                    result.append([pt1, pt2])
        return result
                
    
    
        

            
def main():
    # create 11 sections: s[0] -- s[10]
    s = []
    for i in xrange(11):
        s.append(h.Section(name='s[%d]' % i))
    """
        Create the tree
        
              s0
        s1    s2         s3
        s4           s5      s6
        s7         s8 s9       s10 
    """
    for p, c in [[0, 1], [0, 2], [0, 3], [1, 4], [4, 7], [3, 5], [3, 6], [5, 8], [5, 9], [6, 10]]:
        s[c].connect(s[p])
    # have NEURON print the topology
    h.topology()
    # now try it our way
    morph = MorphologyDB()
    for sec in s:
        print sec.name() + ':'
        print '  children:', ', '.join(child.name() for child in morph.children(sec))
        print '  parent:', morph.parent(sec).name() if morph.parent(sec) is not None else 'None'
    
    conns = morph.connections([s[i] for i in [2, 3, 4, 5, 6, 7, 9, 10]])
    for p1, p2 in conns:
        print '%s(%g)    %s(%g)' % (p1[0].name(), p1[1], p2[0].name(), p2[1])
    return 0
    
if __name__ == '__main__':
    import sys
    sys.exit(main())
