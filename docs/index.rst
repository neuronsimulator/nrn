.. NEURON documentation master file, created by
   sphinx-quickstart on Fri Nov 15 09:46:09 2019.

The NEURON Simulator
====================

NEURON is a simulator for neurons and networks of neurons that runs efficiently on your local machine, in the cloud, or on an HPC.
Build and simulate models using Python, HOC, and/or NEURON's graphical interface. From this page you can watch :ref:`recorded NEURON classes <training_videos>`, 
read the :ref:`Python <python_prog_ref>` or :ref:`HOC <hoc_prog_ref>` programmer's references,
`browse the NEURON forum <https://www.neuron.yale.edu/phpBB/>`_,
explore the `source code for over 750 NEURON models on ModelDB <https://senselab.med.yale.edu/ModelDB/ModelList?id=1882&allsimu=true>`_, and more (use the links on the side or search).

.. toctree::
   :maxdepth: 1
   :caption: Building:
   :hidden:

   install/install
   cmake_doc/index
   install/developer

.. toctree::
   :maxdepth: 2
   :caption: User documentation:
   :hidden:

   videos/index
   guide/index
   courses/exercises2018
   The NEURON forum <https://neuron.yale.edu/phpBB>
   publications
   publications-using-neuron
   


.. toctree::
   :maxdepth: 2
   :caption: NEURON scripting:
   :hidden:

   python/index
   hoc/index
   otherscripting.rst
   tutorials/index
   rxd-tutorials/index
   coreneuron/index

.. toctree::
   :maxdepth: 2
   :caption: Developer documentation:
   :hidden:

   scm/index
   dev/index
   doxygen

.. toctree::
   :maxdepth: 1
   :caption: Removed Features
   :hidden:

   removed_features.rst

.. toctree::
   :maxdepth: 1
   :caption: Changelog
   :hidden:

   changelog.md


.. image:: neuron-overview.jpg
   :width: 75%
   :align: center
   :alt: NEURON image with ShapePlot, code, and cortical column


Installation
------------

.. tab-set::

   .. tab-item:: macOS

      The recommended installation is to:

      .. code::

         pip3 install neuron
      
      Alternatively, you can use the `PKG installer <https://github.com/neuronsimulator/nrn/releases/download/8.2.1/nrn-8.2.1-macosx-10.9-universal2-py-38-39-310.pkg>`_.

      For troubleshooting, see the `detailed installation instructions <install/install_instructions.html>`_.


   .. tab-item:: Linux

      The recommended installation is to:

      .. code::

         pip3 install neuron
      
      For troubleshooting, see the `detailed installation instructions <install/install_instructions.html>`_.


   .. tab-item:: Windows

      `Download the Windows Installer <https://github.com/neuronsimulator/nrn/releases/download/8.2.1/nrn-8.2.1.w64-mingw-py-37-38-39-310-setup.exe>`_.

      You can also install the Linux wheel via the Windows Subsystem for Linux (WSL). See `instructions <install/install_instructions.html#windows-subsystem-for-linux-wsl-python-wheel>`_.

      For troubleshooting, see the `detailed installation instructions <install/install_instructions.html>`_.

   
   .. tab-item:: Cloud

      On `Google Colab <https://colab.research.google.com>`_ and many other cloud Jupyter providers, you can install
      NEURON via

      .. code::

         !pip install neuron
      
      NEURON is already installed on `The Neuroscience Gateway <https://www.nsgportal.org>`_
      and on `EBRAINS <https://ebrains.eu>`_.
   
   .. tab-item:: Source code

      View and suggest changes to the source code at:
      `github.com/neuronsimulator/nrn <https://github.com/neuronsimulator/nrn>`_

      For instructions on how to build from source,
      `go here <install/install_instructions.html#installing-source-distributions>`_.


.. raw:: html


   <script>

      // script for OS detection from http://stackoverflow.com/questions/7044944/jquery-javascript-to-detect-os-without-a-plugin
      osName = 'Unknown';

      function nav(x, y, z) {
         z = z || y;
         if (navigator[x] && navigator[x].indexOf(y) !== -1) {
            osName = z;
         }
      }

      /*   navigator     value     download  */
      nav("appVersion", "Mac", "MacOS");
      nav("appVersion", "Linux");
      nav("userAgent", "Linux");
      nav("platform", "Linux");
      nav("appVersion", "Win", "Windows");
      nav("userAgent", "Windows");
      nav("platform", "Win", "Windows");
      nav("oscpu", "Windows");
      
      if (osName == "MacOS") {
         $("#installation input")[0].checked = true;
      } else if (osName == "Linux") {
         $("#installation input")[1].checked = true;
      } else if (osName == "Windows") {
         $("#installation input")[2].checked = true;
      }

   </script>

|

See also the NEURON documentation `index <genindex.html>`_ and the `NEURON forum <https://www.neuron.yale.edu/phpbb/>`_.
