.. _secspec:

.. _CurrentlyAccessedSection:

Section and Segment Selection
=============================

Dot notation for properties
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Since sections share property names eg. a length called :data:`L` 
it is always necessary to specify *which* section is being discussed;
this is done using dot notation, e.g.

.. code-block::
    python

    dendrite[2].L = dendrite[1].L + dendrite[0].L 
    axon.v = soma.v 
    print(soma.gnabar)
    axon.nseg *= 3

This notation is necessary when one needs to refer to more than 
one section within a single statement. 

sec= for functions
~~~~~~~~~~~~~~~~~~

Many NEURON functions refer to a specific Section. In recent versions of NEURON,
most of these either are available as section methods or take a section or segment
directly. For older code or for the remaining exceptions, the active section may
be specified using a ``sec=`` keyword argument.

For example:

.. code-block::
    python

    my_iclamp = h.IClamp(0.25, sec=soma)   # better to use h.IClamp(soma(0.25)) though
    num_pts_3d = h.n3d(sec=apical)         # could get the same value as an int via apical.n3d()
 

Default section
~~~~~~~~~~~~~~~

If no ``sec=`` keyword argument is specified, the functions will use NEURON's
default Section (sometimes called the *currently accessed section*),
which can be identified via ``h.cas()``.
The default Section is controlled by a stack; it is initially
the first Section created but entries may be pushed onto or popped off of the
stack by the following commands. *Use this only as a last resort.*


.. function:: pop_section


    Syntax:
        ``h.pop_section()``


    Description:
        Take the currently accessed section off the section stack. This can only be used after 
        a function which pushes a section on the section stack such as 
        ``point_process.getloc()``. 

    Example:

        .. code-block::
            python

            from neuron import h
            
            soma = h.Section(name='soma')
            apical = h.Section(name='apical')
            stims = [h.IClamp(soma(i / 4.)) for i in range(5)] + [h.IClamp(apical(0.5))]
            for stim in stims: 
                x = stim.get_loc() 
                print(f"location of {stim} is {h.secname()}({x})")
                h.pop_section() 
            
        (Note: in this example as ``nseg=1``, the current clamps will either be at position 0, 0.5, or 1.)

        (Note: a more Pythonic way to get the location of the point-process ``stim`` is to use ``seg = stim.get_segment()``,
        but this is shown as an example of using ``h.pop_section()``.)


         

----



.. function:: push_section


    Syntax:
        ``h.push_section(number)``

        ``h.push_section(section_name)``


    Description:
        This function, along with ``h.pop_section()`` should only be used as a last resort. 
        It will place a specified section on the top of the section stack, 
        becoming the current section to which all operations apply. It is 
        probably always better to use :class:`SectionRef` 
        or :class:`SectionList` . 


        :samp:`push_section({number})` 
            Push the section identified by the number returned by 
            ``h.this_section()``, etc. which you desire to be the currently accessed 
            section. Any section pushed must have a corresponding ``h.pop_section()``
            later or else the section stack will be corrupted. The number is 
            not guaranteed to be the same across separate invocations of NEURON. 

        :samp:`push_section({section_name})`
            Push the section identified by the name obtained 
            from sectionname(*strdef*). Note: at this time the implementation 
            iterates over all sections to find the proper one; so do not use 
            in loops. 


    Example:

        .. code-block::
            python

            from neuron import h

            soma = h.Section(name='soma')
            apical = h.Section(name='apical')

            # get a number to allow pushing by number
            soma_id = h.this_section(sec=soma)

            # push by name
            h.push_section('apical')

            # push by number
            h.push_section(soma_id)

            # RuntimeError -- no such section
            h.push_section('basal')


    .. seealso::
        :class:`SectionRef`

         
         

