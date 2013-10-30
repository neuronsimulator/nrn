from neuron import h
from . import geometry as geo

_region_count = 0

def _sort_secs(secs):
    # sort the sections
    root_secs = h.SectionList()
    root_secs.allroots()
    all_sorted = h.SectionList()
    for root in root_secs:
        all_sorted.wholetree(sec=root)
    secs_names = {sec.name():sec for sec in secs}
    for sec in secs:
        if h.section_orientation(sec=sec):
            raise Exception('still need to deal with backwards sections')
    return [secs_names[sec.name()] for sec in all_sorted if sec.name() in secs_names]


class Region:
    def __init__(self, sections, nrn_region=None, geometry=None):
        global _region_count
        # TODO: validate sections (should be list of nrn.Section)
        self._secs = _sort_secs(sections)
        if nrn_region not in (None, 'i', 'o'):
            raise Exception('nrn_region must be one of: None, "i", "o"')
        self._nrn_region = nrn_region
        if geometry is None:
            geometry = geo.inside
        
        self._geometry = geometry
        
        self._id = _region_count
        _region_count += 1

    @property
    def nrn_region(self):
        return self._nrn_region
    
    @property
    def _semi_compile(self):
        return 'r%d' % self._id
    
    @property
    def secs(self):
        # return a copy of the section list
        return list(self._secs)
    
