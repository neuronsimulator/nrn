#!/usr/bin/env python
# coding: utf-8

# In[1]:


from neuron import h, rxd

h.load_file("stdrun.hoc")


# In[2]:


axon = h.Section(name="s1")
axon_terminal = h.Section(name="s")
axon.connect(axon_terminal)

axon_terminal_region = rxd.Region([axon_terminal], nrn_region="i")
synaptic_cleft = rxd.Region([axon_terminal], nrn_region="o")
terminal_membrane = rxd.Region(h.allsec(), geometry=rxd.DistributedBoundary(1))


# In[3]:


VA = rxd.Species([axon_terminal_region], name="VA", initial=10, charge=1)
T = rxd.Species([synaptic_cleft], name="T", initial=10, charge=1)


# In[4]:

# if you comment the exocytosis line below, the error will go away
exocytosis = rxd.MultiCompartmentReaction(
    VA[axon_terminal_region],
    T[synaptic_cleft],
    500,
    0,
    membrane=terminal_membrane,
    membrane_flux=True,
)


# In[5]:


VA_conc = h.Vector().record(VA.nodes[0]._ref_concentration)
T_conc = h.Vector().record(T.nodes[0]._ref_concentration)
t_vec = h.Vector().record(h._ref_t)

h.finitialize(-70)
h.continuerun(100)

if __name__ == "__main__":
    from matplotlib import pyplot

    pyplot.plot(t_vec, VA_conc)
    pyplot.plot(t_vec, T_conc)
    pyplot.show()
