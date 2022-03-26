
.. _hoc_timer:

Timer
-----



.. hoc:class:: Timer


    Syntax:
        ``timer = new Timer("stmt")``


    Description:
        Execute "stmt" at the end of each interval specified by timer.seconds(interval). 
        The timer must be started and can be stopped. 
        A Timer is used to implement the :menuselection:`NEURON Main Menu --> Tools --> MovieRun` in 
        :file:`nrn/lib/hoc/movierun.hoc`

    Example:

        .. code-block::
            none

            load_file("nrngui.hoc") 
            objref timer 
            timer = new Timer("p()") 
            invl = .2 
            nstep = 10 
            proc p() {local x 
            	istep += 1 
            	tt = startsw() - t0 
            	print istep, tt 
            	if (istep >= nstep) { 
            		timer.end() 
            	} 
            	doNotify() 
            } 
            proc begin() { 
            	istep = 0 
            	timer.seconds(invl) 
            	t0 = startsw() 
            	tt = 0 
            	timer.start() 
            } 
             
            xpanel("Timer Demo") 
            	xbutton("Start", "begin()") 
            	xbutton("Stop", "timer.end()") 
            	xpvalue("Interval", &invl, 1) 
            	xpvalue("#steps", &nstep, 1) 
            	xpvalue("istep", &istep) 
            	xpvalue("t", &tt) 
            xpanel() 
            begin() 


         

----



.. hoc:method:: Timer.seconds


    Syntax:
        ``interval = timer.seconds()``

        ``interval = timer.seconds(interval)``


    Description:
        Specify the timer interval. Timer resolution is system dependent but is probably 
        around 10 ms. 
        The time it takes to execute the "stmt" is a part of the interval. 

         

----



.. hoc:method:: Timer.start


    Syntax:
        ``timer.start()``


    Description:
        Start the timer. "stmt" will be called at the end of each interval defined 
        by the argument to timer.seconds(interval). 

         

----



.. hoc:method:: Timer.end


    Syntax:
        ``timer.end()``


    Description:
        Stop calling the "stmt". At least on linux, this will prevent the calling 
        of "stmt" at the end of the current interval. 

         

