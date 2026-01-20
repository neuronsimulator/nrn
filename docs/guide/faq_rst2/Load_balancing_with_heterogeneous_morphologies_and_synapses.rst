Load balancing with heterogeneous morphologies and synapses
============================================================

**Question:**  
How can I achieve efficient load balancing in NEURON for a large network with heterogeneous cell morphologies and synaptic distributions? Is the multisplit approach recommended, and how can I assess and improve load balance?

**Answer:**  
For large networks where each principal cell has a unique morphology and synapse distribution, deciding whether to use multisplit or other load balancing strategies depends on the variance in cell computational complexity and the number of hosts:

- If the number of cells is very large and complexity variance is small, simple round-robin distribution of whole cells across hosts may be sufficient.
- Use multisplit when  
  TotalNumberOfStatesForAllCells / number_of_hosts < NumberOfStatesOfLargestCell  
  In this case, splitting individual cells amongst ranks may improve performance.
- A practical approach to estimate cell complexity is to use the built-in `LoadBalance` class, which provides proxies such as number of states or experimental mechanism complexity.

**Measuring cell complexity and load balance:**  
A Python snippet to measure complexity and expected load balance is shown below:

.. code-block:: python

    from neuron import h
    pc = h.ParallelContext()
    nhost = int(pc.nhost())
    rank = int(pc.id())

    h.load_file("loadbal.hoc")

    def measure_complexity():
        lb = h.LoadBalance()
        cell_cx = []
        for sec in h.allsec():
            if sec.parentseg() is None:
                cell_cx.append(lb.cell_complexity(sec=sec))
        max_cx = max(cell_cx) if cell_cx else 0.0
        sum_cx = sum(cell_cx)
        max_cx = pc.allreduce(max_cx, 2)  # max over ranks
        sum_cx = pc.allreduce(sum_cx, 1)  # sum over ranks
        if rank == 0:
            print(f"max_cx = {max_cx}  average cx per rank = {sum_cx/nhost}")

    measure_complexity()
    pc.barrier()
    h.quit()

**Load balancing strategies:**

- Round robin distribution is easy but may not be optimal if complexity varies.
- The *Least Processing Time (LPT)* algorithm can improve balance by assigning the most complex cells to ranks currently with lowest load iteratively.

Pseudocode for LPT:

.. code-block:: text

    REPEAT
      assign the most complex unassigned cell to the host with the least total assigned complexity
    UNTIL all cells are assigned

**Example of using LPT in Python with NEURON:**

.. code-block:: python

    from neuron import h
    pc = h.ParallelContext()
    nhost = int(pc.nhost())
    rank = int(pc.id())

    h.load_file("loadbal.hoc")

    def get_complexity(gidvec):
        lb = h.LoadBalance()
        cxvec = gidvec.c()
        for i, gid in enumerate(gidvec):
            cxvec.x[i] = lb.cell_complexity(pc.gid2cell(gid))
        return cxvec

    def print_load_balance(cxvec):
        sum_cx = sum(cxvec)
        max_sum_cx = pc.allreduce(sum_cx, 2)
        sum_cx = pc.allreduce(sum_cx, 1)
        if rank == 0:
            print(f"Expected load balance: {sum_cx/nhost / max_sum_cx:.2f}")

    def lpt_balance(gidvec, cxvec):
        dest = pc.py_alltoall([ (gidvec, cxvec) if i == rank else None for i in range(nhost) ])
        if rank == 0:
            all_gids = h.Vector()
            all_cx = h.Vector()
            for pair in dest:
                if pair:
                    all_gids.append(pair[0])
                    all_cx.append(pair[1])
            rankvec = h.LoadBalance().lpt(all_cx, nhost, 0)
            src = [(h.Vector(), h.Vector()) for _ in range(nhost)]
            for i in range(len(all_cx)):
                r = int(rankvec.x[i])
                src[r][0].append(all_gids.x[i])
                src[r][1].append(all_cx.x[i])
        else:
            src = [None] * nhost
        balanced = pc.py_alltoall(src)
        return balanced[rank] if rank < len(balanced) else (h.Vector(), h.Vector())

    def test_load_balance(gidvec):
        cxvec = get_complexity(gidvec)
        if rank == 0:
            print("\nCurrent distribution")
        print_load_balance(cxvec)
        balanced_gidvec, balanced_cxvec = lpt_balance(gidvec, cxvec)
        if rank == 0:
            print("\nAfter LPT redistribution")
        print_load_balance(balanced_cxvec)

    # Call with your gid vector:
    # test_load_balance(h.gidvec)

**Additional notes:**

- A more accurate complexity proxy can be obtained with `LoadBalance.ExperimentalMechComplex()`. After running it on all ranks, the resultant data can be read using `LoadBalance.read_mcomplex()` before computing cell complexity.
- The LPT algorithm is much more efficient when implemented in Python (using `heapq`) rather than HOC.
- Load balance can be evaluated dynamically during simulation using timing functions and barriers to measure computation versus communication time.

**Example of load balance measurement during simulation:**

.. code-block:: python

    def prun():
        h.stdinit()
        tstop = h.tstop
        runtime_start = h.startsw()
        pc.psolve(tstop)
        runtime = h.startsw() - runtime_start
        comp_time = pc.step_time()
        cw_time = comp_time + pc.step_wait()
        max_cw_time = pc.allreduce(cw_time, 2)
        avg_comp_time = pc.allreduce(comp_time, 1) / int(pc.nhost())
        load_balance = avg_comp_time / max_cw_time
        return runtime, load_balance, avg_comp_time

**Summary:** 
 
- Evaluate cell complexity distribution to determine if multisplit is needed.
- Use round robin for uniform complexity cells; use advanced algorithms like LPT for heterogeneous complexities.
- Use LoadBalance class routines to assist measuring complexity and generating optimal distributions.
- Consider experimental mechanism complexity for better proxies.
- Use dynamic load balance measures during simulation to guide optimization.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=3628
