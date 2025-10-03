netstim and random seed control
===============================

**Question:**  
How can I ensure that multiple NetStim objects in NEURON generate reproducible and independent random spike trains by fixing their random seeds?

**Answer:**  
By default, all NetStim objects share the same global random number generator. This causes their spike trains to be interdependent: changing parameters of one NetStim affects the spike times of the others. To generate reproducible and independent random spike trains from multiple NetStim instances, each NetStim should be associated with its own `Random` object initialized with a distinct random seed or random stream identifier. This allows each NetStim to use an independent random sequence.

Additionally, calling `seq(1)` on each `Random` object resets the random sequence, enabling reproducible simulations.

**Example in Hoc:**

.. code-block:: hoc

    objref nslist, rslist
    nslist = new List() // List for NetStims
    rslist = new List() // List for Randoms
    
    // Random123 stream identifiers (nonzero integers)
    id1 = 1
    id2 = 2
    id3 = 3
    
    proc makenetstims() {
        local i, ns, rs
        nslist = new List()
        rslist = new List()
        for i=0,4 {  // create 5 NetStims
            rs = new Random()
            rs.Random123(id1, id2, id3 + i) // distinct streams for each
            rslist.append(rs)
            rs.negexp(1) // specify exponential distribution with mean 1
            ns = new NetStim()
            ns.noiseFromRandom(rs)
            nslist.append(ns)
        }
    }
    
    makenetstims()
    
    proc setnsparams() {
        local i
        for i=0, nslist.count()-1 {
            nslist.o(i).interval = 50
            nslist.o(i).number = 200
            nslist.o(i).start = 0
            nslist.o(i).noise = 1
        }
    }
    
    setnsparams()
    
    proc restart() {
        local i
        for i=0, rslist.count()-1 {
            rslist.o(i).seq(1) // reset random sequences
        }
    }

**Example in Python:**

.. code-block:: python

    from neuron import h
    
    nslist = []
    rslist = []
    id1, id2, id3 = 1, 2, 3  # Random123 stream identifiers
    
    def makenetstims(num_netstims=5):
        global nslist, rslist
        nslist = []
        rslist = []
        for i in range(num_netstims):
            rs = h.Random()
            rs.Random123(id1, id2, id3 + i)  # distinct streams
            rs.negexp(1)  # exponential distribution with mean 1
            ns = h.NetStim()
            ns.noiseFromRandom(rs)
            nslist.append(ns)
            rslist.append(rs)
    
    def setnsparams(interval=50, number=200, start=0, noise=1):
        for ns in nslist:
            ns.interval = interval
            ns.number = number
            ns.start = start
            ns.noise = noise
    
    def restart():
        for rs in rslist:
            rs.seq(1)  # reset random sequences
    
    
    # Usage:
    makenetstims()
    setnsparams()
    restart()

**Summary:**  
Associate each NetStim with a separate `Random` object initialized with a unique seed or stream, and call `seq(1)` on all to reset their sequences. This procedure ensures independent, reproducible, and controllable random spike trains from multiple NetStims.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=4388
