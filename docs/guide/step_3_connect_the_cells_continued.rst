.. _step_3_connect_the_cells_continued:

Step 3. Connect the cells
========

continued
-------

B. We need to specify the parameters of the synaptic connections.
---------


Leave all delays = 1 ms.

For now set the synaptic weights to these values :

.. code::
    python 

    Source  Target  Weight
    S0      M1.E0   0.015
    M1      R2.E0   0.0005
    R2      M1.I1   0.0

These weights were chosen to ensure that M1 and R2 will both fire spikes at a steady rate. Later we will change the weight of R2 -> M1.I1 to 0.01 to see what happens when R2 is allowed to inhibit M1.

Now save the Network Builder to a session file.

Then we'll be almost ready to test our network.










