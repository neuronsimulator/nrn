.. _using_nsg_portal:

Using the Neuroscience Gateway (NSG) portal
===========================================

You can do this exercise on your own, or you might prefer to form a team of 2-4 people to work on it.

On your own computer:
---------------------

Whether you're working on your own or in a team, everyone should start by downloading the zip file for the `Jones et al. 2009 model <https://modeldb.yale.edu/136803>`_ from ModelDB. Also get a copy of their paper and examine it to discover what is being modeled and what results are produced.

Expand the zip file and examine its contents to figure out how to run a simulation (look for a "readme" file).

Each member of the team shoudl compile the mod files and run a simulation on their own PC with 1 processor.

What output files were generated?

How long did your run take to complete?

What file contains this information?

Now run simulations with 2 processors and 4 processors and record the run times.

For each number of hosts, plot the run time vs. log2(number of hosts). Compare your results with those of your teammates. Do these graphs demonstrate strong scaling or weak scaling?

Using NSG:
----------

Each member of the team should work through the tutorial at https://kenneth59715.github.io/NSGNEURON/

In that tutorial you will

* Upload the model's zip file to NSG.
* Set up a task that runs a single simulation on 1, 2, 4, or 8 processors on Comet using NEURON 7.5 (each member of the team should choose a different number of processors).
* Execute the simulation.
* Download :file:`output.tar.gz`, and expand and explore the contents of this file.
* Analyze the results

How long did your run take to complete on Comet?

Share your run times on Comet with each other and with at least one other team.

For each number of hosts, plot the average run time vs. log2 number of hosts. Does this plot demonstrate strong scaling or weak scaling? How do these run times compare to the run times you and your team members got with your own hardware?

Examine the other output files. What kind of data is contained in :file:`Mu_output.dat`? How about :file:`out.dat`? Examine the model's source code to determine if your guesses are correct.

Make a graph that plots the results in :file:`Mu_output.dat`, and another graph that plots the results in :file:`out.dat`.

