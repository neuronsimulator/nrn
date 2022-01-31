
.. _hoc_random:

Pseudo-Random Number Generation
-------------------------------

Pseudo-random numbers from a variety of distributions may be generated with the :hoc:class:`Random` class. Multiple random number generators are provided; low level access to the mcell_ran4 generator is described in:

.. toctree:: :maxdepth: 1

    mcran4.rst


Random Class
============

.. hoc:class:: Random

    Syntax:
        ``Random()``

        ``Random(seed)``

        ``Random(seed, size)``


    Description:
        The Random class provides commonly used random distributions which are 
        useful for stochastic 
        simulations. The default distribution is normal with mean = 0 and standard 
        deviation = 1. 
         
        This class is an interface to the RNG class 
        from the gnu c++ class library. As of version 5.2, a cryptographic quality 
        RNG class wrapper for :hoc:func:`mcell_ran4` was added and is available
        with the :hoc:meth:`Random.MCellRan4` method. The current default random generator
        is :hoc:meth:`Random.ACG`.
         
        As of version 7.3, a more versatile cryptographic quality generator, 
        Random123, is available with the :hoc:meth:`Random.Random123` method. This generator
        uses a 34bit counter, up to 3 32 bit identifiers, and a 32 bit global index and 
        is most suitable for managing separate independent, reproducible, restartable 
        streams that are unique to individual cell and synapses in large parallel 
        network models. 
        See: http://www.thesalmons.org/john/random123/papers/random123sc11.pdf 
         
        Note that multiple instances of the Random class will produce different 
        streams of random numbers only if their seeds are different. 
         
        One can switch distributions at any time but if the distribution is 
        stationary then it is more efficient to use :hoc:meth:`Random.repick` to avoid
        constructor/destructor overhead. 
         

    Example:

        .. code-block::
            none

            objref r 
            r = new Random() 
            for i=1,10 print r.uniform(30, 50) // not as efficient as 
            for i=1,10 print r.repick()	   // this 

        prints 20 random numbers ranging in value between 30 and 50. 
         

         

----



.. hoc:method:: Random.ACG


    Syntax:
        ``r.ACG()``

        ``r.ACG(seed)``

        ``r.ACG(seed, size)``


    Description:
        Use a variant of the Linear Congruential Generator (algorithm M) 
        described in Knuth, Art of Computer Programming, Vol. III in 
        combination with a Fibonacci Additive Congruential Generator.  This is 
        a "very high quality" random number generator, Default size is 55, 
        giving a size of 1244 bytes to the structure. Minimum size is 7 (total 
        100 bytes), maximum size is 98 (total 2440 bytes). 

         

----



.. hoc:method:: Random.MLCG


    Syntax:
        ``r.MLCG()``

        ``r.MLCG(seed1)``

        ``r.MLCG(seed1, seed2)``


    Description:
        Use a Multiplicative Linear Congruential Generator.  Not as high 
        quality as the ACG.  It uses only 8 bytes. 

         

----



.. hoc:method:: Random.MCellRan4


    Syntax:
        ``highindex = r.MCellRan4()``

        ``highindex = r.MCellRan4(highindex)``

        ``highindex = r.MCellRan4(highindex, lowindex)``


    Description:
        Use the MCell variant of the Ran4 generator. See :hoc:func:`mcell_ran4`.
        In the no argument case or if the highindex is 0, then the system selects 
        an index which is the random 32 bit integer resulting from 
        an mcell_ran4 call with an index equal to the 
        the number of instances of the Random generator that had been created. 
        Thus, each stream should be statistically independent as long as the 
        highindex values differ by more than the eventual length of the stream. 
        In any case, the 
        initial highindex is returned and can be used to restart an instance 
        of the generator. Use :hoc:func:`mcell_ran4_init` to set the (global)
        low 32 bit index of the generator. The :hoc:meth:`Random.seq` method is useful
        for getting the current sequence number and restarting at that sequence 
        number (highindex). 
        If the lowindex arg is present and nonzero, then that lowindex is used 
        instead of the global one specified by :hoc:func:`mcell_ran4_init`.
        This allows 2^32-1 independent streams that do not overlap. 
         
        Note that for reproducibility, 
        the distribution should be defined AFTER setting the seed since some 
        distributions, such as :hoc:meth:`Random.normal`, hold state information from
        a previous pick from the uniform distribution. 

    .. seealso::
        :hoc:meth:`Random.Random123`

    Example:

        .. code-block::
            none

            objref r, vec, g1, g2, hist 
            r = new Random() 
            index = r.MCellRan4() 
            r.uniform(0, 2) 
            vec = new Vector(1000) 
            g1 = new Graph() 
            g2 = new Graph() 
            g1.size(0, 1000, 0, 2) 
            g2.size(0, 2, 0, 150) 
             
            proc doit() { 
            	g1.erase() g2.erase() 
            	vec.setrand(r) 
            	hist = vec.histogram(0, 2, 0.2) 
            	vec.line(g1) 
            	hist.line(g2, .2) 
            	g1.flush g2.flush 
            } 
            doit() 
             
            variable_domain(&index, 0, 2^32-1) 
            xpanel("MCellRan4 test") 
            xbutton("Sample", "doit()") 
            xpvalue("Original index", &index, 1, "r.MCellRan4(index) doit()") 
            xpanel() 


         

----



.. hoc:method:: Random.Random123

    Syntax:
        ``0 = r.Random123(id1, id2, id3)``

    Description:
        Use the Random123 generator (currently philox4x32 is the crypotgraphic hash 
        used) with the stream identified by the identifiers 0 <= id1,2,3 < 2^32 
        and the global index (see :hoc:meth:`Random.Random123_globalindex`). The counter,
        which increments from 0 to 2^34-1, is initialized to 0 (see :hoc:meth:`Random.seq`).
        If any of the up to 3 arguments are missing, it is assumed 0.
         
        The generators should be usable in the context of threads as long as 
        no instance is used in more than one thread. 
         
        This generator 
        uses a 34bit counter, 3 32 bit identifiers, and a 32 bit global index and 
        is most suitable for managing separate independent, reproducible, restartable 
        streams that are unique to individual cell and synapses in large parallel 
        network models. 
        See: http://www.thesalmons.org/john/random123/papers/random123sc11.pdf 

         

----



.. hoc:method:: Random.Random123_globalindex

    Syntax:
        ``uint32 = r.Random123_globalindex([uint32])``

    Description:
        Gets and sets the global index used by all instances of the Random123 
        instances of Random. 

         

----



.. hoc:method:: Random.seq

    Syntax:
        ``currenthighindex = r.seq()``
        ``r.seq(sethighindex)``

    Description:
        For MCellRan4, 
        Gets and sets the current highindex value when the :hoc:meth:`Random.MCellRan4` is
        in use. This allows restarting the generator at any specified point. 
        Note that the currenthighindex value is incremented every :hoc:meth:`Random.repick`.
        Usually the increment is 1 but some distributions, e.g. :hoc:meth:`Random.poisson`
        can increment by more. Also, some distributions, e.g. :hoc:meth:`Random.normal`,
        pick twice on the first repick but once thereafter. 
         
        For Random123, 
        Gets and sets the counter value which ranges from 0 to 2^34-1. 
        The reason the the greater range is that the internal Random123 generators 
        return 4 uint32 values on each call. So that is done only every 4 picks from 
        the generator. 
         

    Example:

        .. code-block::
            none

            objref r 
            r = new Random() 
            //r.uniform(0,1) 
            r.negexp(1) 
            //r.normal(0,1) 
            mcell_ran4_init(1) 
            r.MCellRan4(1) 
             
            for i=0, 10 print i, r.repick 
             
            r.MCellRan4(1) 
            for i=0, 5 print i, r.repick 
            idum = r.seq 
            print "idum = ", idum 
            for i=6, 10 print i, r.repick 
             
            print "restarting" 
            r.seq(idum) 
            for i=6, 10 print i, r.repick 
             
            print "restarting" 
            r.seq(idum) 
            for i=6, 10 print i, r.repick 


         

----



.. hoc:method:: Random.repick


    Syntax:
        ``r.repick()``


    Description:
        Pick again from the distribution last used. 

         

----



.. hoc:method:: Random.play


    Syntax:
        ``r.play(&var)``


    Description:
        At the beginning of every call to :hoc:func:`fadvance` and :hoc:func:`finitialize` var is set
        to a new value equivalent to 

        .. code-block::
            none

            var = r.repick() 

        (but with no interpreter overhead). This is similar in concept to :hoc:meth:`Vector.play`.
        Play may be called several times for different variables and each variable 
        will get an independent random value but with the same distribution. 
        To disconnect the Random object from its list of variables, either the variables 
        or the Random object must be destroyed. 

    Example:

        .. code-block::
            none

            // run the single script 
            // use the PointProcessManager to select IClamp 
            // set dur of IClamp[0] to 100 
            // open a new Voltage Graph 
            objref r 
            r = new Random() 
            r.poisson(.01) 
            r.play(&IClamp[0].amp) 
            //open a RunControl 
            // press Init&Run several times 


----



.. hoc:method:: Random.uniform


    Syntax:
        ``r.uniform(low, high)``


    Description:
        Create a uniform random variable over the open interval (*low*...\ *high*). 

    Example:

        .. code-block::
            none

            objref r, vec, g1, g2, hist 
            r = new Random() 
            r.uniform(0, 2) 
            vec = new Vector(1000) 
            vec.setrand(r) 
            hist = vec.histogram(0, 2, 0.2) 
             
            g1 = new Graph() 
            g2 = new Graph() 
            g1.size(0, 1000, 0, 2) 
            g2.size(0, 2, 0, 150) 
            vec.plot(g1) 
            hist.plot(g2, .2) 


         

----



.. hoc:method:: Random.discunif


    Syntax:
        ``r.discunif(low, high)``


    Description:
        Create a uniform random variable over the discrete integers from 
        low to high. 

         

----



.. hoc:method:: Random.normal


    Syntax:
        ``r.normal(mean, variance)``


    Description:
        Gaussian distribution. 

    Example:

        .. code-block::
            none

            objref r, g, hist, vec 
            r = new Random() 
            r.normal(-1, .5) 
             
            vec = new Vector() 
            vec.indgen(-3, 2, .1)	// x-axis for plot 
            hist = new Vector(vec.size()) 
            g = new Graph() 
            g.size(-3, 2, 0, 50) 
            hist.plot(g, vec) 
            for(i=0; i<500; i=i+1){ 
            	x = r.repick() 
            	print i, x 
            	j = int((x+3)*10) // -3 to 2 -> 0 to 50 
            	if (j >= 0) { 
            		hist.x[j] = hist.x[j]+1 
            	} 
            	g.flush() 
            	doNotify() 
            } 


         

----



.. hoc:method:: Random.lognormal


    Syntax:
        ``r.lognormal(mean, variance)``


    Description:
        Create a logarithmic normal distribution. 

    Example:

        .. code-block::
            none

            objref r, g, hist, xvec 
            r = new Random() 
            r.lognormal(5,2) 
            n=20 
            xvec = new Vector(n*3)	// bins look like discrete spikes 
            for i=0,n-1 { 
            	xvec.x[3*i] = i-.1 
            	xvec.x[3*i+1] = i 
            	xvec.x[3*i+2] = i+.1 
            } 
            hist = new Vector(xvec.size()) 
            g = new Graph() 
            g.size(0, 15, 0, 120) 
            hist.plot(g, xvec) 
            for(i=0; i<500; i=i+1){ 
            	x = r.repick() 
            	print i, x 
            	j = int(x) 
            	j = 3*j+1 
            	if (j >= hist.size()) { // don't let any off the edge 
            		j = hist.size() -1 
            	} 
            	hist.x[j] = hist.x[j]+1 
            	g.flush() 
            	doNotify() 
            } 


         

----



.. hoc:method:: Random.poisson


    Syntax:
        ``r.poisson(mean)``


    Description:
        Create a poisson distribution. 

    Example:

        .. code-block::
            none

            objref r, g, hist, xvec 
             
            r = new Random() 
            r.poisson(3) 
             
            n=20 
            xvec = new Vector(n*3) 
            for i=0,n-1 { 
            	xvec.x[3*i] = i-.1 
            	xvec.x[3*i+1] = i 
            	xvec.x[3*i+2] = i+.1 
            } 
            hist = new Vector(xvec.size()) 
            g = new Graph() 
            g.size(0, 15, 0, 120) 
            hist.plot(g, xvec) 
            for(i=0; i<500; i=i+1){ 
            	x = r.repick() 
            	print i, x 
            	j = int(x) 
            	j = 3*j+1 
            	if (j >= hist.size()) { 
            		j = hist.size() -1 
            	} 
            	hist.x[j] = hist.x[j]+1 
            	g.flush() 
            	doNotify() 
            } 


         

----



.. hoc:method:: Random.binomial


    Syntax:
        ``r.binomial(N,p)``


    Description:
        Create a binomial distribution. Returns the number of "successes" after 
        *N* trials when the probability of a success after one trial is *p*. 
        (n>0, 0<=p<=1). 
         
        ``P(n, N, p) = p * P(n-1, N-1, p) + (1 - p) * P(n, N-1, p)``

    Example:

        .. code-block::
            none

            objref r, hist, g 
            r = new Random() 
            r.binomial(20, .5) 
             
            g = new Graph() 
            g.size(0, 20, 0, 100) 
            hist = new Vector(20) 
            hist.plot(g) 
            for(i=0; i<500; i=i+1){ 
            	j = r.repick() 
            	hist.x[j] = hist.x[j]+1 
            	g.flush() 
            	doNotify() 
            } 


         

----



.. hoc:method:: Random.geometric


    Syntax:
        ``r.geometric(mean)``


    Description:
        Create a discrete geometric distribution. 
        Given 0<=*mean*<=1, return the number of uniform random samples 
        that were drawn before the sample was larger than the *mean* (always 
        greater than 0). 

    Example:

        .. code-block::
            none

            objref r, hist, g 
            r = new Random() 
            r.geometric(.8) 
            hist = new Vector(1000) 
            proc sample() { 
            	hist = new Vector(1000) 
            	hist.setrand(r) 
            	hist = hist.histogram(0,100,1) 
            	hist.plot(g) 
            } 
            g = new Graph() 
            g.size(0,40,0,200) 
            sample() 
            xpanel("Resample") 
            xbutton("Resample", "sample()") 
            xpanel() 


         

----



.. hoc:method:: Random.hypergeo


    Syntax:
        ``r.hypergeo(mean,variance)``


    Description:
        Create a hypergeometric distribution. 

         

----



.. hoc:method:: Random.negexp


    Syntax:
        ``r.negexp(mean)``


    Description:
        Create a negative exponential distribution. Distributed as the intervals 
        between events in a poisson distribution. 

    Example:

        .. code-block::
            none

            objref r, hist, g 
            r = new Random()  
            r.negexp(2.5)  
            hist = new Vector(1000) 
            proc sample() { 
                    hist = new Vector(1000) 
                    hist.setrand(r) 
                    hist = hist.histogram(0,20,.1) 
                    hist.plot(g, .1) 
            } 
            g = new Graph() 
            g.size(0,20,0,50) 
            sample() 
            xpanel("Resample") 
            xbutton("Resample", "sample()") 
            xpanel() 


         

----



.. hoc:method:: Random.erlang


    Syntax:
        ``r.erlang(mean,variance)``


    Description:
        Create an Erlang distribution. 

         

----



.. hoc:method:: Random.weibull


    Syntax:
        ``r.weibull(alpha,beta)``


    Description:
        Create a Weibull distribution. 

         

