import sys


class coreneuron(object):
    """
    CoreNEURON configuration values.

    An instance of this class stands in as the `neuron.coreneuron` module. Using
    a class instead of a module allows getter/setter methods to be used, which
    lets us ensure its properties have consistent types and values.

    Attributes
    ----------
    cell_permute
    enable
    file_mode
    gpu
    prcellstate
    verbose
    warp_balance

    Examples
    --------
    >>> from neuron import coreneuron
    >>> coreneuron.enable = True
    >>> coreneuron.enable
    True
    """

    def __init__(self):
        self._enable = False
        self._gpu = False
        self._num_gpus = 0
        self._file_mode = False
        self._cell_permute = None
        self._warp_balance = 0
        self._verbose = 2  # INFO
        self._prcellstate = -1
        self._model_stats = False

    def _default_cell_permute(self):
        return 1 if self._gpu else 0

    def valid_cell_permute(self):
        return {1, 2} if self._gpu else {0, 1}

    @property
    def enable(self):
        """Use CoreNEURON when calling ParallelContext.psolve(tstop)."""
        return self._enable

    @enable.setter
    def enable(self, value):
        self._enable = bool(int(value))

    @property
    def gpu(self):
        """Activate GPU computation in CoreNEURON."""
        return self._gpu

    @gpu.setter
    def gpu(self, value):
        self._gpu = bool(int(value))
        # If cell_permute has been set to a value incompatible with the new GPU
        # setting, change it and print a warning.
        if (
            self._cell_permute is not None
            and self._cell_permute not in self.valid_cell_permute()
        ):
            print(
                "WARNING: coreneuron.gpu = {} forbids coreneuron.cell_permute = {}, using {} instead".format(
                    self._gpu, self._cell_permute, self._default_cell_permute()
                )
            )
            self._cell_permute = self._default_cell_permute()

    @property
    def num_gpus(self):
        """Get/set the number of GPU to use.
        0 means to use all that are avilable.
        """
        return self._num_gpus

    @num_gpus.setter
    def num_gpus(self, value):
        self._num_gpus = int(value)

    @property
    def file_mode(self):
        """Run via file transfer mode instead of in-memory transfer."""
        return self._file_mode

    @file_mode.setter
    def file_mode(self, value):
        self._file_mode = bool(value)

    @property
    def cell_permute(self):
        """Get/set the cell permutation order. Possible values are:
        - 0: no permutation (requires gpu = False)
        - 1: optimize node adjacency
        - 2: optimise parent node adjacency (requires gpu = True)
        """
        if self._cell_permute is None:
            return self._default_cell_permute()
        return self._cell_permute

    @cell_permute.setter
    def cell_permute(self, value):
        value = int(value)
        assert value in self.valid_cell_permute()
        self._cell_permute = value

    @property
    def warp_balance(self):
        """Number of warps to balance. A value of 0 indicates no balancing
        should be done. This has no effect without cell_permute = 2 (and
        therefore gpu = True)."""
        return self._warp_balance

    @warp_balance.setter
    def warp_balance(self, value):
        value = int(value)
        assert value >= 0
        self._warp_balance = value

    @property
    def verbose(self):
        """Configure the verbosity of CoreNEURON. Possible values are:
        - 0: quiet/no output
        - 1: error
        - 2: info (default)
        - 3: debug
        """
        return self._verbose

    @verbose.setter
    def verbose(self, value):
        value = int(value)
        assert value >= 0 and value <= 3
        self._verbose = value

    @property
    def prcellstate(self):
        """Output prcellstate information for the given gid at t=0 and at tstop.
        A value of -1 indicates no prcellstate output will be generated.
        """
        return self._prcellstate

    @prcellstate.setter
    def prcellstate(self, value):
        value = int(value)
        assert value >= -1
        self._prcellstate = value

    @property
    def model_stats(self):
        """Print number of instances of each mechanism and detailed memory stats."""
        return self._model_stats

    @model_stats.setter
    def model_stats(self, value):
        self._model_stats = bool(value)

    def nrncore_arg(self, tstop):
        """
        Return str that can be used for pc.nrncore_run(str)
        based on user properties and current NEURON settings.
        :param tstop: Simulation time (ms)

        """
        from neuron import h

        arg = ""
        tstop = float(tstop)
        if not self.enable:
            return arg

        # note that this name is also used in C++ file nrncore_write.cpp
        CORENRN_DATA_DIR = "corenrn_data"

        # args derived from user properties
        if self._gpu:
            arg += " --gpu"
            if self._num_gpus:
                arg += " --num-gpus %d" % self._num_gpus
        if self._file_mode:
            arg += " --datpath %s" % CORENRN_DATA_DIR
        arg += " --tstop %g" % tstop
        arg += " --cell-permute %d" % self.cell_permute
        if self._warp_balance > 0:
            arg += " --nwarp %d" % self.warp_balance
        if self._prcellstate >= 0:
            arg += " --prcellgid %d" % self.prcellstate
        arg += " --verbose %d" % self.verbose
        if self._model_stats:
            arg += " --model-stats"

        # args derived from current NEURON settings.
        pc = h.ParallelContext()
        cvode = h.CVode()
        if pc.nhost() > 1:
            arg += " --mpi"
        arg += " --voltage 1000."
        if cvode.queue_mode() & 1:
            arg += " --binqueue"
        spkcompress = int(pc.spike_compress(-1))
        if spkcompress:
            arg += " --spkcompress %g" % spkcompress

        # since multisend is undocumented in NEURON, perhaps it would be best
        # to make the next three arg set from user properties here
        multisend = int(pc.send_time(8))
        if (multisend & 1) == 1:
            arg += " --multisend"
            interval = 2 if multisend & 4 else 1
            arg += " --ms-subintervals %d" % interval
            phases = 2 if multisend & 8 else 1
            arg += " --ms-phases %d" % phases

        return arg


sys.modules[__name__] = coreneuron()
