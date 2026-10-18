.. _MorphIO_python:

MorphIO (Python)
================

MorphIO is a library for reading and writing neuron morphology files.
It supports the following formats: SWC, ASC (aka. Neurolucida) and H5 v1.

We use MorphIO to provide a C++ implementation that will match Import3D_GUI HOC stack.
This will allow us to read neuron morphology files in a variety of formats, with an increased performance, supporting single-point somas (``SOMA_SINGLE_POINT``), simple contour somas (``SOMA_SIMPLE_CONTOUR``), NeuroMorpho three-point cylinder somas (``SOMA_NEUROMORPHO_THREE_POINT_CYLINDERS``), and cylinder/frustum stack somas (``SOMA_CYLINDERS``).

There will also be a backward-compatible layer that will allow us to use the HOC stack together with MorphIO.
The implementation is located in the ``MorphIO.hoc`` file from the HOC library.
Finally we will be able to update the Import3D_GUI HOC stack to use MorphIO so that CellBuilder/Import3d can use it.

MorphIO is not available for Windows, because further support is required in MorphIO for the MinGW toolchain.

.. _morphio_python_api:

MorphIO Python API
==================

The ``morphio_api`` submodule provides a Python interface to the MorphIO morphology loading,
which is used to load neuronal morphology data.

MorphIO Python Functions
------------------------

.. py:function:: morphio_load(obj, morph_file)
    :noindex:

    Load morphology data from a morphology file.

    :param obj: A NEURON Cell object.
    :param morph_file: A file path that contains morphology data.

.. py:function:: morphio_load(obj, MorphIOWrapper)
    :noindex:

    Load morphology data from a morphology MorphIO wrapper.

    :param obj: A NEURON Cell object.
    :param MorphIOWrapper: A MorphIOWrapper object (see below).

MorphIO Python Classes
----------------------

.. py:class:: MorphIOWrapper
    :noindex:

    A Python binding for the MorphIOWrapper. This holds the MorphIO object and provides utility functions to access the data.

    .. py:method:: __init__(self, path_or_file)
        :noindex:

        Initialize the ``MorphIOWrapper`` object with a file path that contains morphology data.

    .. py:method:: to_hoc(self)
        :noindex:

        Get the HOC morphology as HOC commands.

    .. py:attribute:: section_index_to_name
        :noindex:

        Get a list of strings that represents the names of the sections in the morphology data ordered by section ids.

        An example:

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

        :return: A list of section name strings.

    .. py:attribute:: section_type_distribution
        :noindex:

        Get a list of tuples that represent the distribution of section type IDs in the morphology data.

        A distribution looks like:

        .. code-block:: text

            # This will hold the distribution that will map the different type ids to the
            # start id with regard to section types and their count. For example, axon type(2)
            # starts at section id 0 and totals 2724 sections.

                | type_id | start_id  |   count   |
                | ------- | --------- | --------- |
                |      1  |       -1  |        1  |
                |      2  |        0  |     2724  |
                |      3  |     2724  |       75  |

        :return: A list of ``(type_id, start_id, count)`` tuples.
