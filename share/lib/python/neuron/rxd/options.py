from .rxdException import RxDException

import re

#TODO: This option is not currently observed
use_reaction_contribution_to_jacobian = True

concentration_nodes_3d = "surface"

# the number of electrophysiology fixed steps per rxd step
# WARNING: setting this to anything other than 1 is probably a very bad
#          idea, numerically speaking, at least for now
fixed_step_factor = 1

class _OverrideLockouts:
	def __init__(self):
		self._extracellular = True
	@property
	def extracellular(self):
		return self._extracellular
	@extracellular.setter
	def extracellular(self, val):
		self._extracellular = val

enable = _OverrideLockouts()
