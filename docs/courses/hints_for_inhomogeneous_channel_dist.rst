.. _hints_for_inhomogeneous_channel_distribution:

Overview of the Tasks
=====================

1.
    Specify the

        - subset
        - distance metric ``p``
        - parameter ``param`` that depends on distance
        - function ``f`` that governs the relationship between the parameter and the distance metric

2.
    Verify the implementation.

Details
=======

This involves a lot of steps--remember to save intermediate results to session files!

**Act 1: Specify the subset**

On the CellBuilder's Subsets page, create the subset.

**Act 2: Specify the distance metric**

Do this with a SubsetDomainIterator.

- Create a SubsetDomainIterator.

    - Click on the subset, then click on "Parameterized Domain Page"
    - Click on "Create a SubsetDomainIterator".

    The name of the SubsetDomainIterator will appear in the middle panel of the CellBuilder. It will be called name_x, where *name* is the name of the subset.

- Use the SubsetDomainIterator to specify the distance metric.

    - Click on name_x.

    The right panel of the CellBuider will show the controls for specifying the distance metric. Default is path length in µm from the root of the cell (0 end of soma).

    Drag the slider back and forth to see the corresponding location(s) in the shape plot (boundary between red and black). Distance from the root to the red-black boundary is shown on the canvas as "p=nnn.nnn". You can also click on the canvas near the shape and drag the cursor back and forth; the canvas will now show the "range" of the boundary in the nearest section, and the name of that section.

    - **metric** offers three choices: path length from root, radial (euclidian) distance from root, and "projection onto line" (distance from a plane that passes through the root and is orthogonal to the principal axis of the root section).

    - **proximal** allows specifying an "offset" for the proximal end(s) of the subset. This lets you assign a distance of 0 to the origin of the apical tree.

    - **distal** allows specifying whether or not to normalize the distance metric, and if normalized, whether the metric is to become 1 only at the most distal end, or at all distal ends.


**Act 3: Specify the parameters that depend on distance**

Do this on the CellBuilder's Biophysics page.

- Select Strategy (make a check mark appear in the box).

    name_x (the name of the SubsetDomainIterator) will appear in the middle panel.

- Click on name_x.

- In the right panel of the CellBuider, select the parameters that the SubsetDomainIterator will control.

**Intermezzo**

Time to step back and see where we are.

At this point, name_x "knows" these things:

    - the sections that are in its subset

    - the distance metric for each spatial location in these sections

    - the parameters in these sections that will be governed by this distance metric

But it doesn't know the functional relationships between the parameters and the distance metric--and each parameter can have its own function. Defining these functions is the next item to take care of.

**Act 4: Specify the functional relationship between each parameter and the distance metric**

- Clear the Strategy checkbox.

    name_x will appear in the middle panel of the CellBuilder, and beneath it will be the names of each of the paramters that were selected in the strategy.

- Click on one of the parameter names.

    The right hand panel presents controls for specifying f.

    The default is a Boltzmann function. **f(p)** offers this and three other choices:

        - **Ramp** (linear)

        - **Exponential**

        - **New** lets you enter your own function

    Each of these has its own list of user-settable parameters.

    **show** allows you to bring up a graph or shape plot for visual confirmation of how the biophysical parameter varies in space. This is convenient, but you'll probably want to check the finished model with Model View.

**Finale**

Turn on Continuous Create, and use Model View to verify the channel distributions.











