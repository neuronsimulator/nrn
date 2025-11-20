Only plot dendrites and soma in a Shape plot
=============================================

How can I create a Shape plot that displays only the soma and dendrites, excluding axons?

You can achieve this by creating a `SectionList` containing only the dendrite and soma sections you want to visualize, then use a `Shape` object to plot only those sections. The `Shape`'s `observe()` method restricts the plot to the specified sections.

Example in Hoc:

.. code-block:: hoc

    objref dendrite_and_soma
    dendrite_and_soma = new SectionList()
    // Append dendrite and soma sections by name or by looping over sections
    dendrite_and_soma.append(&soma)
    // example for multiple dendrites:
    forall if (secname() =~ "dend") dendrite_and_soma.append()

    objref myshape
    myshape = new Shape() // create Shape object for the entire cell
    myshape.observe(dendrite_and_soma) // limit plot to dendrites and soma
    myshape.exec_menu("View = plot") // display the shape plot window

Example in Python:

.. code-block:: python

    from neuron import h
    h.load_file("stdrun.hoc")

    # Create a SectionList containing soma and dendrites
    dendrite_and_soma = h.SectionList()
    dendrite_and_soma.append(h.soma)
    for sec in h.allsec():
        if "dend" in sec.name():
            dendrite_and_soma.append(sec)

    # Create Shape and observe only selected sections
    myshape = h.Shape()
    myshape.observe(dendrite_and_soma)
    myshape.exec_menu("View = plot")

This method allows you to customize the visualization by including only desired parts of the neuron morphology in the Shape plot.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=2708
