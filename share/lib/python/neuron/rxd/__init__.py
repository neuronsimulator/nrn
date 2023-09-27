from .rxdException import RxDException

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
from .geometry import (
    membrane,
    inside,
    Shell,
    FractionalVolume,
    FixedCrossSection,
    FixedPerimeter,
    ScalableBorder,
    DistributedBoundary,
    MultipleGeometry,
)
from .plugins import set_solver

# deprecated:
# from geometry import ConstantArea, ConstantVolume
# TODO: if we ever separate Parameter and State from species, then we need to
#       rembember to call rxd._do_nbs_register()


def _model_view(tree):
    from . import species
    from neuron import h

    species_dict = species._get_all_species()
    if "TreeViewItem" not in dir(h):
        return
    if species_dict:
        rxd_head = h.TreeViewItem(None, "Reaction Diffusion Items")
        # TODO: do the species disappear if they go out of scope? or does this overcount?
        rxd_species = h.TreeViewItem(
            rxd_head, "%d Species/State/Parameter" % len(species_dict)
        )
        species_children = [
            h.TreeViewItem(rxd_species, str(name)) for name in species_dict
        ]
        rxd_reactions = h.TreeViewItem(
            rxd_head,
            "%d Reaction/Rate/MultiCompartmentReaction"
            % len([r for r in rxd._all_reactions if r() is not None]),
        )
        tree.append(rxd_head)


def save_state():
    """return a bytestring representation of the current rxd state

    Note: this is dependent on the order items were created."""
    from . import species
    import array
    import itertools
    import gzip

    version = 0
    state = []
    num_species = 0
    for sp in species._all_species:
        s = sp()
        if s is not None:
            my_state = s._state
            state.append(array.array("Q", [len(my_state)]).tobytes())
            state.append(my_state)
            num_species += 1
    if num_species == 0:
        return b""
    data = gzip.compress(
        array.array("Q", [num_species]).tobytes()
        + bytes(itertools.chain.from_iterable(state))
    )
    metadata = array.array("Q", [version, len(data)]).tobytes()
    return metadata + data


def restore_state(oldstate):
    """restore rxd state from a bytestring

    Note: this is dependent on the order items were created."""
    from . import species
    import array
    import itertools
    import gzip

    if oldstate == b"":
        for sp in species._all_species:
            s = sp()
            if s is not None:
                raise RxDException("Invalid state data: inconsistent number of Species")
        return
    metadata = array.array("Q")
    metadata.frombytes(oldstate[:16])
    version, length = metadata
    if version != 0:
        raise RxDException("Unsupported state version")
    # discard header and decompress remainder
    if len(oldstate) != length + 16:
        raise RxDException("Invalid state data: bad length")
    oldstate = gzip.decompress(oldstate[16:])
    metadata = array.array("Q")
    metadata.frombytes(oldstate[:8])
    num_species = metadata[0]
    active_species = []
    for sp in species._all_species:
        s = sp()
        if s is not None:
            active_species.append(s)
    if len(active_species) != num_species:
        raise RxDException("Invalid state data: inconsistent number of Species")
    position = 8
    for sp in active_species:
        data = array.array("d")
        size_array = array.array("Q")
        size_array.frombytes(oldstate[position : position + 8])
        position += 8
        size = size_array[0]
        data.frombytes(oldstate[position : position + size])
        position += size
        sp._state = bytes(data)
    if position != len(oldstate):
        raise RxDException("Invalid state data: bad length")
