Adding Many Cell Template Instances Slows Down Simulation Setup – How to Speed It Up?
==========================================================================================

**Question:**  
When creating many instances of a cell template in NEURON, the process slows down exponentially as more cells are instantiated. Is this normal, and how can the speed be improved?

**Short Answer:**  
This slowdown is typically caused by creating too many high-overhead objects such as `PointProcessManager` and `VectorPlay` for each cell. These tools are designed for interactive use with few instances. Replacing them with lighter-weight objects like `IClamp` and using the `Vector` class’s `play()` method to drive stimulation directly dramatically improves speed and reduces memory usage. Additionally, separating “biological” cell definition from “instrumentation” and “control” code improves modularity and debugging.

**Recommendations and Best Practices:**

- Avoid creating thousands of `PointProcessManager` and `VectorPlay` instances; instead, use direct `IClamp` objects for stimulation.
- Use the `Vector.play()` method to drive `IClamp.amp` directly, avoiding the overhead of `VectorPlay`.
- Keep the cell template focused on anatomy and biophysics; attach stimulation and control code outside the template.
- Reduce print/debug output during large loops to save execution time.
- For complex or repeated stimuli with many cells (e.g., sine waves with different delays), define a custom `MOD` file point process that generates the stimulus, thus greatly reducing memory and computational overhead.

---

**Example: Original Slow Template with PointProcessManager and VectorPlay (Hoc)**

.. code-block:: hoc

    begintemplate tpTCN 
        external newseed, grDur, stimulatortime
        public soma,vcell,gnoise,electrode,stimulator
        create soma
        objref vcell,gnoise,electrode,stimulator
        
        proc init() { 
            access soma
            soma {
                nseg = 1
                diam = 76.58
                L = 100
                cm = 1
                insert hh2    
                ena = 50
                ek = -100
                vtraub_hh2 = -52
                gnabar_hh2 = 0.01    
                gkbar_hh2 = 0.01     
                insert itGHK
                ...
                
                gnoise = new Gfluct(0.5) 
                ...
                
                electrode = new PointProcessManager(0) 
                electrode.pp = new IClamp(0)
                electrode.pp.del = 0
                electrode.pp.dur = grDur
                electrode.pp.amp = 0 
            }
            vcell = new Vector()
            vcell.record(&soma.v(0.5))
            stimulartor = new VectorPlay(1)
            stimulartor.vy = new Vector(grDur+1)
            stimulartor.vx = stimulatortime
            stimulartor.vy.play(&electrode.pp.amp, stimulartor.vx)
        }
    endtemplate tpTCN

**Creation loop causing slowdown:**

.. code-block:: hoc

    rowsTCN = 30
    colsTCN = 30
    slabsTCN = 2
    objref TCN[colsTCN][rowsTCN][slabsTCN]
    for col=0, colsTCN-1 for row=0, rowsTCN-1 for slab=0, slabsTCN-1 {
        TCN[col][row][slab] = new tpTCN()
        TCN[col][row][slab].gnoise.g_e0 = 0.05
        print "created TCN[", col,"][", row,"][", slab,"]"
    }

---

**Improved Approach: Use IClamp and Vector.play() Directly (Hoc)**

Create `IClamp` objects directly in hoc, and use `Vector` to play stimulation waveforms:

.. code-block:: hoc

    objref stimvec, timevec
    stimvec = new Vector()
    timevec = new Vector()
    // Define time and amplitude points of stimulus waveform (e.g., sine wave)

    // Create IClamp on soma(0.5)
    objref stim
    stim = new IClamp(0.5)
    stim.del = 0
    stim.dur = 1000  // duration in ms if needed
    stim.amp = 0

    // Play the waveform directly into IClamp.amp
    stimvec.play(&stim.amp, timevec)

---

**Custom Point Process for Sinusoidal Current Injection (MOD file)**

This example defines a point process delivering a sine wave current with parameters for frequency, amplitude, delay, phase, and duration:

.. code-block:: nmodl

    NEURON {
      POINT_PROCESS Isin
      RANGE del, dur, f, phi, amp, i
      ELECTRODE_CURRENT i
    }

    UNITS {
      (nA) = (nanoamp)
      PI = (pi) (1)
      (Radian) = (1)
    }

    PARAMETER {
      del (ms)
      dur (ms) <0, 1e9>
      f (Radian/s) <0, 1e9>
      phi (Radian)
      amp (nA)
    }

    ASSIGNED {
      i (nA)
      on (1)
    }

    INITIAL {
      i = 0
      on = 0
      net_send(del, 1)
    }

    FUNCTION fsin(t (ms)) {
      fsin = sin(f*(t - del)/1000 + phi)
    }

    BREAKPOINT {
      i = on * amp * fsin(t)
    }

    NET_RECEIVE (w) {
      if (flag == 1) {
        if (on == 0) {
          on = 1
          net_send(dur, 1)
        } else {
          on = 0
        }
      }
    }

---

**Python example for creating and stimulating cells with IClamp and Vector.play():**

.. code-block:: python

    from neuron import h

    cells = []
    rowsTCN, colsTCN, slabsTCN = 30, 30, 2
    
    # Define stimulation waveform
    stim_times = h.Vector([0, 10, 20, 30])
    stim_amps = h.Vector([0, 1, -1, 0])
    
    for col in range(colsTCN):
        for row in range(rowsTCN):
            for slab in range(slabsTCN):
                cell = h.Section(name='soma')
                cell.L = 100
                cell.diam = 76.58
                cell.nseg = 1
                cell.insert('hh')
                stim = h.IClamp(cell(0.5))
                stim.delay = 0
                stim.dur = 100
                stim.amp = 0
                stim_amps.play(stim._ref_amp, stim_times)
                cells.append(cell)

---

**Summary:**  
To avoid exponential slowdown when creating many instances of neuron templates, minimize the use of high-overhead instrument objects like `PointProcessManager` and `VectorPlay`. Instead, create lightweight `IClamp` objects and use `Vector.play()` to control stimulus amplitude. For repetitive or complex stimulus patterns shared across many cells, implement a custom `MOD` point process. Keep biological and instrumentation code modular for maintainability and easy debugging.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=1522
