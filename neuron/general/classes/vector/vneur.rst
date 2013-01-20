.. _vneur:

trigavg
-------



.. class:: trigavg


    Syntax:
        :code:`v1.trigavg(data,trigger,pre,post)`


    Description:
        Perform an event-triggered average of <*data*> using times given by 
        <*trigger*>. The duration of the average is from -<*pre*> to <*post*>. 
        This is useful, for example, in calculating a spike triggered stimulus 
        average. 

         

----



.. method:: trigavg.spikebin


    Syntax:
        :code:`v.spikebin(data,thresh)`


    Description:
        Used to make a binary version of a spike train.  <*data*> is a vector 
        of membrane potential.  <*thresh*> is the voltage threshold for spike 
        detection.  <*v*> is set to all zeros except at the onset of spikes 
        (the first dt which the spike crosses threshold) 

         

----



.. method:: trigavg.psth


    Syntax:
        :code:`vmeanfreq = vdest.psth(vsrchist,dt,trials,size)`


    Description:
        The name of this function is somewhat misleading, since its 
        input, vsrchist, is a finely-binned post-stimulus time histogram, 
        and its output, vdest, is an array whose elements are the mean 
        frequencies f_mean[i] that correspond to each bin of vsrchist. 
         
        For bin i, the corresponding mean frequency f_mean[i] is 
        determined by centering an adaptive square window on i and 
        widening the window until the number of spikes under the 
        window equals size.  Then f_mean[i] is calculated as 
         
        \ :code:`  f_mean[i] = N[i] / (m dt trials) ` 
         
        where 

        .. code-block::
            none

              f_mean[i] is in spikes per _second_ (Hz). 
              N[i] = total number of events in the window 
                       centered on bin i 
              m = total number of bins in the window 
                       centered on bin i 
              dt = binwidth of vsrchist in _milliseconds_ 
                       (so m dt is the width of the window in milliseconds) 
              trials = an integer scale factor 

         
        trials is used to adjust for the number of traces that were 
        superimposed to compute the elements of vsrchist.  In other words, 
        suppose the elements of vsrchist were computed by adding up the 
        number of spikes in n traces 

        .. code-block::
            none

                          n 
              v1.x[i] = SUMMA # of spikes in bin i of trace j 
                        j = 1 

        Then trials would be assigned the value n.  Of course, if 
        the elements of vsrchist are divided by n before calling psth(), 
        then trials should be set to 1. 
         
        Acknowledgment: 
        The documentation and example for psth was prepared by Ted Carnevale. 

    .. warning::
        The total number of spikes in vsrchist must be greater than size. 

    Example:
        objref g1, g2, b 
        b = new VBox() 
        b.intercept(1) 
        g1 = new Graph() 
        g1.size(0,200,0,10) 
        g2 = new Graph() 
        g2.size(0,200,0,10) 
        b.intercept(0) 
        b.map("psth and mean freq") 

        .. code-block::
            none

            VECSIZE = 200 
            MINSUM = 50 
            DT = 1000	// ms per bin of v1 (vsrchist) 
            TRIALS = 1 
             
            objref v1, v2 
            v1 = new Vector(VECSIZE) 
               
            objref r 
            r = new Random() 
                        
               
            for (ii=0; ii<VECSIZE; ii+=1) { 
            	v1.x[ii] = int(r.uniform(0,10)) 
            } 
            v1.plot(g1) 
             
            v2 = new Vector() 
            v2.psth(v1,DT,TRIALS,MINSUM) 
            v2.plot(g2) 


         

----



.. method:: trigavg.inf


    Syntax:
        :code:`v.inf(i,dt,gl,el,cm,th,res,[ref])`


    Description:
        Simulate a leaky integrate and fire neuron.  <*i*> is a vector containing 
        the input.  <*dt*> is the timestep.  <*gl*> and <*el*> are the conductance 
        and reversal potential of the leak term <*cm*> is capacitance.  <*th*> 
        is the threshold voltage and <*res*> is the reset voltage. <*ref*>, if 
        present sets the duration of ab absolute refractory period. 
         
        N.b. Currently working with forward Euler integration, which may give 
        spurious results. 

         
         

----



.. method:: trigavg.resample


    Syntax:
        :code:`v1.resample(v2,rate)`


    Description:
        Resamples the vector at another rate -- integers work best. 

    .. seealso::
        :func:`copy`


