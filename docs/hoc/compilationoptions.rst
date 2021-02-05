Rarely Used Compilation Options
===============================

The following definitions are found in nrnoc/SRC/options.h and add extra 
functionality which not everyone may need. The extras come at the cost 
of larger memory requirements for node and section structures. METHOD3 is too large 
and obscure to benefit most users. 

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
     
    #define METHOD3		1	/* third order spatially correct method */ 
    				/* testing only, not completely implemented */ 
    				/* not working at this time */ 
     
    #if METHOD3 
     spatial_method(i) 
    	no arg, returns current method 
    	i=0 The standard NEURON method with zero area nodes at the ends 
    		of sections. 
    	i=1 conventional method with 1/2 area end nodes 
    	i=2 modified second order method 
    	i=3 third order correct spatial method 
    	Note: i=1-3 don't work under all circumstances. They have been 
    	insufficiently tested and the correctness must be established for 
    	each simulation. 
    #endif 
     
    #if NEMO 
    neuron2nemo("filename") Beginning of translator between John Millers 
    		nemosys program	and NEURON. Probably out of date. 
    nemo2neuron("filename") 
    #endif 



