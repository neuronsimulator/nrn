Spatial discretization for radial diffusion of calcium (Ca)
==========================================================

**Question:**  
How should radial shells be discretized when modeling calcium dynamics, and how should the calcium concentration be used to activate membrane mechanisms like BK channels?

**Answer:**  
Radial discretization for calcium diffusion differs from the spatial discretization of the cable equation for longitudinal electrical signals. There is no fixed rule like the d_lambda rule for radial diffusion, so discretization must be chosen and validated empirically.

Key points and recommendations:

- The outermost shell in the radial discretization should be half as thick as the inner shells to maintain second-order spatial accuracy at the membrane where BK channels reside. Equal shell widths introduce spatial errors in calcium concentration at the membrane.

- Start with a simple model (single section), varying the number of shells (`NANN`) from coarse (e.g., 4 shells) to fine (e.g., 50 or 250 shells), and compare the calcium concentration (`cai`), BK conductance, and BK current.

- Evaluate whether differences in calcium dynamics and BK currents affect model behavior significantly compared to experimental variability in channel densities (~±20%). Often, differences less than ~30-40% in BK current integral are acceptable.

- Use the calcium concentration in the outermost shell (closest to the membrane) for activating membrane mechanisms like BK channels.

- For modeling compartments with intracellular stores (e.g., ER), fractional volumes can be assigned using COMPARTMENT statements in the KINETIC block, for example 83% cytoplasm and 17% ER volume per shell, with calcium concentrations (`ca` and `caer`) as state variables.

**Example: Calculating shell volumes (Python-like pseudocode)**

.. code-block:: python

    def coord(NANN, diameter):
        import math
        r = diameter / 2.0  # radius
        dr = r / NANN       # shell thickness
        vol = [0]*(NANN)
        frat = [0]*(NANN)
        for i in range(NANN):
            outer = r - i*dr
            inner = r - (i+1)*dr if i < NANN - 1 else r - i*dr - dr/2  # last shell half thickness
            shell_thickness = outer - inner
            vol[i] = math.pi * (outer**2 - inner**2) * 1  # assuming 1 um length
            frat[i] = 2*math.pi*outer / shell_thickness
        return vol, frat

**Example: Using calcium from outermost shell to activate BK channels (Python with NEURON)**

.. code-block:: python

    from neuron import h

    soma = h.Section(name='soma')
    soma.L = soma.diam = 20  # 20 um diameter and length

    # Insert calcium accumulation mechanism with radial shells (example)
    soma.insert('cadifus') # cadifus mechanism models radial Ca shells
    
    # For simplicity, assume 4 shells
    h('NANN = 4')
    
    # Access calcium concentration in outermost shell (index 0 or last depending on implementation)
    # Example: cai_shell0 = soma.cai_shell[0] (pseudocode)
    # Using cai_shell as concentration at outermost shell to activate BK channels.
    
    # Insert BK channel mechanism that depends on outermost shell Ca
    soma.insert('bkchan')
    # Mechanism would internally use soma.cai_shell[0] for activation

**Example: Equivalent in Hoc**

.. code-block:: hoc

    create soma
    access soma
    soma {
        L = 20
        diam = 20
        insert cadifus
        NANN = 4
        insert bkchan
        // bkchan uses calcium concentration from outermost shell, e.g. cai_shell[0]
    }

**Example: Defining volumes for cytoplasm and ER in KINETIC block**

.. code-block:: mod

    KINETIC state {
        COMPARTMENT ii, (1 + bbr) * diam * diam * vol[ii] * 0.83 { ca }   // cytoplasm volume fraction
        COMPARTMENT jj, diam * diam * vol[jj] * 0.17 { caer }             // ER volume fraction

        // Example SERCA pump flux affecting cytosolic and ER Ca concentrations
        dsq = diam * diam

        FROM i = 0 TO NANN - 1 {
            dsqvol = dsq * vol[i] * 0.83
            dsqvoler = dsq * vol[i] * 0.17

            jserca[i] = (-vmaxsr * ca[i]^2 / (ca[i]^2 + kpsr^2))
            ~ ca[i]  << (dsqvol * jserca[i])
            ~ caer[i] << (-dsqvoler * jserca[i])
        }
    }

**Summary:** 
 
- Use an empirically validated radial discretization with thinner outermost shell to ensure accurate calcium concentration at the membrane.  
- Activate membrane mechanisms using calcium concentration in the outermost shell.  
- Test sensitivity by varying shell numbers and confirm impact on BK current and overall model behavior.  
- When modeling compartments like cytoplasm and ER, specify volume fractions explicitly in the kinetics to correctly capture calcium dynamics.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=3485
