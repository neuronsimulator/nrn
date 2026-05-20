Solving Nonlinear Systems
-------------------------

For linear systems, see :meth:`Matrix.solv`.

----    

.. function:: prmat

    .. tab:: Python
    
            Prints the form of the matrix defined by :ref:`eqn <keyword_eqn>` statements. Each nonzero 
            element is printed as an "*". 


    .. tab:: HOC


        Prints the form of the matrix defined by :ref:`eqn <hoc_keyword_eqn>` statements. Each nonzero
        element is printed as an "*". 
        
----

.. function:: solve

    .. tab:: Python
    
        Does one iteration of the non-linear system defined by :ref:`eqn <keyword_eqn>` statements. 
        Returns the linear norm of the difference between left and right hand sides 
        plus the change in the dependent variables. 


    .. tab:: HOC


        Does one iteration of the non-linear system defined by :ref:`eqn <hoc_keyword_eqn>` statements.
        Returns the linear norm of the difference between left and right hand sides 
        plus the change in the dependent variables. 
        
----

.. function:: eqinit

    .. tab:: Python
    
            Throws away previous dependent variable and equation specifications 
            from :ref:`eqn <keyword_eqn>` statements. 


    .. tab:: HOC


        Throws away previous dependent variable and equation specifications 
        from :ref:`eqn <hoc_keyword_eqn>` statements.
        
