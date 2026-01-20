.. _incompatible-nmodl-intermediate-files:

I installed a new version of NEURON, and now I see error messages like this: 'mecanisms fooba needs to be re-translated. its version 5.2 "c" code is incompatible with this neuron version'.
-----------------------------------------------------------------------------------------------------------------------------

Compiling NMODL files produces several "intermediate files" whose names end in .o and .c . This error message means that you have some old intermediate files that were produced under the earlier version of NEURON. So just delete all the .o and .c files, then run nrnivmodl (or mknrndll), and the problem should disappear.

