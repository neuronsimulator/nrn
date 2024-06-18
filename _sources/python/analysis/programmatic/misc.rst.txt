Miscellaneous
=============

.. function:: ghk


    Syntax:
        ``h.ghk(v, ci, co, charge)``


    Description:
        Return the Goldman-Hodgkin-Katz current (normalized to unit permeability). 
        Use the present value of celsius. 

        .. code-block::
            none

            mA/cm2 = (permeability in cm/s)*h.ghk(mV, mM, mM, valence) 


         

----



.. function:: nernst


    Syntax:
        ``h.nernst(ci, co, charge, sec=section)``

        ``h.nernst("ena" or "nai" or "nao", [x], sec=section)``


    Description:


        ``h.nernst(ci, co, charge)`` 
            returns nernst potential. Utilizes the present value of celsius. 

        ``h.nernst("ena" or "nai" or "nao", [x])`` 
            calculates ``nao/nai = exp(z*ena/RTF)`` for the ionic variable 
            named in the string. 

        Celsius, valence, and the other two ionic variables are taken from their 
        values at the ``section`` at position x (.5 default). 
        An error is printed if the ionic species does not exist at this location. 

         
