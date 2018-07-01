from neuron import h
import itertools

# aliases to avoid repeatedly doing multiple hash-table lookups
_h_section_ref = h.SectionRef
_h_allsec = h.allsec
_h_parent_connection = h.parent_connection
_h_section_orientation = h.section_orientation
_itertools_combinations = itertools.combinations

def parent(sec):
    """Return the parent of sec or None if sec is a root"""
    sref = _h_section_ref(sec=sec)
    if sref.has_trueparent():
        return sref.trueparent().sec
    elif sref.has_parent():
        temp = sref.parent().sec
        # check if temp owns the connection point
        if _h_section_ref(sec=temp).has_parent() and _h_parent_connection(sec=temp) == _h_section_orientation(sec=temp):
            # connection point belongs to temp's ancestor
            return parent(temp)
        return temp
    else:
        return None
    

def parent_loc(sec, trueparent):
    """Return the position on the (true) parent where sec is connected
    
    Note that _h_section_orientation(sec=sec) is which end of the section is
    connected.
    """
    # TODO: would I ever have a parent but not a trueparent (or vice versa)
    sref = _h_section_ref(sec=sec)
    parent = sref.parent().sec
    while parent != trueparent:
        sec, parent = parent, _h_section_ref(sec=sec).parent().sec
    return _h_parent_connection(sec=sec)

class MorphologyDB:
    """
        MorphologyDB preserves the Neuron morphology at the time it was created.
        
        Subsequent changes will be ignored.
    """
    def __init__(self):
        """Create a MorphologyDB with the current NEURON morphology"""
        self._children = dict([(sec,[]) for sec in _h_allsec() ])
        self._parents = {}
        self._connection_pts = {}
        self._roots = []
        for sec in _h_allsec():
            parent_sec = parent(sec)
            if parent_sec is not None:
                self._children[parent_sec].append(sec)
                pt = (parent_sec, parent_loc(sec, parent_sec))
                local_pt = (sec, _h_section_orientation(sec=sec))
                if pt in self._connection_pts:
                    self._connection_pts[pt].append(local_pt)
                else:
                    self._connection_pts[pt] = [pt, local_pt]
            else:
                self._roots.append(sec)
            self._parents[sec] = parent_sec
    
    @property
    def roots(self):
        return self._roots
    
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
        for pts in list(self._connection_pts.values()):
            for pt1, pt2 in _itertools_combinations(pts, 2):
                if pt1[0] in secs and pt2[0] in secs:
                    result.append([pt1, pt2])
        return result
                
    
    
        

            
def main():
    # create 13 sections: s[0] -- s[12]
    s = [h.Section(name='s[%d]' % i) for i in range(13)]

    """
        Create the tree
        
              s0
        s1    s2         s3
        s4           s5      s6
        s7         s8 s9       s10 
    """
    for p, c in [[0, 1], [0, 2], [0, 3], [1, 4], [4, 7], [3, 5], [3, 6], [5, 8], [5, 9], [6, 10]]:
        s[c].connect(s[p])
    
    """
    and now s11 and s12, connected at the 0 ends
    """
    s[11].connect(s[12](0))
    
    # have NEURON print the topology
    h.topology()
    # now try it our way
    morph = MorphologyDB()
    for sec in s:
        print(sec.name() + ':')
        print('  children:', ', '.join(child.name() for child in morph.children(sec)))
        print('  parent:', morph.parent(sec).name() if morph.parent(sec) is not None else 'None')
    
    conns = morph.connections([s[i] for i in [2, 3, 4, 5, 6, 7, 9, 10, 11, 12]])
    for p1, p2 in conns:
        print('%s(%g)    %s(%g)' % (p1[0].name(), p1[1], p2[0].name(), p2[1]))
    
    print()
    print()
    print('roots:', morph.roots)
    return 0
    
if __name__ == '__main__':
    import sys
    sys.exit(main())
