
# Properties settable by user
enable = False     # Use CoreNEURON when calling ParallelContext.psolve(tstop).
gpu = False        # Activate GPU computation.
file_mode = False  # Run via file transfer mode instead of in-memory transfer
cell_permute = 1   # 0 no permutation; 1 optimize node adjacency
                   # 2 optimize parent node adjacency (only for gpu = True)
warp_balance = 0   # Number of warps to balance. (0 no balance)
verbose = 0        # 0 quiet
prcellstate = -1   # Output prcellstate information for the gid at t=0
                   # and at tstop. -1 means no output

def property_check(tstop):
  '''
  Check type and value in range for the user properties.
  '''
  global enable, gpu, cell_permute, warp_balance, verbose, prcellstate
  tstop = float(tstop) # error if can't be a float
  enable = bool(int(enable))
  gpu = bool(gpu)
  cell_permute = int(cell_permute)
  assert(cell_permute in range(3))
  assert(cell_permute in (range(3) if gpu else range(2)))
  warp_balance = int(warp_balance)
  assert(warp_balance >= 0)
  verbose = int(verbose)
  prcellstate = int(prcellstate)

def nrncore_arg(tstop):
  """
  Return str that can be used for pc.nrncore_run(str)
  based on user properties and current NEURON settings.
  :param tstop: Simulation time (ms)

  """
  from neuron import h
  arg = ''
  property_check(tstop)
  if not enable:
    return arg

  # note that this name is also used in C++ file nrncore_write.cpp
  CORENRN_DATA_DIR = "corenrn_data"

  # args derived from user properties
  arg += ' --gpu' if gpu else ''
  arg += ' --datpath %s' % CORENRN_DATA_DIR if file_mode else ''
  arg += ' --tstop %g' % tstop
  arg += (' --cell-permute %d' % cell_permute) if cell_permute > 0 else ''
  arg += (' --nwarp %d' % warp_balance) if warp_balance > 0 else ''
  arg += (' --prcellgid %d' % prcellstate) if prcellstate >= 0 else ''
  arg += (' --verbose %d' % verbose) if verbose > 0 else ''

  # args derived from current NEURON settings.
  pc = h.ParallelContext()
  cvode = h.CVode()
  arg += ' --mpi' if pc.nhost() > 1 else ''
  arg += ' --voltage 1000.'
  binqueue = cvode.queue_mode() & 1
  arg += ' --binqueue' if binqueue else ''
  spkcompress = int(pc.spike_compress(-1))
  arg += (' --spkcompress %g' % spkcompress) if spkcompress else ''

  # since multisend is undocumented in NEURON, perhaps it would be best
  # to make the next three arg set from user properties here
  multisend = int(pc.send_time(8))
  if (multisend & 1) == 1:
    arg += ' --multisend'
    interval = (multisend & 2)/2 + 1
    arg += (' --ms_subinterval %d' % interval) if interval != 2 else ''
    phases = (multisend & 4)/4 + 1
    arg += (' --ms_phases %d' % phases)  if phases != 2 else ''

  return arg


if __name__ == '__main__':
  enable = True
  print(nrncore_arg(5.0))
  gpu=True
  cell_permute=2
  warp_balance=32
  prcellstate = 7
  pc = h.ParallelContext()
  # multisend cannot be used with cmake build
  #pc.spike_compress(0, 0, 1)
  print(nrncore_arg(10.0))
