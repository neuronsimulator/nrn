#from neuron import h
#h.load_file('stdrun.hoc')
from rxdException import RxDException

try:
    import scipy
except:
    raise RxDException('NEURON RxD module requires SciPy')

import rxd
from species import Species
from region import Region
from rate import Rate
from reaction import Reaction
import geometry
from multiCompartmentReaction import MultiCompartmentReaction
from rxd import re_init
import dimension3
from rangevar import RangeVar
from geometry import membrane, inside, Shell, FractionalVolume, FixedCrossSection, FixedPerimeter, ScalableBorder
# deprecated:
# from geometry import ConstantArea, ConstantVolume
Parameter = Species
State = Species

def _model_view(tree):
    import species
    from neuron import h
    species_dict = species._get_all_species()
    if 'TreeViewItem' not in dir(h): return
    if species_dict:
        rxd_head = h.TreeViewItem(None, 'Reaction Diffusion Items')
        # TODO: do the species disappear if they go out of scope? or does this overcount?
        rxd_species = h.TreeViewItem(rxd_head, '%d Species/State/Parameter' % len(species_dict))
        species_children = [h.TreeViewItem(rxd_species, str(name)) for name in species_dict]
        rxd_reactions = h.TreeViewItem(rxd_head, '%d Reaction/Rate/MultiCompartmentReaction' % len([r for r in rxd._all_reactions if r() is not None]))        
        tree.append(rxd_head)
