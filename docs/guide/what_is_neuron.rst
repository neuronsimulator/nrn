What is NEURON
==============

NEURON:

* is a flexible and powerful simulator of neurons and networks
* has important advantages over general-purpose simulators
* helps users focus on important biological issues rather than purely computational concerns
* has a convenient user interface
* has a user-extendable library of biophysical mechanisms
* has many enhancements for efficient network modeling
* offers customizable initialization and simulation flow control
* is widely used in neuroscience research by experimentalists and theoreticians
* is well-documented and actively supported
* is free, open source, and runs on (almost) everything

A flexible and powerful simulator of neurons and networks
---------------------------------------------------------

NEURON is a simulation environment for modeling individual neurons and networks of neurons. It provides tools for conveniently building, managing, and using models in a way that is numerically sound and computationally efficient. It is particularly well-suited to problems that are closely linked to experimental data, especially those that involve cells with complex anatomical and biophysical properties.

Advantages over general-purpose simulators
------------------------------------------
NEURON had its beginnings in the laboratory of John W. Moore at Duke University, where he and Michael Hines started their collaboration to develop simulation software for neuroscience research. It has benefited from judicious revision and selective enhancement, guided by feedback from the growing number of neuroscientists who have used it to incorporate empirically-based modeling into their research strategies.

NEURON's computational engine employs special algorithms that achieve high efficiency by exploiting the structure of the equations that describe neuronal properties. It has functions that are tailored for conveniently controlling simulations, and presenting the results of real neurophysiological problems graphically in ways that are quickly and intuitively grasped. Instead of forcing users to reformulate their conceptual models to fit the requirements of a general purpose simulator, NEURON is designed to let them deal directly with familiar neuroscience concepts. Consequently, users can think in terms of the biophysical properties of membrane and cytoplasm, the branched architecture of neurons, and the effects of synaptic communication between cells.

Separates biology from purely computational concerns
----------------------------------------------------

From its inception, one of NEURON's design goals has been to help modelers address high-level neuroscience research questions without being distracted by low-level mathematical or computational issues. Here are some of the strategies that help it approach this goal.

Working with familiar idioms
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The most important strategy is to offer users what might be called "natural syntax," which allows them to specify model properties in familiar idioms, rather than having to cast kinetic schemes or differential equations in the form of statements in C or some other generic programming language. Perhaps the most prominent example of natural syntax in NEURON is the notion of a section, which is a continuous unbranched cable (directly analogous to an unbranched neurite). Sections can be connected together to form branched trees, and are endowed with properties that can vary continuously with position along their lengths. Sections let investigators represent neuronal anatomy without having to wrestle with the cable equation. They also easily lend themselves to manipulation by graphical tools, such as the CellBuilder, for building and managing models. Other examples of natural syntax are seen in the NMODL programming language, and in the Linear Circuit Builder and Channel Builder (see User-extendable library of biophysical mechanisms below).

Integrator-independent model specification
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

NEURON offers several different, user-selectable numerical integration methods.

* The default integration method is implicit Euler, which provides robust stability and first order accuracy in time (sufficient for most applications).
* There is also a Crank-Nicholson method that provides second order accuracy at little additional computational cost. However, this is prone to numerical oscillations if dt is too long, voltage clamps are present, or system states are described by algebraic equations.
* Increased accuracy, faster run times, and sometimes both, may be achieved by choosing adaptive integration, which adjusts integration order and time step as necessary to satisfy a local error criterion. For historical reasons, the adaptive integrators are genericallly called "CVODE" in NEURON; the actual method is either IDA (Hindmarsh and Taylor, 1999) or CVODES (Hindmarsh and Serban, 2002), a decision that is made automatically (i.e. without requiring user judgement) depending on whether or not a model involves states that are described by algebraic equations.

Users can switch between these integration methods without having to rewrite the model specification because NEURON avoids computation-specific representations of biological properties. This convenience is essential because deciding which method is best in any particular situation is often an empirical question. Further details about numeric integration in NEURON are provided in chapter 4 of The NEURON Book (Carnevale and Hines, 2006).

Efficient and painless spatial and temporal discretization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Another important strategy is to allow users to defer or avoid the explicit specification of spatial and temporal discretization. NEURON's approach to spatial discretization, which sets it apart from other modeling tools, eliminates preoccupation with compartments. This approach has two components:

1. Each section has its own discretization parameter nseg, which governs the number of internal points at which the discretized form of the cable equation is integrated, and can be changed after the anatomical and biophysical properties of the model have been specified.
2. Parameters and variables that are spatially nonuniform are described in terms of normalized distance from one end of the section, e.g. soma.v(0.5) means the membrane potential midway between the 0 and 1 ends of the soma section.

In addition, NEURON provides tools that can automatically take care of discretization. Numerical simulation of models of biological neurons can pose a special challenge because such models tend to include mechanisms that span a wide range of spatial and temporal scales, with state variables that involve a wide range of magnitudes. With the CellBuilder, users may apply the d_lambda rule (Hines and Carnevale 2001; p. 122 et seq. in Carnevale and Hines 2006), to automatically assign values of nseg based on the model's anatomical and biophysical properties. The d_lambda rule can also be applied via hoc code. Similarly, the error tolerances that govern time step size (and integration order) during adaptive integration can be adjusted automatically by the GUI's Absolute Tolerance Scale Tool, which is part of the VariableTimeStep tool.

Convenient user interface
-------------------------

The interpreters
~~~~~~~~~~~~~~~~

NEURON offers users a choice of programming languages. Most programming has been done done with hoc, an interpreted language with C-like syntax (Kernighan and Pike, 1984) that has been extended to include functions specific to the domain of modeling neurons, implementing a graphical interface, and object-oriented programming. More recently, Python was adopted as an alternative interpreter. All hoc variables, procedures, and functions can be accessed from Python, and vice-versa. This provides a tremendous degree of flexibility for setting up the anatomical and biophysical properties of models, defining the appearance of the graphical interface, controlling simulations, and plotting results.

Users can take advantage of multiprocessors or workstation clusters in order to accelerate embarrassingly parallel problems, such as optimization and parameter space exploration. Furthermore, network models, and complex models of a single neuron, can be distributed over multiple processors to achieve nearly linear speedup.

The graphical user interface
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The default graphical user interface can be used to create and exercise models that have a wide range of complexity. With the GUI, it is possible to generate publication-quality results without having to write any program code at all. However, the most flexible and powerful strategy is to combine hoc programming and the GUI in order to exploit the strengths of both approaches.

Many GUI tools are designed to be analogous to biology and lab instrumentation, and are very convenient for initial exploratory simulations, setting parameters, common control of voltage and current stimuli, and graphing variables as functions of time and position. Perhaps the most noteworthy model specification tools are

* The Channel Builder, which is discussed below (see User-extendable library of biophysical mechanisms).
* The CellBuilder, which can be used to create new models of cells from scratch and modify existing models without writing any code. Biophysical properties may be constant over the length of each section, or governed by user-specified functions of position according to radial distance from a point, path distance from a reference location in the cell, or distance along an axis in the xy plane.
* The Import3D tool, which can convert morphometric data from Eutectic, Neurolucida, and SWC formats into model cells. It fixes many common errors automatically, and helps pinpoint complex problems that require judgment.
* The Linear Circuit Builder, a tool for setting up models that involve gap junctions, ephaptic interactions, dual-electrode voltage clamps, dynamic clamps, or other combinations of neurons and electrical circuit elements. For example, this model of a dynamic clamp was implemented with the Linear Circuit Builder.
* The Network Builder, which can be used to prototype small networks that can be mined later for reusable code in order to create large-scale networks.

There are also powerful tools for model analysis and optimization.

* The ModelView tool automatically discovers model properties and presents them in a browsable textual and graphical form. This can be very helpful for code development and maintenance. It is growing increasingly important as code sharing becomes more common, not only because it helps users understand each others' models, but also because it can emit and import model specifications in XML, thereby facilitating interoperability with other simulators.
* The Impedance tools perform electrotonic analyses of a model cell, including input and transfer impedance, voltage transfer ratios, and the electrotonic transformation, and presents the results in easily understood graphs.
* The Multiple Run Fitter facilitates setting up and managing optimization protocols for automated tuning of model parameters that are constrained by data from one or more experimental protocols.

For some examples of how NEURON can be used, be sure to see Demonstrations and examples.

User-extendable library of biophysical mechanisms
-------------------------------------------------

User-defined mechanisms such as voltage- and ligand-gated ion channels, diffusion, buffering, active transport, etc., can be added by writing model descriptions in NMODL, a high-level programming language that has a simple syntax for expressing kinetic schemes and sets of simultaneous algebraic and/or differential equations. NMODL can also be used to write model descriptions for new classes of artificial spiking cells. These model descriptions are compiled so that membrane voltage and states can be computed efficiently using integration methods that have been optimized for branched structures. A large number of mechanisms written in NMODL have been made available on the WWW by the authors of published models; many of these have been entered into `ModelDB <https://modeldb.yale.edu>`_ which makes it easy for users to find and retrieve model source code according to search criteria such as author, type of model (e.g. cell or network), ionic currents, etc..

As mentioned above, NEURON also has a GUI tool called the ChannelBuilder that makes it easy to specify voltage- and ligand-gated ion channels in terms of ODEs (HH-style, including Borg-Graham formulation) and/or kinetic schemes. ChannelBuilder channels actually execute faster than identical mechanisms specified with NMODL. Their states and total conductance can be simulated as deterministic (continuous in time), or stochastic (countably many channels with independent state transitions, producing random, abrupt conductance changes).

Enhancements for modeling networks
----------------------------------

Although NEURON began in the domain of single-cell models, since the early 1990s it has been applied to network models that contain large numbers of cells and connections. Its suitability for network models has been enhanced by the introduction of features that include

* an event delivery system that manages delivery of spike events to synaptic targets, and has enabled the implementation of artificial spiking cells that are several orders of magnitude faster than biophysical model cells and can be used in network models that may also include biophysical model cells
* a Network Connection object class, which has greatly simplified the construction and management of networks
* generalized synapses which facilitate computationally efficient implementation of synaptic convergence, while allowing stream-specific synaptic plasticity governed by user-specified rules (e.g. use-dependent and/or spike-timing-dependent plasticity)
* global and local variable-order, variable-stepsize integration methods, which can substantially shorten runtimes for some models
* ability to implement network models that are distributed across processors in a parallel computational architecture (Migliore et al, 2006). Properly written code will work on a single processor standalone PC, or on parallel hardware, without modification.

Customizable initialization and simulation flow control
-------------------------------------------------------

Complex models often require custom initialization and/or simulation flow control. Simulation control in NEURON employs a standard run system that is designed for convenient customization.

Large user base
---------------

As of February 2021, we know of `more than 2400 scientific articles and books have reported work that was done with NEURON <https://neuron.yale.edu/neuron/publications/neuron-bibliography>`_. The NEURON mailing list has about 500 subscribers, and `The NEURON Forum <https://www.neuron.yale.edu/phpBB/>`_, which was started in May 2005 and is gradually supplanting the old mailing list, has more than 1600 registered users and over 17,000 posted messages in greater than 4000 discussion threads.

Development, support, and documentation
---------------------------------------

NEURON is actively developed and supported, with new standard releases appearing about twice a year, supplemented by bug fixes as needed. Alpha versions that contain "the very latest features" are made available at shorter intervals.

 

Support is available by email, telephone, and consultation. Users can also post questions and share information with other members of the NEURON community through a mailing list and the NEURON Forum. In addition to being a browsable and searchable venue for discussions on specific questions, the Forum contains a growing list of tips, how-tos, and hacks of interest to users at all levels of expertise.

Extensive documentation is available, including an on-line programmer's reference, FAQ list, tutorials, and preprints of key articles about NEURON. The authoritative book about NEURON is The NEURON Book (Carnevale and Hines, 2006). Four other books have made extensive use of NEURON (Destexhe and Sejnowski, 2001; Johnston and Wu, 1995; Lytton, 2002; Moore and Stuart, 2000), and several of these include source code or have made it available online.

One day courses on NEURON are presented as a satellite to the annual meetings of the Society for Neuroscience, and intensive five day hands-on summer courses are given at the University of California in San Diego and other locations; `click here for announcements of future courses <https://neuron.yale.edu/neuron/courses>`_. Special seminars and tutorials are presented at The NEURON Simulator Meeting, an episodic gathering of neuroscience investigators and educators who are interested in using computational modeling in their work. Instruction and consultation on NEURON are also provided at the european Advanced Course in Computational Neuroscience.

Availability and system requirements
------------------------------------

NEURON is distributed free of charge from `neuron.yale.edu <https://neuron.yale.edu>`_. It runs on all popular hardware platforms under MSWin (98 or later), UNIX, Linux, and OS X, and on parallel hardware including Beowulf clusters, the IBM Blue Gene, and the Cray XT3.

References
----------
Carnevale, N.T. and Hines, M.L. The NEURON Book. Cambridge, UK: Cambridge University Press, 2006.

Destexhe, A. and Sejnowski, T.J. Thalamocortical Assemblies. New York: Oxford University Press, 2001.

Hindmarsh, A.C. and Serban, R. User documentation for CVODES, an ODE solver with sensitivity analysis capabilities. Lawrence Livermore National Laboratory, 2002.

Hindmarsh, A.C. and Taylor, A.G. User documentation for IDA, a differential-algebraic solver for sequential and parallel computers. Lawrence Livermore National Laboratory, 1999.

Hines, M.L. and Carnevale, N.T. NEURON: a tool for neuroscientists. The Neuroscientist 7:123-135, 2001.

Johnston, D. and Wu, S.M.-S. Foundations of Cellular Neurophysiology. Cambridge,MA: MIT Press, 1995.

Kernighan, B.W. and Pike, R. Appendix 2: Hoc manual. In: The Unix Programming Environment. Englewood Cliffs, NJ: Prentice-Hall, 1984.

Lytton, W.W. From Computer to Brain. New York: Springer-Verlag, 2002.

Migliore, M., Cannia, C., Lytton, W.W., Markram, H. and Hines, M.L. Parallel network simulations with NEURON. Journal of Computational Neuroscience 21:110-119, 2006.

Moore, J.W. and Stuart, A.E. Neurons in Action: Computer Simulations with NeuroLab. Sunderland, MA: Sinauer Associates, 2000.

SUNDIALS is available from https://computation.llnl.gov/projects/sundials 

