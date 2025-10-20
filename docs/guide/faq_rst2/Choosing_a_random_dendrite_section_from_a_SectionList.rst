Choosing a random dendrite section from a SectionList
=======================================================

Q: How can I select a random dendrite section from a SectionList in NEURON to connect a synapse?

A: While a SectionList does not provide direct methods to index and select individual sections, you can use the SectionRef class to create a List of SectionRefs referring to sections in your SectionList. This allows indexed access and random selection of sections. For placing synapses randomly with probability proportional to surface area, it is common to use a cumulative sum of areas to map random numbers to a particular segment.

**Method 1: Using SectionRef to index sections**

.. code-block:: hoc

    objref srlist
    srlist = new List()
    // Assuming sl is your SectionList containing dendrites
    forsec sl {
        srlist.append(new SectionRef())
    }
    printf("Number of sections in srlist: %d\n", srlist.count())
    
    // Select a random section index
    srand(seed)  // seed with an integer
    int rand_index = int(rand()*srlist.count())
    sec = srlist.o(rand_index).sec
    printf("Randomly selected section: %s\n", secname())

**Method 2: Random synapse placement proportional to surface area**

1. Compute cumulative sum of segment areas over all sections in the SectionList.

2. Draw a random number between 0 and total surface area.

3. Find the segment corresponding to this number.

4. Place synapse at the position within the section corresponding to the segment.

.. code-block:: hoc

    objref myseclist, acusum
    myseclist = new SectionList()
    // populate myseclist with dendrite sections

    // Function to compute cumulative sum of area
    proc makecusum() { local i localobj tmpvec
        tmpvec = new Vector()
        forsec myseclist for (x,0) tmpvec.append(area(x))
        for i=1,tmpvec.size()-1 tmpvec.x[i] += tmpvec.x[i-1]
        return tmpvec
    }

    acusum = new Vector()
    acusum = makecusum()
    objref syn
    // total area is last element of acusum
    total_area = acusum.x[acusum.size()-1]

    // For each synapse to place:
    for i=0,49 {
        randloc = total_area * rand()
        index = acusum.nearest(randloc)  // find segment index
        // find section and segment corresponding to index (exercise for user)
        // attach synapse at that segment location
        syn = new ExpSyn(section(x))
        syn.tau = 2
        // ...
    }

**Method 3: Random selection uniformly from SectionList**

If you want to select a random section uniformly (not weighted by surface area):

.. code-block:: python

    import random
    from neuron import h

    # Assuming sl is a h.SectionList containing dendrites
    num_sections = sl.count()
    rand_index = random.randint(0, num_sections - 1)
    # To access the section at rand_index, use SectionRef list
    srlist = h.List()
    for sec in sl:
        srlist.append(h.SectionRef(sec=sec))
    random_sec = srlist.o(rand_index).sec
    print("Randomly chosen section: ", random_sec.name())

.. code-block:: hoc

    objref sl, srlist
    sl = new SectionList()
    // populate sl

    srlist = new List()
    forsec sl {
        srlist.append(new SectionRef())
    }
    int n = srlist.count()
    srand(seed)
    int rand_index = int(rand() * n)
    objref secref
    secref = srlist.o(rand_index)
    printf("Random section: %s\n", secname(secref.sec))

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=245
