mcell_ran4
----------

         


.. function:: mcell_ran4

    Syntax:
        ``x = h.mcell_ran4(highindex_ptr)``


    Description:
        A 64bit cryptographic quality random number generator. Uniformly distributed 
        in the range 0-1.  The highindex integer (0 <= highindex <= 2^32-1) 
        is incremented by one on each call. Multiple independent 
        streams are possible, each associated with a separate  highindex 
        with different starting values but the difference between highindex starting 
        values should be greater than the length of each stream. 
        The lowindex 32 bits are 0 by default and 
        can be set using :func:`mcell_ran4_init` . Streams can be replayed at any index 
        (as long as the lowindex is the same). 
         
        See :meth:`Random.Random123` for an even more useful alternative. 
         
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
            python

            from neuron import h, gui

            class McellRan4Test:
                def __init__(self):
                    self.vec = h.Vector(1000)
                    self.g1 = h.Graph()
                    self.g2 = h.Graph()
                    self.g1.size(0, 1000, 0, 1) 
                    self.g2.size(0, 1, 0, 150) 
                    self.highindex = 1
                    self.lowindex = h.mcell_ran4_init() 
                    self.mcell_func()

                def mcell_func(self):
                    self.g1.erase() 
                    self.g2.erase()
                    highindex_ptr = h.ref(self.highindex)
                    for i in range(len(self.vec)):            
                        self.vec[i] = h.mcell_ran4(highindex_ptr) 
                    # resync the highindex (needed for the GUI)
                    self.highindex = highindex_ptr[0]
                    self.hist = self.vec.histogram(0, 1, 0.1) 
                    self.vec.line(self.g1) 
                    self.hist.line(self.g2, .1) 
                    self.g1.flush()
                    self.g2.flush() 

                def mcell_func2(self):
                    h.mcell_ran4_init(self.lowindex) 
                    self.mcell_func()
 
                window = McellRan4Test()
                h.xpanel('mcell_ran4 test') 
                h.xbutton('Sample', window.mcell_func) 
                h.xvalue('highindex', (window, 'highindex'), 1, window.mcell_func) 
                h.xvalue('lowindex', (window, 'lowindex'), 1, window.mcell_func2) 
                h.xpanel() 

    .. |logo1| image:: ../../images/mcran4-xvalue.png
        :width: 200px
        :height: 150px
        :align: top
    .. |logo2| image:: ../../images/mcran4-graph1.png
        :align: middle
        :width: 200px
        :height: 150px
    .. |logo3| image:: ../../images/mcran4-graph2.png
        :align: bottom
        :width: 200px
        :height: 150px
    +-----+---------+---------+---------+
    |     | |logo1| | |logo2| | |logo3| |
    +-----+---------+---------+---------+


    .. seealso::
        :class:`Random`, :meth:`Random.MCellRan4`, :func:`use_mcell_ran4`, :func:`mcell_ran4_init`,
        :meth:`Random.Random123`

         

----



.. function:: use_mcell_ran4


    Syntax:
        ``previous = h.use_mcell_ran4(next) # next must be 0 or 1``

        ``boolean = h.use_mcell_ran4()``


    Description:
        h.use_mcell_ran4(1) causes scop_random in model descriptions to use 
        the :func:`mcell_ran4` cryptographic quality random generator. Otherwise, the 
        low quality (but faster) linear congruential generator is used. 
         
        At present (version 5.2) the default is 0. 
         
        Note that this affects the random numbers computed within 
        model descriptions that use the functions: 
        scop_random, exprand, normrand, and poisrand. Also note that when a model 
        description uses set_seed(seed) and use_mcell_ran4 is true then the 
        seed refers to the 32 bit highindex as in :func:`mcell_ran4` . 

         

----



.. function:: mcell_ran4_init


    Syntax:
        ``previous_lowindex = h.mcell_ran4_init(lowindex)``

        ``lowindex= h.mcell_ran4_init()``


    Description:
        Sets the 32 bit lowindex of the :func:`mcell_ran4` generator. The default lowindex 
        is 0. This affects random number streams (when use_mcell_ran4() returns 1) 
        in model descriptions using scop_rand, etc. It also affects Random 
        class streams that are using the :meth:`Random.MCellRan4` generator. 
         
        :meth:`Random.Random123_globalindex` plays a similar role as this function for 
        the :meth:`Random.Random123` generator. 
         

         
         

