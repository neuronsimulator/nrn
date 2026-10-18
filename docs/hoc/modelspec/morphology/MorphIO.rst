.. _MorphIO_hoc:

MorphIO (HOC)
=============

MorphIO is a library for reading and writing neuron morphology files.
It supports the following formats: SWC, ASC (aka. Neurolucida) and H5 v1.

We use MorphIO to provide a C++ implementation that will match Import3D_GUI HOC stack.
This will allow us to read neuron morphology files in a variety of formats, with an increased performance, supporting single-point somas (``SOMA_SINGLE_POINT``), simple contour somas (``SOMA_SIMPLE_CONTOUR``), NeuroMorpho three-point cylinder somas (``SOMA_NEUROMORPHO_THREE_POINT_CYLINDERS``), and cylinder/frustum stack somas (``SOMA_CYLINDERS``).

There will also be a backward-compatible layer that will allow us to use the HOC stack together with MorphIO.
The implementation is located in the ``MorphIO.hoc`` file from the HOC library.
Finally we will be able to update the Import3D_GUI HOC stack to use MorphIO so that CellBuilder/Import3d can use it.

MorphIO is not available for Windows, because further support is required in MorphIO for the MinGW toolchain.

.. _morphio_hoc_api:

MorphIO HOC API
===============

.. hoc:function:: morphio_load(obj, morph_file[, write_hoc])
    :noindex:

    Loads a morphology from a file and creates a HOC representation of it.
    The HOC representation is executed in the Cell object's context.

    This function requires a Cell object and a path to the morphology file.

    If the optional third argument is set to 1, the HOC representation of the morphology will be written to a file in the same directory as the morphology file.

    :param obj: A NEURON Cell object.
    :param morph_file: A file path that contains morphology data.
    :param write_hoc: Optional flag (default 0); when set to 1, writes the generated HOC commands to ``<stem>.hoc``.
    :return: 1 if the morphology was loaded successfully, 0 otherwise.

    Raises
    ------
    - ``hoc_execerror``: if the Cell object or the path to the morphology file are not provided, or if the morphology loading failed.

.. hoc:function:: morphio_read(obj, morph_file)
    :noindex:

    Convenience wrapper defined in ``MorphIO.hoc`` that calls ``morphio_load(obj, morph_file, 1)``.

.. hoc:class:: MorphIOWrapper
    :noindex:

    Object-oriented HOC template defined in ``MorphIO.hoc`` providing an interface similar to ``Import3d_GUI``. Also accessible via the alias template ``MorphIO``.

    .. code-block:: hoc

        load_file("MorphIO.hoc")

        objref morph, cell
        cell = new HocBasicCell()
        morph = new MorphIOWrapper("path/to/neuron.swc")
        morph.instantiate(cell)

    .. hoc:method:: instantiate(cell[, write_hoc])
        :noindex:

        Instantiates the morphology into the provided Cell object. When optional second argument ``write_hoc`` is set to 1, writes the generated HOC commands to ``<stem>.hoc``.
