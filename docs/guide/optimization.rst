Using NEURON's Optimization Tools
=================================

This collection of tutorials shows how to use NEURON's optimization tools. Before working through these tutorials, most readers should probably examine the on-line "Introduction to Optimization" http://neos-guide.org/content/optimization-introduction

    Those who have a special interest in the topic of optimization may wish to browse the NEOS Guide more extensively http://neos-guide.org/.

The general idea is that we want to adjust the parameters of a function or computational model so that its performance satisfies a set of criteria. These tutorials show how to accomplish this task by using the Multiple Run Fitter to

* specify the function or model that we want to optimize
* specify the criteria that we want it to satisfy
* specify the parameters that will be adjusted and any constraints on those parameters
* perform the optimization

Why is it called "Multiple Run Fitter," and what does it really do?
-------------------------------------------------------------------

The simplest optimization task in computational modeling is to adjust model parameters so that the results of a simulated experiment (a "simulation run") fit measurements that were obtained under a particular experimental protocol.

However, experimental techniques have advanced to the point where it is now possible to carry out several protocols on the same cell, often involving simultaneous recording from multiple sites. The availability of this kind of data raises the possibility of tighter constraint of model parameters. NEURON's Multiple Run Fitter can be used to perform optimizations that involve fitting many simulation runs, each generated with a different protocol and involving several recordings, to the corresponding data. The Multiple Run Fitter is much more powerful than the old Run Fitter and Function Fitter, and it is recommended for all optimization problems.

A Multiple Run Fitter uses the results from one or more "Generators" to compute the total error that will be minimized during optimization. A Generator is similar to a RunFitter, in that it specifies a simulation protocol (the computational modeling equivalent of an experimental protocol) and computes an error by calculating the difference between simulation results and data. Every Generator can have its own simulation protocol, data, and criteria for goodness of fit. Each Generator can be turned on or off; a Generator that is off is not used (does not result in a run and does not contribute to the total error value).

At present, the optimization algorithm used by the Multiple Run Fitter is the PRAXIS (principal axis) method described by Brent (1976).

    Brent, R. A new algorithm for minimizing a function of several variables without calculating derivatives. In: Algorithms for Minimization without Derivatives. Englewood Cliffs, NJ : Prentice Hall, 1976, p. 200-248.


.. toctree::
    :maxdepth: 2
    :caption: The tutorials:

    optimization1
    optimization2


Other optimization tools that work with NEURON
----------------------------------------------

You may also be interested in `BluePyOpt <https://github.com/BlueBrain/BluePyOpt>`_.

    Van Geit W, Gevaert M, Chindemi G, Rössert C, Courcol J, Muller EB, Schürmann F, Segev I and Markram H (2016). BluePyOpt: Leveraging open source software and cloud infrastructure to optimise model parameters in neuroscience. *Front. Neuroinform.* 10:17. `doi:10.3389/fninf.2016.00017 <https://doi.org/10.3389/fninf.2016.00017>`_.

After installing it via

    .. code::
        none

        pip install bluepyopt

See their `quick start guide <https://github.com/BlueBrain/BluePyOpt#quick-start>`_ for more examples of usage.