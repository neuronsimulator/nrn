Reproducing QUB Ion Channel Models in NEURON and MATLAB
==========================================================

Users converting ion channel models from QUB to NEURON and attempting to reproduce equivalent simulations in MATLAB may observe discrepancies in initial steady-state probabilities and resulting channel state traces. The main points to clarify are:

1. **Initial Voltage (v_init) in NEURON:**
   NEURON initializes all voltage-dependent variables, including channel state probabilities, based on the membrane potential `v_init`. If `v_init` differs from the voltage of the initial clamp command (e.g., SEClamp or VClamp), the channel states will start from steady-state values at `v_init`, not at the clamp voltage.  
   **Tip:** Always set `v_init` equal to the initial clamp voltage (e.g., `-20 mV`) to ensure the expected starting probabilities.

2. **Steady-State Probabilities Calculation:**
   Steady-state probabilities in both MATLAB and NEURON can be computed by solving the balance equations for the transition rate matrix `Q`, ensuring the sum of all state probabilities is 1.  
   In MATLAB, this is done by solving a linear system:
   
   ```matlab
   p(:,1) = [Q; ones(1, length(Q))] \ [zeros(length(Q), 1); 1];
   ```
   
   In NEURON, the `INITIAL` block with `SOLVE states STEADYSTATE sparse` achieves similar computations, assuming the voltage is correctly set.

3. **Forward and Backward Transition Rates:**
   In NEURON's KINETIC block, order and assignment of rate variables are important. Ensure that the rates correspond correctly to the forward and reverse transitions:

   Incorrect (may cause simulation mismatch):
   ```
   ~ C1 <-> C2 (C2C1, C1C2)
   ```
   Correct if `C1C2` is forward rate and `C2C1` is backward:
   ```
   ~ C1 <-> C2 (C1C2, C2C1)
   ```
   
4. **Multi-State Models:**
   For models with three or more states, correct rate assignment and initialization are critical to match MATLAB and NEURON results. Also, verify the use of `SEClamp` instead of `VClamp` for voltage clamp experiments, as it may better suit certain voltage protocols.

5. **Debugging:**
   NMODL supports `printf` statements for runtime debugging inside the `.mod` files.

---

**Example: 3-State Channel in NEURON (`QUB_K_CHANNEL.mod`)**

.. code-block:: hoc

    NEURON {
      SUFFIX QUB_K_CHANNEL
      USEION k READ ek WRITE ik
      RANGE gbar, gk, ik
    }
    
    PARAMETER {
      gbar = 0.001 (pS/um2)
      k0_C1_O1 = 0.0005 (/ms)
      k1_C1_O1 = 0.08 (/mV)
      k0_O1_C1 = 0.0006 (/ms)
      k1_O1_C1 = 0.04 (/mV)
      k0_C2_C1 = 0.0008 (/ms)
      k1_C2_C1 = 0.07 (/mV)
      k0_C1_C2 = 0.002 (/ms)
      k1_C1_C2 = 0.03 (/mV)
    }
    
    STATE { O1 C2 C1 }
    ASSIGNED { v (mV) ek (mV) ik (mA/cm2) gk (pS/um2) C2C1 (/ms) C1C2 (/ms) C1O1 (/ms) O1C1 (/ms) }
    
    INITIAL {
      SOLVE states STEADYSTATE sparse
    }
    
    BREAKPOINT {
      SOLVE states METHOD sparse
      gk = gbar * O1
      ik = (1e-4) * gk * (v - ek)
    }
    
    KINETIC states {
      rates(v)
      CONSERVE O1 + C2 + C1 = 1
      ~ C1 <-> C2 (C1C2, C2C1)
      ~ O1 <-> C1 (C1O1, O1C1)
    }
    
    PROCEDURE rates(v (millivolt)) {
      C2C1 = k0_C2_C1 * exp(k1_C2_C1 * v)
      C1C2 = k0_C1_C2 * exp(k1_C1_C2 * v)
      C1O1 = k0_C1_O1 * exp(k1_C1_O1 * v)
      O1C1 = k0_O1_C1 * exp(k1_O1_C1 * v)
    }

**Example: Equivalent MATLAB Code Snippet**

.. code-block:: python

    import numpy as np
    from scipy.linalg import expm
    
    time_end = 500  # ms
    v_rev = -77
    V = np.zeros(time_end)
    V[:100] = -20
    V[100:] = 60
    
    gbar = 0.001
    k0 = {'C1O1': 0.0005, 'O1C1':0.0006, 'C2C1':0.0008, 'C1C2':0.002}
    k1 = {'C1O1': 0.08,  'O1C1':0.04,  'C2C1': 0.07,  'C1C2': 0.03}
    
    states = ['O1', 'C2', 'C1']
    n_states = len(states)
    
    p = np.zeros((n_states, time_end+1))
    I = np.zeros(time_end)
    C = np.array([gbar, 0, 0])  # Conductance vector
    
    def compute_Q(Vm):
        Q = np.zeros((n_states, n_states))
        # Rates at Vm
        C2C1 = k0['C2C1'] * np.exp(k1['C2C1'] * Vm)
        C1C2 = k0['C1C2'] * np.exp(k1['C1C2'] * Vm)
        C1O1 = k0['C1O1'] * np.exp(k1['C1O1'] * Vm)
        O1C1 = k0['O1C1'] * np.exp(k1['O1C1'] * Vm)
        # Fill matrix Q with transitions as per NEURON model
        # Indices: O1=0, C2=1, C1=2
        Q[1,2] = C2C1
        Q[2,1] = C1C2
        Q[2,0] = C1O1
        Q[0,2] = O1C1
        for i in range(n_states):
            Q[i,i] = -np.sum(Q[:,i])
        return Q
    
    # Initial steady state
    Q0 = compute_Q(V[0])
    A = np.vstack([Q0, np.ones(n_states)])
    b = np.hstack([np.zeros(n_states), 1])
    p[:, 0] = np.linalg.lstsq(A.T, b, rcond=None)[0]
    
    for t in range(time_end):
        Q = compute_Q(V[t])
        A_t = expm(Q)
        p[:, t+1] = A_t.dot(p[:, t])
        p[:, t+1] /= np.sum(p[:, t+1])
        I[t] = 1e-4 * C.dot(p[:, t+1]) * (V[t] - v_rev)
        
    # plot p[0,:] to observe open state probability

---

**Summary:**  
Ensure that the initial membrane voltage `v_init` matches the voltage clamp’s first command to obtain consistent initial steady-state probabilities in NEURON. Carefully define forward and backward rate constants in the KINETIC block to match your conceptual model. When reproducing models in MATLAB, construct and solve the transition rate matrix similarly to verify correctness. Use `printf` statements in NMODL for debugging state variables as needed.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=2459
