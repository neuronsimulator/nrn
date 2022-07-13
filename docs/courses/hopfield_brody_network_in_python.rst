.. _hopfield_brody_network_in_python:

Hopfield Brody synchronization (sync) model
==========

The exercises below are intended primarily to familiarize the student with techniques and tools useful for the implementation of networks in NEURON. We have chosen a relatively simple network in order to minimize distractions due to the complexities of channel kinetics, dendritic trees, detailed network architecture, etc. The following network uses an artificial integrate-and-fire cell without channels or compartments. There is only one kind of cell, so no issues of organizing interactions between cell populations. There is only one kind of synapse. Additionally, suggested algorithms were chosen for ease of implementation rather than quality of results.

Although this is a minimal model, learning the ropes is still difficult. Therefore, we suggest that you go through the entire lesson relatively quickly before returning to delve more deeply into the exercises. Some of the exercises are really more homework projects (eg design a new synchronization measure). These are marked with asterisks.

As you know, NEURON is optimized to handle the complex channel and compartment simulations that have been omitted from this exercise. The interested student might wish to convert this network into a network of spiking cells with realistic inhibitory interactions or a hybrid network with both realistic and artificial cells. Such an extended exercise would more clearly demonstrate NEURON's advantages for performing network simulations.

Standard intfire implementation (eg :hoc:class:`IntFire1` from ``intfire1.mod``))
---------------------

Individual units are integrate-and-fire neurons.

The basic intfire implementation in neuron utilizes a decaying state variable (``m`` as a stand-in for voltage) which is pushed up by the arrival of an excitatory input or down by the arrival of an inhibitory input (``m = m + w``). When m exceeds threshold the cell "fires," sending events to other connected cells.

.. code::
    python

    if (m>1) {  ...


        net_event(t)    : trigger synapses 


IntIbFire in sync model
---------

The integrate-and-fire neuron in the current model must fire spontaneously with no input, as well as firing when a threshold is reached. This is implemented by utilizing a ``firetime()`` routine to calculate when the state variable m will reach threshold **assuming no other inputs during that time**. This firing time is calculated based on the natural firing interval of the cell (``invl``) and the time constant for state variable decay (``tau``). When an input comes in, a new firetime is calculated after taking into account the synaptic input (``m = m + w``) which perturbs the state variable's trajectory towards threshold.

Cell template
--------

IntIBFire is enclosed in a template named "Cell." An instantiation of this template provides access to the underlying mechanism through object pointer pp. Execute the following:

.. code::
    python

    >>> mycell = Cell()
    >>> print mycell.pp, mycell.pp.tau, mycell.pp.invl

The Cell template also provides 3 procedures. ``connect2target()`` is optionally used to hook this cell to a postsynaptic cell.

Network
-------

The network has all-to-all inhibitory connectivity with all connections set to equal negative values. The network is initially set up with fast firing cells at the bottom of the graph (``Cell[0]``, highest natural interval) and slower cells at the top (``Cell[ncell-1]``, lowest natural interval). Cells in between have sequential evenly-spaced periods.

How it works
=============

The synchronization mechanism requires that all of the cells fire spontaneously at similar frequencies. It is obvious that if all cells are started at the same time, they will still be roughly synchronous after one cycle (since they have similar intrinsic cycle periods). After two cycles, they will have drifted further apart. After many cycles, differences in period will be magnified, leading to no temporal relationship of firing.

The key observation utilized here is that firing is fairly synchronized one cycle after onset. The trick is to reset the cells after each cycle so that they start together again. They then fire with temporal differences equal to the differences in their intrinsic periods. This resetting can be provided by an inhibitory input which pushes state variable *m* down far from threshold (hyperpolarized, as it were). This could be accomplished through an external pacemaker that reset all the cells, thereby imposing an external frequency onto the network. The interesting observation in this network is that pacemaking can also be imposed from within, though an intrinsic connectivity that enslaves all members to the will of the masses.

Exercises to gain familiarity with the model
----------

Increase to 100 neurons and run.
++++++++++

Many neurons do not fire. These have periods that are too long -- before they can fire, the population has fired again and reset them. Notice that the period of network firing is longer than the natural periods of the individual cells. This is because the threshold is calculated to provide this period when m starts at 0. However, with the inhibition, m starts negative.


Narrow the difference between fast and slow cells so as to make more of them fire.
++++++++++

Alternatively, increase the delay.

Reduce the inhibition and demonstrate that synchrony worsens.
++++++++++

With inhibition set to zero, there is no synchrony and each cell fires at its natural period.

Increase cell time constant.
++++++++++

This will destroy synchrony. Increase inhibitory weight; synchrony recovers. This is a consequence of the exponential rise of the state variable. If the interval is short but the time constant long, then the cell will amplify small variations in the amount of inhibition received.

Beyond the GUI -- Saving and displaying spikes
=============

Spike times are being saved in a series of vectors in a template: ``sp.vecs[0]`` .. ``sp.vecs[ncell-1]``
++++++++++

Count the total number of spikes using a ``for`` loop and ``total+=len(sp.vecs[ii])``
 

We will instead save spike times in a single vector (``tvec``), using a second vector (``ind``) for indices
++++++++++
.. code::
    python

    >>> savspks() # record spike times to tvec; indices to ind
    >>> h.run() # or hit run button on GUI
 

Make sure that the same number of spikes are being saved as were saved in sp.vecs[]
++++++++++

.. code::
    python

    >>> print ind.size(), tvec.size()

Wise precaution -- check step by step to make sure that nothing's screwed up

Can use for ... : ``vec.append(sp.vecs[ii]); vec.sort(); tvec.sort(); vec.eq(tvec)`` to make sure have all the same spike times (still doesn't tell you they correspond to the same cells)

Graph spike times -- should look like SpikePlot1 graph
----------

.. code::
    python

    >>> g = h.Graph()
    >>> ind.mark(g,tvec) # throw them up there
    >>> showspks() # fancier marking with sync lines

Synchronization measures
=============

Look at synchronization routine
++++++++++

.. code::
    python

    >>> syncer()
    >>> for w in np.arange(0,-5e-2,-5e-3): weight(w); h.run(); print w,syncer()
 

Exercise*: write (or find and implement) a better synchronization routine
++++++++++
 

Graph synchronization
++++++++++

.. code::
    python

    >>>  [vec.resize(0) for vec in veclist]        # clear
    >>>  for w in np.arange(0, -5e-2, -5e-3):
            weight(w) 
            h.run() 
            vec[1].append(w) 
            vec[2].append(syncer())
    >>>  print vec[1].size(), vec[2].size()         # make sure nothing went wrong
    >>>  g.erase()                     # assuming it's still there, else put up a new one
    >>>  vec[2].line(g,vec[1])         # use "View = plot" on pull down to see it
    >>>  vec[2].mark(g,vec[1],"O",8,2,1)         # big (8) red (2) circles ("O") 


**Make sure that the values graphed are the same as the values printed out before**


Exercises
+++++++++

- enclose the weight exploration above in a procedure

- write a similar routine to explore cell time constant (``param`` is called ``ta``; set with ``tau(ta)``); run it

- write a similar routine to explore synaptic delay (``param`` is called ``del``; set with ``delay(del)``); run it

- write a general proc that takes 3 args: ``min``, ``max``, ``iter`` that can be used to explore any of the params

(hint: call a general ``setpar()`` procedure that can be redefined eg def ``setpar(x): weight(x)`` depending on which param you are changing


Procedure interval2() in ocomm.hoc sets cell periods randomly
=========

- can be used instead of ``interval()`` in :download:`synchronize.hoc <data/synchronize.hoc>`

- randomizing cell identities is easier than randomizing connections

- with randomized identities can attach cell 0 to cells 1-50 and not have interval uniformity

- To replace ``interval()`` with ``interval2()``, overwrite ``interval()``:

    .. code::
        python

        >>> def interval (x,y): interval2(x,y)
        
- Run ``interval()`` from command line or by changing low and high in GUI panel

- Check results

    .. code::
        python

        >>> for ii in range(ncell): h.printf("%g ",cells[ii].pp.invl)

- Exercise: check results graphically by setting wt to 0, running sim, and graphing results

Rewiring the network
=========

All of the programs discussed in the lecture are available in ocomm.hoc. The student may wish to use or rewrite any of these procedures. Below we suggest a different approach to wiring the network.
 
procedure wire() in :file:`ocomm.hoc` is slightly simplified from that in :file:`synchronize.hoc` but does the same thing


.. code::
    python

    def wire ():
        nclist.remove_all()
        for pre in cells: 
            for post in cells: 
                if pre!=post: 
                    nclist.append(h.NetCon(pre.pp,post.pp)) 


Exercises
--------

rewrite ``wire()`` to connect each neuron to half of the neurons


suggestion: for each neuron, pick an initial projection randomly

eg

.. code::
    python

    rdm.discunif(0,ncell-1) 
    proj=rdm.repick()
    if proj < ncell/2:
        # project to 0->proj 
    else: # project to proj->ncell-1 


This algorithm is not very good since cells in center get more convergence

- rewrite wire to get even convergence

suggestions: counting upwards from ``proj``, use modulus (%) to wrap-around and get values between 0 and ncell-1

- ``run()``, graph and check synchrony

- generalize your procedure to take argument ``pij`` that defines connection density

- assess synchrony at different connection densities

Assessing connectivity
==========

- ``cvode.netconlist(cells.object(0).pp,"","")`` gives a divergence list for cell#0

- ``cvode.netconlist("","",cells.object(0).pp)`` gives a convergence list for cell#0

- Exercise: use these lists to calculate average, min, max for conv and div


Graphing connectivity
==============

- use ``fconn(prevec,postvec)`` to get parallel vecs of pre and postsyn cell numbers

- use ``postvec.mark(g,prevec)`` to demonstrate that central cells get most of the convergence (if using original suggestion for ``wire()`` rewrite)

- use ``showdiv1(cell#)`` and ``showconv1(cell#)`` to graph connections for selected cells

- Exercise: write a procedure to count, print out and graph all cells with reciprocal connectivity, eg A->B and B->A

- Exercise: modify your ``wire()`` to eliminate reciprocal connectivity

Animate
========

- Use ``animplot()`` to put up squares for animating simulation

- Resize and shift around as needed but afterwards make sure that "Shape Plot" is set on pulldown menu

- After running a simulation to set tvec and ind, run anim() to look at the activity

Difficult or extended exercises

*Last updated: Jun 16, 2015 (9:34)*
