Miscellaneous
=============

.. hoc:function:: ghk


    Syntax:
        ``ghk(v, ci, co, charge)``


    Description:
        Return the Goldman-Hodgkin-Katz current (normalized to unit permeability). 
        Use the present value of celsius. 

        .. code-block::
            none

            mA/cm2 = (permeability in cm/s)*ghk(mV, mM, mM, valence) 


         

----



.. hoc:function:: nernst


    Syntax:
        ``nernst(ci, co, charge)``

        ``nernst("ena" or "nai" or "nao", [x])``


    Description:


        ``nernst(ci, co, charge)`` 
            returns nernst potential. Utilizes the present value of celsius. 

        ``nernst("ena" or "nai" or "nao", [x])`` 
            calculates ``nao/nai = exp(z*ena/RTF)`` for the ionic variable 
            named in the string. 

        Celsius, valence, and the other two ionic variables are taken from their 
        values at the currently accessed section at position x (.5 default). 
        A hoc error is printed if the ionic species does not exist at this location. 

         
