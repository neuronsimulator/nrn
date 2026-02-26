.. _seclamp-vclamp-difference:

What is the difference between SEClamp and VClamp, and which should I use?
--------------------------------------------------------------------------

:class:`SEClamp` is just an ideal voltage source in series with a resistance (Single Electrode Clamp), while :class:`VClamp` is a model of a two electrode voltage clamp with this equivalent circuit:

.. code::

                   tau2
                   gain
                  +-|\____rstim____>to cell
  -amp --'\/`-------|/
                  |
                  |----||---
                  |___    __|-----/|___from cell
                      `'`'        \|
                      tau1 


If the purpose of your model is to study the properties of a cell, use :class:`SEClamp`. If the purpose is to study how instrumentation artefacts affect voltage clamp data, use :class:`VClamp`. 

