.. _timer:

Timer
-----



.. class:: Timer(python_func)

    :param python_func: Function to be called periodically
    :type python_func: callable
    :return: A new Timer object
    :rtype: Timer


    Execute a Python function at the end of each interval specified by timer.seconds(interval). 
    The timer must be started and can be stopped. 
    A Timer is used to implement the :menuselection:`NEURON Main Menu --> Tools --> MovieRun` in 
    :file:`nrn/lib/hoc/movierun.hoc`

    .. warning::
        This code must be run with `nrniv -python` and not directly via `python`.
        The better solution is to `use Python's threading module <https://docs.python.org/3/library/threading.html>`_
        which works regardless of how NEURON is launched.
            


    Example:

        .. code-block::
            python

            from neuron import h

            def foo():
                print('Hello')

            timer = h.Timer(foo)
            timer.seconds(1)
            timer.start()
            # type timer.end() to end timer


         

----



.. method:: Timer.seconds([interval])

    :param interval: the timer interval in seconds
    :type interval: float
    :return: ???
    :rtype: ???

    Specify the timer interval. Timer resolution is system dependent but is probably 
    around 10 ms. 
    The time it takes to execute the Python function is part of the interval. 

         

----



.. method:: Timer.start()

    Start the timer. The Python function will be called at the end of each interval defined 
    by the argument to timer.seconds(interval). 

         

----



.. method:: Timer.end()

    Stop calling the Python function. At least on Linux, this will prevent the calling 
    of the function at the end of the current interval. 

         

