"""NEURON morphology loading and manipulation utilities using MorphIO."""

import os
from pathlib import Path
from typing import Optional, Union

try:
    from neuron import morphio_api

    MorphIOWrapper = morphio_api.MorphIOWrapper

    def load(
        cell, morphology: Union[str, os.PathLike, MorphIOWrapper]
    ) -> MorphIOWrapper:
        """Load a morphology into a cell.

        Parameters
        ----------
        cell:
            A NEURON cell object (or HOC object context) containing somatic,
            axonal, basal, and apical SectionLists.
        morphology:
            A file path (str or os.PathLike / pathlib.Path) to an SWC, ASC, or H5
            morphology file, or an existing MorphIOWrapper instance.

        Returns
        -------
        MorphIOWrapper
            The MorphIOWrapper instance that was instantiated into the cell.
        """
        if isinstance(morphology, MorphIOWrapper):
            morphology.instantiate(cell)
            return morphology
        m = MorphIOWrapper(os.fspath(morphology))
        m.instantiate(cell)
        return m

    __all__ = ["MorphIOWrapper", "load"]

except ImportError:
    MorphIOWrapper = None
    load = None
    __all__ = []
