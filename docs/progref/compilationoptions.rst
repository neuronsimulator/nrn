Rarely Used Compilation Options
===============================

The following definitions are found in `src/nrnoc/options.h <https://github.com/neuronsimulator/nrn/blob/master/src/nrnoc/options.h>`_ and add extra 
functionality which not everyone may need. The extras come at the cost 
of larger memory requirements for node and section structures.

.. code-block::
    none

    #define VECTORIZE	1	/* hope this speeds up simulations on a Cray */ 
    				/* this is no longer optional */ 
     
    #define EXTRACELLULAR	1	/* extracellular membrane mechanism */ 
     
    #define DIAMLIST	1	/* section contains diameter info */ 
    				/* shape plots make use of this */ 
     
    #define EXTRAEQN	0	/* ionic concentrations calculated via 
    				 * jacobian along with v (not implemented) */ 
    #if DIAMLIST 
    #define NTS_SPINE	1	/* A negative diameter in pt3dadd() tags that 
    				 * diamlist location as having a spine. 
    				 * diam3d() still returns the positive diameter 
    				 * spined3d() returns 1 or 0 signifying presence 
    				 * of spine. setSpineArea() tells how much 
    				 * area/spine to add to the segment. */ 
    #endif 
