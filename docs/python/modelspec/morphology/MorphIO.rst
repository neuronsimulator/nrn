.. _MorphIO_python:

MorphIO (Python)
================

MorphIO is a library for reading and writing neuron morphology files.
It supports SWC, ASC (Neurolucida), and HDF5 (H5 v1) formats.

We use MorphIO to provide a fast C++ implementation matching the Import3D_GUI HOC stack,
supporting single-point somas (``SOMA_SINGLE_POINT``), simple contour somas (``SOMA_SIMPLE_CONTOUR``),
NeuroMorpho three-point cylinder somas (``SOMA_NEUROMORPHO_THREE_POINT_CYLINDERS``), and cylinder/frustum stack somas (``SOMA_CYLINDERS``).

There will also be a backward-compatible layer that will allow us to use the HOC stack together with MorphIO.
The implementation is located in the ``MorphIO.hoc`` file from the HOC library.
Finally we will be able to update the Import3D_GUI HOC stack to use MorphIO so that CellBuilder/Import3d can use it.

MorphIO is not available on Windows, because further support is required in MorphIO for the MinGW toolchain.

.. _morphio_python_api:

MorphIO Python API
==================

The ``neuron.morphology`` module (also accessible via ``neuron.morphio_api``) provides an idiomatic Python interface for loading and querying morphologies.

Examples
--------

Loading directly into a cell:

.. code-block:: python

    from pathlib import Path
    from neuron import h, morphology

    # Define a cell template with standard section lists
    cell = h.HocBasicCell()

    # Load from a string path or pathlib.Path
    morph = morphology.load(cell, Path("path/to/neuron.swc"))
    print(morph)  # <MorphIOWrapper 'path/to/neuron.swc' (sections=194)>
    print(f"Total sections: {len(morph)}")
    print(f"Section counts: {morph.section_counts}")  # {'soma': 1, 'axon': 61, 'dend': 60, 'apic': 72}

Instantiating from an existing wrapper:

.. code-block:: python

    from neuron.morphology import MorphIOWrapper

    # Pre-parse and inspect before instantiating
    m = MorphIOWrapper("path/to/astrocyte.h5")
    print(m.section_names[:5])

    cell = h.HocBasicCell()
    m.instantiate(cell)

MorphIO Python Functions
------------------------

.. py:function:: load(cell, morphology)
    :noindex:

    Load morphology data from a file or wrapper into a NEURON cell.

    :param cell: A NEURON Cell object (containing ``somatic``, ``axonal``, ``basal``, and ``apical`` SectionLists).
    :param morphology: A file path (``str`` or ``pathlib.Path`` / ``os.PathLike``) or an existing ``MorphIOWrapper`` instance.
    :return: The ``MorphIOWrapper`` instance.

MorphIO Python Classes
----------------------

.. py:class:: MorphIOWrapper
    :noindex:

    Holds the loaded MorphIO morphology and provides utility methods and properties to inspect and instantiate the data.

    .. py:method:: __init__(self, path)
        :noindex:

        Initialize the ``MorphIOWrapper`` from a morphology file path (accepts ``str`` or ``os.PathLike`` / ``pathlib.Path``).

    .. py:method:: instantiate(self, cell)
        :noindex:

        Instantiate the morphology into a NEURON cell object.

    .. py:method:: to_hoc(self)
        :noindex:

        Return the list of HOC statements defining the morphology sections, subsets, and 3D points.

    .. py:attribute:: filename
        :noindex:

        The path of the loaded morphology file.

    .. py:attribute:: section_names
        :noindex:

        List of section name strings in the morphology, e.g. ``["soma", "axon[0]", "dend[0]", ...]``.
        Indexing ``m[i]`` or iterating over ``m`` delegates to this list.

    .. py:attribute:: section_counts
        :noindex:

        Dictionary mapping section types to their counts, e.g. ``{'soma': 1, 'axon': 61, 'dend': 60, 'apic': 72}``.

    .. py:attribute:: section_type_distribution
        :noindex:

        List of ``(type_id, start_id, count)`` tuples describing the internal section distribution.

    .. py:method:: __len__(self)
        :noindex:

        Returns the total number of sections in the morphology.

    .. py:method:: __getitem__(self, index)
        :noindex:

        Returns the section name at the specified index.
