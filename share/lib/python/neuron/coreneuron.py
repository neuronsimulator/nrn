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
        self._file_mode = False
        self._cell_permute = None
        self._warp_balance = 0
        self._verbose = 2
        self._prcellstate = -1

    def _valid_cell_permute(self):
        return {1, 2} if self._gpu else {0, 1}

    def _default_cell_permute(self):
        return 1 if self._gpu else 0

    @property
    def enable(self):
        """Use CoreNEURON when calling ParallelContext.psolve(tstop)."""
        return self._enable

    @enable.setter
    def enable(self, value):
        self._enable = bool(int(value))

    @property
    def gpu(self):
        """Activate GPU computation."""
        return self._gpu

    @gpu.setter
    def gpu(self, value):
        self._gpu = bool(int(value))
        # If cell_permute has been set to a value incompatible with the new GPU
        # setting, change it and print a warning.
        if (
            self._cell_permute is not None
            and self._cell_permute not in self._valid_cell_permute()
        ):
            print(
                "WARNING: coreneuron.gpu = {} forbids coreneuron.cell_permute = {}, using {} instead".format(
                    self._gpu, self._cell_permute, self._default_cell_permute()
                )
            )
            self._cell_permute = self._default_cell_permute()

    @property
    def file_mode(self):
        """Run via file transfer mode instead of in-memory transfer."""
        return self._file_mode

    @file_mode.setter
    def file_mode(self, value):
        self._file_mode = bool(value)

    @property
    def cell_permute(self):
        """0 no permutation; 1 optimize node adjacency; 2 optimize parent node
        adjacency (only for gpu = True)"""
        if self._cell_permute is None:
            return self._default_cell_permute()
        return self._cell_permute

    @cell_permute.setter
    def cell_permute(self, value):
        value = int(value)
        assert value in self._valid_cell_permute()
        self._cell_permute = value

    @property
    def warp_balance(self):
        """Number of warps to balance. (0 no balance)"""
        return self._warp_balance

    @warp_balance.setter
    def warp_balance(self, value):
        value = int(value)
        assert value >= 0
        self._warp_balance = value

    @property
    def verbose(self):
        """0 quiet, 1 Error, 2 Info, 3 Debug"""
        return self._verbose

    @verbose.setter
    def verbose(self, value):
        value = int(value)
        assert value >= 0 and value <= 3
        self._verbose = value

    @property
    def prcellstate(self):
        """Output prcellstate information for the gid at t=0 and at tstop. -1 means no output"""
        return self._prcellstate

    @prcellstate.setter
    def prcellstate(self, value):
        value = int(value)
        assert value >= -1
        self._prcellstate = value

    def property_check(self, tstop):
        """Legacy check of user property values."""
        tstop = float(tstop)

    def nrncore_arg(self, tstop):
        """
        Return str that can be used for pc.nrncore_run(str)
        based on user properties and current NEURON settings.
        :param tstop: Simulation time (ms)

        """
        from neuron import h

        arg = ""
        self.property_check(tstop)
        if not self.enable:
            return arg

        # note that this name is also used in C++ file nrncore_write.cpp
        CORENRN_DATA_DIR = "corenrn_data"

        # args derived from user properties
        arg += " --gpu" if self.gpu else ""
        arg += " --datpath %s" % CORENRN_DATA_DIR if self.file_mode else ""
        arg += " --tstop %g" % tstop
        arg += " --cell-permute %d" % self.cell_permute
        arg += (" --nwarp %d" % self.warp_balance) if self.warp_balance > 0 else ""
        arg += (" --prcellgid %d" % self.prcellstate) if self.prcellstate >= 0 else ""
        arg += " --verbose %d" % self.verbose

        # args derived from current NEURON settings.
        pc = h.ParallelContext()
        cvode = h.CVode()
        arg += " --mpi" if pc.nhost() > 1 else ""
        arg += " --voltage 1000."
        binqueue = cvode.queue_mode() & 1
        arg += " --binqueue" if binqueue else ""
        spkcompress = int(pc.spike_compress(-1))
        arg += (" --spkcompress %g" % spkcompress) if spkcompress else ""

        # since multisend is undocumented in NEURON, perhaps it would be best
        # to make the next three arg set from user properties here
        multisend = int(pc.send_time(8))
        if (multisend & 1) == 1:
            arg += " --multisend"
            interval = (multisend & 2) / 2 + 1
            arg += (" --ms_subinterval %d" % interval) if interval != 2 else ""
            phases = (multisend & 4) / 4 + 1
            arg += (" --ms_phases %d" % phases) if phases != 2 else ""

        return arg


sys.modules[__name__] = coreneuron()
