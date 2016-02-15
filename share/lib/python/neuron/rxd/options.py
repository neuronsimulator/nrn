from .rxdException import RxDException

try:
    import scipy
except:
    raise RxDException("NEURON's rxd module requires scipy")

import warnings
import re

# only default to using reaction contribution to Jacobian if scipy more recent than 0.10.0
# if the version format has changed, then we assume it is a recent version of scipy
scipy_number_parts = re.match(r'(\d*)\.(\d*)\.(\d*)', scipy.__version__)
if scipy_number_parts:
    scipy_version = [int(v) for v in scipy_number_parts.groups()]

if scipy_number_parts is None or scipy_version[0] > 0 or scipy_version[1] > 10 or (scipy_version[1] == 10 and scipy_version[2] >= 1):
    use_reaction_contribution_to_jacobian = True
    """Should we use the reaction contribution to the Jacobian? probably yes if cvode"""
else:
    warnings.warn('scipy < 0.10.1 found; setting rxd.options.use_reaction_contribution_to_jacobian = False to avoid a memory leak in scipy.sparse.linalg.factorized')
    use_reaction_contribution_to_jacobian = False

# the number of electrophysiology fixed steps per rxd step
# WARNING: setting this to anything other than 1 is probably a very bad
#          idea, numerically speaking, at least for nwo
fixed_step_factor = 1
