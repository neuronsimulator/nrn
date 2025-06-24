Obsolete Stimuli
----------------

.. warning::
    
    None of these functions are recommended for use in new code.
    They are provided for backward compatibility with old code.

.. function:: fstim

    .. tab:: Python

        Consider this obsolete.  Nevertheless, it does work.

        ``n.fstim(maxnum)``

            allocates space for maxnum synapses. Space for
            previously existing synapses is released. All synapses initialized to
            0 maximum conductance.

        ``n.fstim(i, loc, delay, duration, stim)``

            The ith current stimulus is injected at parameter `loc'
            different current stimuli do not concatenate but can ride on top of
            each other. delay refers to onset of stimulus relative to t=0
            delay and duration are in msec.
            stim in namps.

        ``n.fstimi(i)``

            returns stimulus current for ith stimulus at the value of the
            global time t.


    .. tab:: HOC

        Consider this obsolete.  Nevertheless, it does work.

        ``fstim(maxnum)``

            allocates space for maxnum synapses. Space for
            previously existing synapses is released. All synapses initialized to
            0 maximum conductance.

        ``fstim(i, loc, delay, duration, stim)``

            The ith current stimulus is injected at parameter `loc'
            different current stimuli do not concatenate but can ride on top of
            each other. delay refers to onset of stimulus relative to t=0
            delay and duration are in msec.
            stim in namps.

        ``fstimi(i)``

            returns stimulus current for ith stimulus at the value of the
            global time t.
        
----



.. function:: fstimi

    .. tab:: Python
    
    
        Syntax:
            ``n.fstimi()``


        Description:
            Obsolete 

         


    .. tab:: HOC


        Syntax:
            ``fstimi()``
        
        
        Description:
            Obsolete 
        
----



.. function:: fclamp

    .. tab:: Python
    
    
        Syntax:
            ``n.fclamp()``


        Description:
            obsolete. Use the :class:`VClamp` or :class:`SEClamp` point process. 

         

    .. tab:: HOC


        Syntax:
            ``fclamp()``
        
        
        Description:
            obsolete. Use the :class:`VClamp` or :class:`SEClamp` point process.
        
----



.. function:: fclampi

    .. tab:: Python
    
    
        Syntax:
            ``n.fclampi()``


        Description:
            obsolete. Use the :class:`VClamp` or :class:`SEClamp` point process. 

         

    .. tab:: HOC


        Syntax:
            ``fclampi()``
        
        
        Description:
            obsolete. Use the :class:`VClamp` or :class:`SEClamp` point process.
        
----



.. function:: fclampv

    .. tab:: Python
    
    
        Syntax:
            ``n.fclampv()``


        Description:
            obsolete. Use the :class:`VClamp` or :class:`SEClamp` point process. 

         

    .. tab:: HOC


        Syntax:
            ``fclampv()``
        
        
        Description:
            obsolete. Use the :class:`VClamp` or :class:`SEClamp` point process.
        
----



.. function:: prstim

    .. tab:: Python
    
    
        Syntax:
            ``n.prstim()``


        Description:
            obsolete. Print the info about ``fstim``, ``fclamp``, and ``fsyn`` 


    .. tab:: HOC


        Syntax:
            ``prstim()``
        
        
        Description:
            obsolete. Print the info about ``fstim``, ``fclamp``, and ``fsyn`` 
        
