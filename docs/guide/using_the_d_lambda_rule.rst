.. _using_the_d_lambda_rule:

Using the d_lambda Rule
============

The d_lambda rule is built into the CellBuilder and is available by clicking on a checkbox--see the CellBuilder tutorial.

If you don't use the GUI, you can still use the d_lambda rule. Just save this code

.. code::
    c++

    /* Sets nseg in each section to an odd value
        so that its segments are no longer than 
            d_lambda x the AC length constant
        at frequency freq in that section.

        Be sure to specify your own Ra and cm before calling geom_nseg()

        To understand why this works, 
        and the advantages of using an odd value for nseg,
        see  Hines, M.L. and Carnevale, N.T.
                NEURON: a tool for neuroscientists.
                The Neuroscientist 7:123-135, 2001.
    */

    // these are reasonable values for most models
    freq = 100      // Hz, frequency at which AC length constant will be computed
    d_lambda = 0.1

    func lambda_f() { local i, x1, x2, d1, d2, lam
            if (n3d() < 2) {
                    return 1e5*sqrt(diam/(4*PI*$1*Ra*cm))
            }
    // above was too inaccurate with large variation in 3d diameter
    // so now we use all 3-d points to get a better approximate lambda
            x1 = arc3d(0)
            d1 = diam3d(0)
            lam = 0
            for i=1, n3d()-1 {
                    x2 = arc3d(i)
                    d2 = diam3d(i)
                    lam += (x2 - x1)/sqrt(d1 + d2)
                    x1 = x2   d1 = d2
            }
            //  length of the section in units of lambda
            lam *= sqrt(2) * 1e-5*sqrt(4*PI*$1*Ra*cm)

            return L/lam
    }

    proc geom_nseg() {
        soma area(0.5) // make sure diam reflects 3d points
        forall { nseg = int((L/(d_lambda*lambda_f(freq))+0.9)/2)*2 + 1  }
    }


in a file called ``fixnseg.hoc``

After specifying the topolgy, geometry, and biophysics of your model, execute the statements

.. code::
    c++

    xopen("fixnseg.hoc")
    geom_nseg()
