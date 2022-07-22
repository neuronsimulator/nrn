.. _quality_issues_with_morphometric_data:

Quality Issues with Morphometric Data
=====================================

Like any other experimental measurements, morphometric data are subject to their own sources of error. Many pitfalls await the uninformed. The investigator who lacks practical experience in microscopic neuroanatomy in general, and in the acquisition of morphometric data in particular, would do well to find a colleague who has such experience, and ask that person how it is done, what are criteria for "quality," and what caveats apply--maybe even offer to do a reconstruction for the sake of gaining experience.

`MBF Bioscience's website <https://www.mbfbioscience.com/>`_ can be a useful source of information. Also, there is a scientific literature about the quality of morphometric data from the perspective of computational modeling. These two articles could serve as a starting point--they discuss various artifacts that frequenly affect morphometric data and can have serious adverse effects on simulation results:

Kaspirzhny AV, Gogan P, Horcholle-Bossavit G, Tyc-Dumont S. 2002. Neuronal morphology data bases: morphological noise and assesment of data quality. Network: Computation in Neural Systems 13:357-380. `doi:10.1088/0954-898X_13_3_307 <https://www.tandfonline.com/doi/abs/10.1088/0954-898X_13_3_307>`_

Scorcioni, R., Lazarewicz, M.T., and Ascoli, G.A. Quantitative morphometry of hippocampal pyramidal cells: differences between anatomical classes and reconstructing laboratories. Journal of Comparative Neurology 473:177-193, 2004. `doi:10.1002/cne.20067 <https://onlinelibrary.wiley.com/doi/10.1002/cne.20067>`_

These also appear on the :ref:`working with morphometric data <_using_morphometric_data>` page.

A more recent source of information is **Chapter 6. Methods** in Corinne Teeter's dissertation
**Characterizing the Spatial Density Functions of Neural Arbors**
which is available at `this link <https://escholarship.org/uc/item/2jq2z2xq>`_.

Important things to keep in mind
--------------------------------

There is no guarantee that any morphometric data file will be suitable for any use other than that which motivated the authors from whose lab it came.

Much, if not most, morphometric data were acquired by students who worked long hours while being paid minimum wage, and whose careers didn't depend on the quality of that data.

The files in the `NeuroMorpho.org <https://neuromorpho.org>`_ archive have been cleaned up so that most if not all of them should be importable by NEURON, but the fact that a given file imports without complaint does not guarantee the quality of the data or its suitability for computational modeling. Indeed, morphometric data are subject to many limitations and artifacts that need to be considered.

At the very least, before using any such data, the modeler should

1.
    Read the paper written by the anatomist(s) who collected the data. What staining and imaging techniques were used (some are more likely than others to produce incomplete "fills", or to lead to under or overestimation of the diameter of fine branches)? What controls did the authors use to reject cells that were incompletely filled? Were the authors concerned about accurate diameter measurements, or did they only want to capture the branched architecture and the lengths of individual neurites? What was the optical resolution of the imaging method?

2.
    Examine the data. This can be done by parsing the original file, or by exploring the pt3d data after it has been imported into NEURON. What is the finest diameter measurement, and what is the diameter quantization (smallest difference between different diameter measurements)? Do the diameter measurements reveal "favorite numbers" (bias for "nice" diameter values)? Watch out for possible bottlenecks, i.e. diameter measurements that are unreasonably small. Remember the practical limits of ordinary light microscopy--any diameter smaller than about 0.3 µm is unlikely to be "correct."

3.
    Pay attention to the comments and error messages that are generated when a morphometric data file is imported by the Import3d tool. Some surprising errors may turn up; many of these can be easily resolved, but others may make you decide to avoid using that particular morphology.

4.
    Examine the shape of the reconstructed cell, e.g. in a shape plot. Does it look like a representative member of its cell class? Did the authors use "bogus neurites" as fiducial marks, e.g. to indicate the location of a cell body layer or the boundaries of different cortical layers? In the shape plot, rotate the cell 90 degrees around the y (or x) axis to see if there are abrupt jumps in the z direction. Such jumps result from drift and backlash in the microscope's z axis position control, and can make a morphology file useless for modeling purposes.
