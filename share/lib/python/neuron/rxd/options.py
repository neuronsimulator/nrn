# TODO: This option is not currently observed
use_reaction_contribution_to_jacobian = True

concentration_nodes_3d = "surface"

# how far inside must a voxel be to be consider inside
# the value is necessary to account for floating point precision
ics_distance_threshold = -1e-12

# resolution (relative to dx) of sampling points use to determine surface
# volume fractions.
ics_partial_volume_resolution = 2

# resolution (relative to dx) of sampling points use to determine surface
# areas of surface voxels.
ics_partial_surface_resolution = 1

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
