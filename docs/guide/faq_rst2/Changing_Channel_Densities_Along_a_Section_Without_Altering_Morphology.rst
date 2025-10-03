Changing Channel Densities Along a Section Without Altering Morphology
=========================================================================

**Q:** How can I vary the density of ion channels at different positions along a dendritic section in NEURON without changing the section lengths or geometry?

**A:** Instead of subdividing sections into smaller pieces, you can use *range variables* to assign ion channel densities as a function of the normalized position along the section. This approach preserves the neuron’s morphology while allowing fine spatial control of channel properties.

**Key points:**

- The number of segments `nseg` should be set sufficiently high (preferably an odd number) to allow spatial variation.
- Channel density parameters (e.g., `gnabar_hh`) can be assigned at any location along the section using the syntax `section_name param_name(x) = value`, where `x` ranges from 0 (start) to 1 (end).
- This feature avoids the need to manually split sections to achieve spatially varying channel densities.

Example in Python::

    from neuron import h
    
    soma = h.Section(name='soma')
    soma.L = 30
    soma.diam = 30
    soma.nseg = 9  # Use an odd number to ensure proper spatial discretization
    
    soma.insert('hh')
    
    # Set sodium channel density at different normalized positions along soma
    soma.gnabar_hh[0](0.1) = 0.01  # near the beginning
    soma.gnabar_hh[0](0.5) = 0.05  # middle
    soma.gnabar_hh[0](0.9) = 0.09  # near the end
    
    print(soma.gnabar_hh[0](0.1))  # 0.01
    print(soma.gnabar_hh[0](0.5))  # 0.05
    print(soma.gnabar_hh[0](0.9))  # 0.09

Example in Hoc::

    create soma
    soma {
        L = 30
        diam = 30
        nseg = 9  // Set odd number of segments
        insert hh
        
        gnabar_hh(.1) = 0.01  // Set channel density near start
        gnabar_hh(.5) = 0.05  // Set channel density in middle
        gnabar_hh(.9) = 0.09  // Set channel density near end
        
        print gnabar_hh(.1)
        print gnabar_hh(.5)
        print gnabar_hh(.9)
    }

This method enables gradient or region-specific channel densities without changing the morphology. Refer to NEURON’s documentation on range variables and the CellBuilder tutorials for additional guidance.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=2718
