.. _now_that_installed_NEURON_how_do_i_run_it?:

Now that I've installed NEURON, how do I run it?
------------------------------------------------

To start NEURON and bring up the NEURON Main Menu toolbar (which you can use to build new models and load existing ones) :

* UNIX/Linux : type ``nrngui`` or ``nrngui -python`` at the system prompt. (The ``-python`` flag will give you a Python prompt instead of a HOC prompt.)
* OS X : double click on the nrngui icon in the folder where you installed NEURON.
* MSWin : double click on the nrngui icon in the NEURON Program Group (or in the desktop NEURON folder).

To start NEURON from python and bring up the NEURON Main Menu, launch "python" then type

.. code:: python

    from neuron import n, gui

To make NEURON read a file called ``foo.hoc`` when it starts :

* UNIX/Linux : type nrngui foo.hoc at the system prompt. This also works for ses files.
* OS X : drag and drop foo.hoc onto the nrngui icon. This also works for ses files.
* MSWin : use Windows Explorer (not Internet Explorer) to navigate to the directory where foo.hoc is located, and then double click on foo.hoc . This does not work for ses files.


:ref:`macOS users will find additional information here. <using_neuron_on_the_mac>`

To exit NEURON : type ``quit()`` or ``^D`` ("control D") at the ``oc>`` or ``>>>`` prompt, or use :menuselection:`File --> Quit` in the NEURON Main Menu toolbar.
