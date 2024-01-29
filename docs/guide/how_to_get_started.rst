.. _how_to_get_started:

How to get started with NEURON
==============================

Installation
------------

If you haven't already, install NEURON. Detailed instructions for special cases are available `here <../install/install_instructions.html>`_, but in brief, you'll probably want to install Python (there are many options; `Anaconda <https://www.anaconda.com/>`_ is one such option). On Linux and macOS, you can then install NEURON by typing ``pip install neuron``. On Windows, use the latest NEURON installer from `our GitHub site <https://github.com/neuronsimulator/nrn/releases>`_. For parallel simulation, you will also want to install a version of MPI.

Forum
-----

You'll likely have questions. Join `The NEURON Forum <https://www.neuron.yale.edu/phpBB/index.php>`_ to ask and answer questions.

Basic NEURON Usage
------------------

* Watch :ref:`recorded videos <training_videos>` from various recent NEURON and NEURON-related courses.
* Read "`The NEURON Simulation Environment <https://doi.org/10.1162/neco.1997.9.6.1179>`_." 
* Use the `Programmer's Reference <../python/index.html>`_ early and often. 
* Work through the tutorials on the Documentation and Courses pages.
* Use the GUI tools as much as possible. You'll get more done, faster, and you won't have to write any code. Some of the GUI tools are described in the tutorials; others are demoed in the :ref:`course videos <training_videos>`. Save the GUI tools to session files; these files contain HOC and can be modified, adapted, and reused.
* Examine `ModelDB <https://modeldb.yale.edu>`_ and the list of :ref:`publications about NEURON <publications_about_neuron>` to find models of interest. Many authors have deposited their model code in ModelDB, posted it somewhere else on the WWW, or will provide code upon request.

.. _extending_neuron_with_nmodl:

Using NMODL to add new mechanisms to NEURON
-------------------------------------------

* First, consider using the ChannelBuilder instead. This is an extremely powerful GUI tool for specifying voltage- and ligand-gated ionic conductances. It's much easier to use than NMODL. Mechanisms specified with the ChannelBuilder actually execute faster than if they were specified with NMODL. Also, you can use it to make stochastic channel models.
* If you absolutely must use NMODL (e.g. for ion accumulation mechanisms or to add new kinds of artificial spiking cells), read chapters 9 and 10 of The NEURON Book, or at least the articles "Expanding NEURON's Repertoire of Mechanisms with NMODL" and "Discrete event simulation in the NEURON environment".
* NEURON comes with a bunch of mod files that can serve as starting points for "programming by example." Under MSWin the default mechanisms (hh, pas, expsyn etc.) are in `github.com/neuronsimulator/nrn/tree/master/src/nrnoc <https://github.com/neuronsimulator/nrn/tree/master/src/nrnoc>`_. A large collection of mod files is at `github.com/neuronsimulator/nrn/tree/master/share/examples/nrniv/nmodl <https://github.com/neuronsimulator/nrn/tree/master/share/examples/nrniv/nmodl>`_.
* You may also find useful examples in `ModelDB <https://modeldb.yale.edu>`_.

.. include:: ../rst_substitutions.txt

.. note::
  Starting in |neuron_with_cpp_mechanisms|, NMODL is translated into C++ code
  instead of C.
  This change is not fully backwards compatible, and you may find that older
  NMODL code needs to be updated to work with |neuron_with_cpp_mechanisms|.
  This is principally an issue for MOD files that include ``VERBATIM`` blocks.
  For more information, see :ref:`porting-mechanisms-to-cpp`.

.. note::
  Starting in |neuron_with_soa_data|, the model data structures used by NEURON
  have been completely overhauled.
  This change is not fully backwards compatible, and you may find that older
  NMODL code needs to be updated to work with |neuron_with_soa_data|.
  This is principally an issue for MOD files that include ``VERBATIM`` blocks.
  For more information, see :ref:`porting-mechanisms-to-new-data-structures`.
