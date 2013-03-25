#from neuron import h
#h.load_file('stdrun.hoc')

try:
    import scipy
except:
    raise Exception('NEURON RxD module requires SciPy')

import rxd
from species import Species
from region import Region
from rate import Rate
from reaction import Reaction
import geometry
from multiCompartmentReaction import MultiCompartmentReaction
from rxd import re_init

from geometry import membrane, inside, Shell, FractionalVolume, FixedCrossSection, FixedPerimeter
# deprecated:
# from geometry import ConstantArea, ConstantVolume
Parameter = Species
State = Species
