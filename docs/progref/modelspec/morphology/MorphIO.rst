.. _morphio_progref:

MorphIO
=======

MorphIO is a library for reading and writing neuron morphology files.
It supports the following formats: SWC, ASC (aka. Neurolucida) and H5 v1.

We use MorphIO to provide a C++ implementation that matches the ``Import3d_GUI`` HOC stack.
This allows reading neuron morphology files in a variety of formats with increased performance, supporting single-point somas (``SOMA_SINGLE_POINT``), simple contour somas (``SOMA_SIMPLE_CONTOUR``), NeuroMorpho three-point cylinder somas (``SOMA_NEUROMORPHO_THREE_POINT_CYLINDERS``), and cylinder/frustum stack somas (``SOMA_CYLINDERS``).

There is also a backward-compatible layer that allows using the HOC stack together with MorphIO.
The implementation is located in the ``MorphIO.hoc`` file from the HOC library.

MorphIO is not available for Windows, because further support is required in MorphIO for the MinGW toolchain.

.. seealso::

    For language-specific reference pages, see :ref:`MorphIO_python` and :ref:`MorphIO_hoc`.

.. _morphio_api:

MorphIO API
===========

.. function:: morphio_load

    .. tab:: Python

        Syntax:
            ``h.morphio_load(cell_obj, morph_file[, write_hoc])``

            ``morphio_api.morphio_load(cell_obj, morph_file)``

            ``morphio_api.morphio_load(cell_obj, morph_wrapper)``

        Description:
            Loads a morphology from a file or a :class:`MorphIOWrapper` instance and executes its HOC representation in the ``cell_obj`` context.

            If the optional third argument ``write_hoc`` in ``h.morphio_load`` is set to ``1``, the generated HOC commands are written to ``<stem>.hoc`` in the same directory as ``morph_file``.

        Example:

            .. code-block:: python

                from neuron import h, morphio_api

                cell = h.HocBasicCell()
                m = morphio_api.MorphIOWrapper("test/morphology/test.h5")
                morphio_api.morphio_load(cell, m)

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
            ``m = morphio_api.MorphIOWrapper(morph_file)``

        Description:
            Python binding for ``neuron::morphology::MorphIOWrapper``. Holds the loaded MorphIO morphology and provides access to the generated HOC commands and section metadata.

    .. tab:: HOC

        Description:
            ``MorphIOWrapper`` is available through the Python ``neuron.morphio_api`` submodule. In HOC, use :func:`morphio_load` or ``morphio_read``.

.. method:: MorphIOWrapper.to_hoc

    .. tab:: Python

        Syntax:
            ``cmds = m.to_hoc()``

        Description:
            Returns the list of HOC commands that instantiate the morphology sections, subsets, soma 3D points, connections, and neurite 3D points.

.. data:: MorphIOWrapper.section_index_to_name

    .. tab:: Python

        Syntax:
            ``names = m.section_index_to_name``

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
