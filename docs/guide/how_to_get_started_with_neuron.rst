.. _how_to_get_started_with_neuron:

How to get started with NEURON
=============

To learn the basics about using NEURON:
-------------

Don't forget to join `The NEURON Forum <https://www.neuron.yale.edu/phpBB/index.php>`_.

Then

1.
    Read `"The NEURON Simulation Environment." <https://pubmed.ncbi.nlm.nih.gov/9248061/#:~:text=The%20NEURON%20simulation%20program%20provides,and%20membrane%20currents%20are%20complex.>`_

2.
    Use the Programmer's Reference early and often. If you are running NEURON under MSWindows and accepted the default installation, the Programmer's Reference is already in the NEURON program group--put a shortcut to it on your desktop. If you are using MacOS or UNIX, you will have to download it separately from the `Documentation page <https://nrn.readthedocs.io/en/latest/python/index.html>`_. 

3.
    Work through the tutorials on the `Documentation <https://nrn.readthedocs.io/en/latest/python/index.html>`_ and `Courses <https://nrn.readthedocs.io/en/latest/courses/exercises2018.html>`_ pages.

4.
    Use the GUI tools as much as possible. You'll get more done, faster, and you won't have to write any code. Some of the GUI tools are described in the tutorials, and others are demonstrated in more detail in John Moore's User's Manual (see the `Documentation <https://nrn.readthedocs.io/en/latest/python/index.html>`_ page).

5.
    The GUI tools can also help you learn how to use hoc, NEURON's programming language. The CellBuilder and Network Builder can export hoc code that you can examine and reuse to do new things. You can also save the smaller GUI tools to session files, which contain reusable hoc statements.

6.
    Examine `ModelDB <https://senselab.med.yale.edu/modeldb/>`_ and the list of `publications about NEURON <https://nrn.readthedocs.io/en/latest/publications.html>`_ to find models of interest. Many authors have deposited their model code in ModelDB, posted it somewhere else on the WWW, or will provide code upon request.

To learn how to use NMODL to add new mechanisms to NEURON:
------------------------

1.
    First, consider using the ChannelBuilder instead (see the `Documentation page <https://nrn.readthedocs.io/en/latest/python/index.html>`_ for a tutorial). This is an extremely powerful GUI tool for specifying voltage- and ligand-gated ionic conductances. It's much easier to use than NMODL. Mechanisms specified with the ChannelBuilder actually execute *faster* than if they were specified with NMODL. Also, you can use it to make stochastic channel models.

2.
    If you absolutely must use NMODL (e.g. for ion accumulation mechanisms or to add new kinds of artificial spiking cells), read chapters 9 and 10 of The NEURON Book, or at least the articles "Expanding NEURON's Repertoire of Mechanisms with NMODL" and "Discrete event simulation in the NEURON environment" (downloadable from the `publications about NEURON <https://nrn.readthedocs.io/en/latest/publications.html>`_ page).

3.
    NEURON comes with a bunch of mod files that can serve as starting points for "programming by example." Under MSWin the default mechanisms (hh, pas, expsyn etc.) are in ``c:\nrn\src\nrnoc`` (on my Linux box this is ``/usr/local/src/nrn-x.x/src/nrnoc``). A large collection of mod files is in ``c:\nrn\examples\nrniv\nmodl`` (Linux ``/usr/local/src/nrn-x.x/share/examples/nrniv/nmodl``).

4.
    You may also find useful examples in `ModelDB <https://senselab.med.yale.edu/modeldb/>`_.

For courses about NEURON, see the :ref:`Course Exercises <exercises2018>` page and the :ref:`Training Videos <training_videos>` page.




