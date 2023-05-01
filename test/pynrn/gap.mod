COMMENT
changed this copy of nrn/examples/nrniv/nmodl/gap.mod file to be
an ELECTRODE_CURRENT so that the gap current does not contribute to the
membrane current and therefore the gap current is properly ignored in
the presence of the extracellular mechanism.  Also, as a consequence,
since positive ELECTRODE_CURRENT depolarizes, the (v - vgap) was
reversed.  Lastly, the vgap was changed to an ASSIGNED RANGE variable
instead of POINTER since this version of gap.mod is intended to be used
with a -with-paranrn configured version of NEURON and the gap junctions
are connected with the ParallelContext.(source_var target_var) methods. 
ENDCOMMENT

NEURON {
	POINT_PROCESS HalfGap
	ELECTRODE_CURRENT i
	RANGE r, i, vgap
}

PARAMETER {
	r = 1e10 (megohm)
}

ASSIGNED {
	v (millivolt)
	vgap (millivolt)
	i (nanoamp)
}

BREAKPOINT {
	i = (vgap - v)/r
}

