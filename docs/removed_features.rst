Removed Features
================

This page aims at providing a short knowledge base designed to track dropped and potentially useful features for future development.
To that end, the following table's columns constitute:

* the name of the feature; also stands as a keyword.
* a short description; the main purpose is to have the feature as greppable as possible.
* removal PR number(s) related to their removal from the codebase.
* Short SHA pointing to the last master commit holding the feature for easy checkout.

.. list-table:: Removed Features
   :widths: 20 60 13 7
   :header-rows: 1
   :class: fixed-table

   * - Name / Keyword
     - Short description
     - Removal PR(s)
     - SHA
   * - ni_pci_6229
     - This related to a circa 2007 project implementing hard real time dynamic clamp under RTAI linux.
       The development was done in conjunction with National Instruments' NI PCI-6229 DAQ card.
     - `#1399 <https://github.com/neuronsimulator/nrn/pull/1399>`_
     - c46dbc7
   * - bluegene
     - Code related to BlueGene L, P and Q; support for BlueGene Checkpoint API (keywords: BGLCheckpoint, BGLCheckpointInit).
     - `#1286 <https://github.com/neuronsimulator/nrn/pull/1286>`_
     - 74d3db9
   * - SEJNOWSKI
     - C Preprocessor flag.
     - `#1415 <https://github.com/neuronsimulator/nrn/pull/1415>`_
     - e9ef741
   * - NRN_REALTIME
     - NEURON support for RTAI - Real Time Application Interface for Linux. Also relates to `ni_pci_6229`
     - `#1401 <https://github.com/neuronsimulator/nrn/pull/1401>`_
     - d5f6139
   * - CYGWIN
     - Windows versions now use MINGW (more native to WINDOWS).
     - `#1802 <https://github.com/neuronsimulator/nrn/pull/1802>`_
     - 2f90f37
   * - Carbon
     - This legacy macOS toolkit was deprecated in 2012 and removed from macOS 10.15
     - `#1869 <https://github.com/neuronsimulator/nrn/pull/1869>`_
     - 8fecd77
