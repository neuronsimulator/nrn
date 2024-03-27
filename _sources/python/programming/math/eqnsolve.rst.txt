Solving Nonlinear Systems
-------------------------

For linear systems, see :meth:`Matrix.solv`.

----    

.. function:: prmat

        Prints the form of the matrix defined by :ref:`eqn <keyword_eqn>` statements. Each nonzero 
        element is printed as an "*". 


----

.. function:: solve

        Does one iteration of the non-linear system defined by :ref:`eqn <keyword_eqn>` statements. 
        Returns the linear norm of the difference between left and right hand sides 
        plus the change in the dependent variables. 


----

.. function:: eqinit

        Throws away previous dependent variable and equation specifications 
        from :ref:`eqn <keyword_eqn>` statements. 


