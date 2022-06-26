mcell_ran4
----------

         


.. hoc:function:: mcell_ran4


    Syntax:
        ``x = mcell_ran4(&highindex)``


    Description:
        A 64bit cryptographic quality random number generator. Uniformly distributed 
        in the range 0-1.  The highindex integer (0 <= highindex <= 2^32-1) 
        is incremented by one on each call. Multiple independent 
        streams are possible, each associated with a separate  highindex 
        with different starting values but the difference between highindex starting 
        values should be greater than the length of each stream. 
        The lowindex 32 bits are 0 by default and 
        can be set using :hoc:func:`mcell_ran4_init` . Streams can be replayed at any index
        (as long as the lowindex is the same). 
         
        See :hoc:meth:`Random.Random123` for an even more useful alternative.
         
        This generator was obtained from Tom Bartol ``<bartol@salk.edu>``
        who uses them in his mcell program. (hence the mcell prefix) 
        He comments: 
        For MCell, Ran4 has the distinct advantage of generating 
        streams of random bits not just random numbers.  This means that you can 
        break the 32 bits of a single returned random number into several smaller 
        chunks without fear of correlations between the chunks.  Ran4 is not the 
        fastest generator in the universe but it's pretty fast (16 million 
        floating point random numbers per second on my 1GHz Intel PIII and 20 
        million integer random numbers per second) and of near cryptographic 
        quality. I've modified it so that a given seed will generate the same 
        sequence on Intel, Alpha, RS6000, PowerPC, and MIPS architectures (and 
        probably anything else out there).  It's also been modified to generate 
        arbitrary length vectors of random numbers.  This makes generating numbers 
        more efficient because you can generate many numbers per function call. 
        MCell generates them in chunks of 10000 at a time. 

    Example:

        .. code-block::
            none

            objref vec, g1, g2, hist 
            vec = new Vector(1000) 
            g1 = new Graph() 
            g2 = new Graph() 
            g1.size(0, 1000, 0, 1) 
            g2.size(0, 1, 0, 150) 
             
            highindex = 1 
            lowindex = mcell_ran4_init() 
             
            proc doit() {local i 
                    g1.erase() g2.erase() 
            	for i=0, vec.size-1 { 
            		vec.x[i] = mcell_ran4(&highindex) 
            	} 
                    hist = vec.histogram(0, 1, 0.1) 
                    vec.line(g1) 
                    hist.line(g2, .1) 
                    g1.flush g2.flush 
            } 
             
            variable_domain(&highindex, 0, 2^32-1) 
            xpanel("mcell_ran4 test") 
            xbutton("Sample", "doit()") 
            xpvalue("highindex", &highindex, 1, "doit()") 
            xpvalue("lowindex", &lowindex, 1, "mcell_ran4_init(lowindex) doit()") 
            xpanel() 
             
            doit() 


    .. seealso::
        :hoc:class:`Random`, :hoc:meth:`Random.MCellRan4`, :hoc:func:`use_mcell_ran4`, :hoc:func:`mcell_ran4_init`,
        :hoc:meth:`Random.Random123`

         

----



.. hoc:function:: use_mcell_ran4


    Syntax:
        ``previous = use_mcell_ran4(next) // next must be 0 or 1``

        ``boolean = use_mcell_ran4()``


    Description:
        use_mcell_ran4(1) causes scop_random in model descriptions to use 
        the :hoc:func:`mcell_ran4` cryptographic quality random generator. Otherwise, the
        low quality (but faster) linear congruential generator is used. 
         
        At present (version 5.2) the default is 0. 
         
        Note that this affects the random numbers computed within 
        model descriptions that use the functions: 
        scop_random, exprand, normrand, and poisrand. Also note that when a model 
        description uses set_seed(seed) and use_mcell_ran4 is true then the 
        seed refers to the 32 bit highindex as in :hoc:func:`mcell_ran4` .

         

----



.. hoc:function:: mcell_ran4_init


    Syntax:
        ``previous_lowindex = mcell_ran4_init(lowindex)``

        ``lowindex= mcell_ran4_init()``


    Description:
        Sets the 32 bit lowindex of the :hoc:func:`mcell_ran4` generator. The default lowindex
        is 0. This affects random number streams (when use_mcell_ran4() returns 1) 
        in model descriptions using scop_rand, etc. It also affects Random 
        class streams that are using the :hoc:meth:`Random.MCellRan4` generator.
         
        :hoc:meth:`Random.Random123_globalindex` plays a similar role as this function for
        the :hoc:meth:`Random.Random123` generator.
         

         
         

