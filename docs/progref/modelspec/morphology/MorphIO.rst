.. _morphio_progref:

MorphIO
=======

MorphIO is a library for reading and writing neuron morphology files.
It supports the following formats: SWC, ASC (aka. Neurolucida) and H5 v1.

We use MorphIO to provide a C++ implementation that matches the ``Import3d_GUI`` HOC stack.
This allows reading neuron morphology files in a variety of formats with increased performance, supporting single-point somas (``SOMA_SINGLE_POINT``), simple contour somas (``SOMA_SIMPLE_CONTOUR``), NeuroMorpho three-point cylinder somas (``SOMA_NEUROMORPHO_THREE_POINT_CYLINDERS``), and cylinder/frustum stack somas (``SOMA_CYLINDERS``).

There will also be a backward-compatible layer that will allow us to use the HOC stack together with MorphIO.
The implementation is located in the ``MorphIO.hoc`` file from the HOC library.
Finally we will be able to update the Import3D_GUI HOC stack to use MorphIO so that CellBuilder/Import3d can use it.

MorphIO is not available for Windows, because further support is required in MorphIO for the MinGW toolchain.

.. _morphio_api:

MorphIO API
===========

.. function:: load

    .. tab:: Python

        Syntax:
            ``from neuron import morphology``

            ``morphology.load(cell_obj, morph_file_or_wrapper)``

            ``m.instantiate(cell_obj)``

            ``h.morphio_load(cell_obj, morph_file[, write_hoc])``

        Description:
            Loads a morphology from a file path (string or ``pathlib.Path``) or a :class:`MorphIOWrapper` instance and instantiates it into ``cell_obj``.

            Alternatively, call :meth:`MorphIOWrapper.instantiate` on an existing wrapper. The underlying HOC function ``h.morphio_load`` can also be called directly; when its optional third argument ``write_hoc`` is set to ``1``, the generated HOC commands are written to ``<stem>.hoc`` in the same directory as ``morph_file``.

        Example:

            .. code-block:: python

                from pathlib import Path
                from neuron import h, morphology

                cell = h.HocBasicCell()
                m = morphology.load(cell, Path("test/morphology/test.h5"))

    .. tab:: HOC

        Syntax:
            ``morphio_load(obj, morph_file[, write_hoc])``

            ``morphio_read(obj, morph_file)``

        Description:
            Loads a morphology from ``morph_file`` and executes its HOC representation in the Cell object's context.

            If the optional third argument ``write_hoc`` is set to ``1`` (or when calling ``morphio_read`` after ``load_file("MorphIO.hoc")``), the HOC representation of the morphology is written to a ``.hoc`` file in the same directory as the morphology file.

            Returns ``1`` if the morphology was loaded successfully, ``0`` otherwise. Raises ``hoc_execerror`` if the Cell object or path are not provided or if loading fails.

.. class:: MorphIOWrapper

    .. tab:: Python

        Syntax:
            ``from neuron.morphology import MorphIOWrapper``

            ``m = MorphIOWrapper(morph_file)``

        Description:
            Python binding for ``neuron::morphology::MorphIOWrapper``. Holds the loaded MorphIO morphology and provides access to the generated HOC commands, instantiation into cells, and section metadata. Accepts strings and ``pathlib.Path`` objects.

    .. tab:: HOC

        Syntax:
            ``load_file("MorphIO.hoc")``

            ``objref m``

            ``m = new MorphIOWrapper(morph_file)``

            ``m = new MorphIO(morph_file)``

        Description:
            Object-oriented HOC template defined in ``MorphIO.hoc`` providing an interface similar to ``Import3d_GUI``. Also accessible via the alias template ``MorphIO``.

.. method:: MorphIOWrapper.instantiate

    .. tab:: Python

        Syntax:
            ``m.instantiate(cell)``

        Description:
            Instantiates the morphology into the provided Cell object.

    .. tab:: HOC

        Syntax:
            ``m.instantiate(cell[, write_hoc])``

        Description:
            Instantiates the morphology into the provided Cell object. When optional second argument ``write_hoc`` is set to 1, writes the generated HOC commands to ``<stem>.hoc``.

.. method:: MorphIOWrapper.to_hoc

    .. tab:: Python

        Syntax:
            ``cmds = m.to_hoc()``

        Description:
            Returns the list of HOC commands that instantiate the morphology sections, subsets, soma 3D points, connections, and neurite 3D points.

.. data:: MorphIOWrapper.section_names

    .. tab:: Python

        Syntax:
            ``names = m.section_names``

        Description:
            Read-only property returning the list of section names ordered by section ID:

            .. code-block:: python

                [
                    "soma",     # section id 0
                    "axon[0]",  # section id 1
                    "axon[1]",  # section id 2
                    "axon[2]",  # section id 3
                    "dend[0]",  # section id 4
                    "dend[1]",  # section id 5
                    "dend[2]"   # section id 6
                ]

.. data:: MorphIOWrapper.section_counts

    .. tab:: Python

        Syntax:
            ``counts = m.section_counts``

        Description:
            Read-only property returning a dictionary mapping section types to their counts, e.g. ``{'soma': 1, 'axon': 61, 'dend': 60, 'apic': 72}``.

.. data:: MorphIOWrapper.section_type_distribution

    .. tab:: Python

        Syntax:
            ``dist = m.section_type_distribution``

        Description:
            Read-only property returning a list of ``(type_id, start_id, count)`` tuples describing the distribution of section type IDs in the morphology:

            .. code-block:: text

                | type_id | start_id  |   count   |
                | ------- | --------- | --------- |
                |      1  |       -1  |        1  |
                |      2  |        0  |     2724  |
                |      3  |     2724  |       75  |
