from .rxdException import RxDException

#import sys
#if 'neuron.rxd' in sys.modules:
#    raise RxDException('NEURON CRxD module cannot be used with NEURON RxD module.')

from . import rxd, constants
from .species import Species, Parameter, State
from .region import Region, Extracellular
from .rate import Rate
from .reaction import Reaction
from . import geometry
from .multiCompartmentReaction import MultiCompartmentReaction
from .rxd import re_init, set_solve_type, nthread
from .rxdmath import v
try:
  from . import dimension3
except:
  pass
from .rangevar import RangeVar
from .geometry import membrane, inside, Shell, FractionalVolume, FixedCrossSection, FixedPerimeter, ScalableBorder, DistributedBoundary
from .plugins import set_solver
# deprecated:
# from geometry import ConstantArea, ConstantVolume
# TODO: if we ever separate Parameter and State from species, then we need to
#       rembember to call rxd._do_nbs_register()

def _model_view(tree):
    from . import species
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


