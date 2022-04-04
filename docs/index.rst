.. NEURON documentation master file, created by
   sphinx-quickstart on Fri Nov 15 09:46:09 2019.

The NEURON Simulator
====================

NEURON is a simulator for neurons and networks of neurons that runs efficiently on your local machine, in the cloud, or on an HPC.
Build and simulate models using Python, HOC, and/or NEURON's graphical interface. From this page you can watch :ref:`recorded NEURON classes <training_videos>`, 
read the :ref:`Python <python_prog_ref>` or :ref:`HOC <hoc_prog_ref>` programmer's references,
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
   publications
   


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


.. image:: neuron-overview.png
   :width: 75%
   :align: center
   :alt: NEURON image with ShapePlot, code, and cortical column


Installation
------------

.. raw:: html

   <div id="downloadMacOS" style="display:none">
      <p>
         On macOS, install via:
         <code>pip3 install neuron</code>
      </p>
      <p>
         Alternatively, 
         <a href="https://github.com/neuronsimulator/nrn/releases/download/8.1.0/nrn-8.1.0-macosx-10.9-universal2-py-38-39-310.pkg">download macOS installer</a>
      </p>
   </div>
   <div id="downloadWindows" style="display:none">
      <a class="button" href="https://github.com/neuronsimulator/nrn/releases/download/8.1.0/nrn-8.1.0.w64-mingw-py-36-37-38-39-310-setup.exe">Download Windows installer (64 bit)</a>
   </div>
   <div id="downloadLinux" style="display:none">
      On Linux, install via:
      <code>pip3 install neuron</code>
   </div>
   <p>
      <a href="https://github.com/neuronsimulator/nrn/releases/tag/8.1.0">All standard versions</a><br>
      <a href="http://github.com/neuronsimulator/nrn">Source on github</a><br>

   For troubleshooting, see the <a href="install/install_instructions.html">detailed installation instructions</a>.</p>

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

   console.log("download" + osName);
   document.getElementById("download" + osName).style.display = "block";


   </script>

----

See also the NEURON documentation `index <genindex.html>`_.
