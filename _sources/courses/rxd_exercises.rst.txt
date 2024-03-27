.. _rxd_exercises:

Reaction-Diffusion Exercises
============================

Exercise 1
----------

Consider the signaling molecule IP3, which in Wagner et al 2004 was modeled as having a diffusion coefficient in the cytosol of 0.283 μm\ :sup:`2`/ms. (Our units here are not those usually used by cell biologists, but they are the ones used by NEURON’s reaction-diffusion module as they are reasonable for neuronal electrophysiology.)

Consider a 101 micron long section of dendrite discretized into 101 compartments. Set IP3 at an initial concentration of 1 μM for 0.4 ≤ seg.x ≤ 0.6 and 0 elsewhere. (Be careful about units.) How long does it take the concentration at seg.x = 0.7 to rise to a concentration of 100 nM? What is the peak concentration obtained at this point over all time? What is the limiting value as :math:`t \to \infty`?

In 25 °C water, calcium has a diffusion coefficient of :math:`1.97 \times 10^{-5}` cm\ :sup:`2`/s according to `physiologyweb <http://www.physiologyweb.com/calculators/diffusion_time_calculator.html>`_. What is this in NEURON's units of μm\ :sup:`2`/ms? (Note: calcium will have a different effective diffusion constant in a cell than in water due to buffering molecules, etc.) Answer the question about time for a concentration increase at seg.x = 0.7 for calcium in water. Do the same for glucose which has a diffusion coefficient of :math:`6 \times 10^{-6}` cm\ :sup:`2`/s according to the same source. How does varying the diffusion coefficient affect the time it takes to raise concentration at a given distance by a given amount?

 

Exercise 2
----------

Do the same pulse-diffusion calcium simulation, but on a cylindrical domain with 5 cylindrical spines 3 microns long and 1 micron in diameter. How does the addition of spines affect the diffusion? Why?

How many molecules of calcium would be present in such a spine if it had an average concentration of 100 nM, a biological plausible concentration? (Hint: one cubic micron at 1mM concentration has about 602,200 molecules.) If one molecule stochastically entered or left the spine, what would be the percentage change in the spine’s concentration?

 

Exercise 3
----------

Import a CA1 pyramidal cell into NEURON from `NeuroMorpho.Org <https://neuromorpho.org>`_. Declare chemical species ``X`` to diffuse across the entire cell at D=1 μm\ :sup:`2`/ms with initial concentration of 1 mM in the soma and 0 elsewhere. View the distribution of ``X`` at 5 and 25 ms on both a ShapePlot and along the apical as a function of distance from the soma. Plot a time series of the concentration of ``X`` at the center of the soma.

 

Exercise 4
----------

Neurons are well-known for their propagating action potentials, but electrical signalling is not their only form of regenerative signaling. `Berridge 1998 <https://doi.org/10.1016/S0896-6273(00)80510-3>`_ described neuronal calcium dynamics as functioning as a "neuron-within-a-neuron."

Why is pure diffusion insufficient? What is the expected time for a molecule of IP3 to travel a distance of 50 µm? 100 µm? 1 meter (the approximate length of the longest axon in a human)?

Hint: :math:`E[t] = x^2 / (2 D)`

The actual dynamics underlying these calcium wave dynamics is complicated, so let’s start with the simplest reaction-diffusion model of a propagating wave:

On a 500 µm long piece of dendrite add a species ``Y`` with diffusion constant 1 changing by an :class:`rxd.Rate` of ``scale * (0 - y) * (alpha - y) * (1 - y)``. For a first test take ``scale = 1`` and ``alpha = 0.3``. Set initial conditions such that ``Y`` is 1 where seg.x < 0.2 and 0 elsewhere. Plot the concentration of ``Y`` vs position at several time points to show the development of a traveling wave front.

Run the same simulation with alpha = 0.7. What changes? More generally, how does alpha affect the wave?

Calculate the speed of propagation of the wave front at alpha = 0.3. How can you change scale and the diffusion coefficient to get a wave of the same shape that travels at 50 microns per second? What happens to the shape of the wave front if the diffusion coefficient is increased but the other parameters stay the same?

Things to consider: How do you know if you are using an appropriate value of nseg or dt?

 

References
----------

Berridge, M. J. (1998). Neuronal calcium signaling. Neuron, 21(1), 13-26.

Wagner, J., el al. (2004). A wave of IP 3 production accompanies the fertilization Ca 2+ wave in the egg of the frog, Xenopus laevis: theoretical and experimental support. Cell Calcium, 35(5), 433-447.