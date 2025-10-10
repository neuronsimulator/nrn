Templates with a variable number of sections
=============================================

How can I create a NEURON template with a variable number of sections, such as dendrites, that can be specified at instantiation?

In NEURON’s hoc language, the number of sections in a template must be declared at the "top level" with `create` statements. However, it is possible to declare an array of sections and then resize it dynamically inside an initialization procedure. This allows creating templates with a flexible number of sections.

Example in Hoc:

.. code-block:: hoc

    begintemplate Foo
        public dend
        create dend[0]   // initially an empty array
        proc init() { local i
            create dend[$1]    // resize the array to $1 sections
            for i=0, $1-1 dend[i] {
                // specify properties of each dend[i], e.g.:
                dend[i].L = 100
                dend[i].diam = 1
                connect dend[i](0), soma(1)
            }
        }
    endtemplate

    objref cell
    cell = new Foo(5)   // creates a Foo with 5 dendrite sections: dend[0]..dend[4]

Example in Python:

.. code-block:: python

    from neuron import h

    class Foo(h.Section):
        def __init__(self, n):
            self.dend = [h.Section(name='dend[%d]' % i) for i in range(n)]
            for dend in self.dend:
                dend.L = 100
                dend.diam = 1
                # Example connection to soma if it exists
                # dend.connect(self(1))

    cell = Foo(5)  # creates an instance with 5 dendritic sections

This approach maintains hoc’s parsing requirements while allowing flexible template instantiation with a variable number of sections.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=3209
