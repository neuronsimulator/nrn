Longitudinal Diffusion
======================

The idea behind ``LONGITUDINAL_DIFFUSION`` is to allow a ``STATE`` variable to
diffuse along a section, i.e. from one segment into a neighbouring segment.

This problem is solved by registering callbacks. In particular, NEURON needs to
be informed of the volume and diffusion rate. Additionally, the implicit
time-stepping requires information about certain derivatives.

Implementation in NMODL
-----------------------

The following ``KINETIC`` block

  .. code-block::

    KINETIC state {
        COMPARTMENT vol {X}
        LONGITUDINAL_DIFFUSION mu {X}

        ~ X << (ica)
    }

Will undergo two transformations. The first is to create a system of ODEs that
can be solved. This consumed the AST node. However, to print the code for
longitudinal diffusion we require information from the ``COMPARTMENT`` and
``LONGITUDINAL_DIFFUSION`` statements. This is why there's a second
transformation, that runs before the other transformation, to extract the
required information and store it a AST node called
``LONGITUDINAL_DIFFUSION_BLOCK``. This block can then be converted into an
"info" object, which is then used to print the callbacks.

