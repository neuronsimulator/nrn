.. _MorphIO:

MorphIO
=======

MorphIO is a library for reading and writing neuron morphology files.
It supports the following formats: SWC, ASC (aka. neurolucida) and H5 v1.

We use MorphIO to provide a C++ implementation that will match Import3D_GUI HOC stack.
This will allow us to read neuron morphology files in a variety of formats, with an increased performance.

There will also be a backward compatible layer that will allow us to use the HOC stack together with MorphIO.
Finally we will be able to update the Import3D_GUI HOC stack to use MorphIO so that CellBuilder/Import3d can use it. 

Morphio is not available for Windows, because further support is required in MorphIO for the MinGW toolchain.

.. _morphio_api:

MorphIO Python API
===============


The `morphio_api` submodule provides a Python interface to the MorphIO morphology loading,
which is used to load neuronal morphology data.

Functions
---------

.. py:function:: morphio_load(obj, morph_file)

    Load morphology data from a morphology file.

    :param obj: A NEURON Cell object.
    :param morph_file: A file path that contains morphology data.

.. py:function:: morphio_load(obj, MorphIOWrapper)

    Load morphology data from a morphology MorphIO wrapper.

    :param obj: A NEURON Cell object.
    :param MorphIOWrapper: A MorphIOWrapper object (see below).

Classes
-------

.. py:class:: MorphIOWrapper

    A Python binding for the MorphIOWrapper. This holds the MorphIO object and provides utility functions to access the data.

    .. py:method:: __init__(self, path_or_file)

        Initialize the `MorphIOWrapper` object with a file path or a Python object that contains morphology data.

    .. py:method:: morph_as_hoc(self)

        Get the HOC morphology as HOC commands.

    .. py:function:: sec_idx2names(self)

        Get a reference to a vector of strings that represents the names of the sections in the morphology data ordered by section ids.

        An example: 

        .. code-block::

            "soma",     # section id 0
            "axon[0]",  # section id 1
            "axon[1]",  # section id 2
            "axon[2]",  # section id 3 
            "dend[0]",  # section id 4 
            "dend[1]",  # section id 5 
            "dend[2]"   # section id 6

        :return: A reference to a vector of strings.

    .. py:function:: sec_typeid_distrib(self)

        Get a reference to a vector of tuples that represent the distribution of section type IDs in the morphology data.
        
        A distribution looks like: 
        .. code-block::

            # This will hold the distribution that will map the different type ids to the
            # start id with regards to to section types and their count. For example, axon type(2)
            # starts at section id 0 and totals 2724 sections.

                | type_id | start_id  |   count   |
                | ------- | --------- | --------- |
                |      1  |       -1  |        1  |
                |      2  |        0  |     2724  |
                |      3  |     2724  |       75  |

        :return: A reference to a vector of tuples.
