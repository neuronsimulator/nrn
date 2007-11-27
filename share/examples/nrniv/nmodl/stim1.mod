:this model is built-in to neuron with suffix stim
COMMENT
Note:
$NEURONHOME/nrnoc/SRC/stim.mod shows the correct current stimulus model.
The model here does not work correctly if the "extracellular" mechanism is also
inserted at the same location because the NONSPECIFIC_CURRENT is
 treated as a transmembrane current.
For electrodes, there is a special ELECTRODE_CURRENT directive in
the NEURON block that should be used. Also a positive ELECTRODE_CURRENT
is equivalent to an inward membrane current so the assignment to i has
a different sign.
ENDCOMMENT

NEURON {
	POINT_PROCESS stim1
	RANGE del, dur, amp, i
	NONSPECIFIC_CURRENT i
}
UNITS {
	(nA) = (nanoamp)
}

PARAMETER {
	del (ms)
	dur (ms)
	amp (nA)
}
ASSIGNED { i (nA) }

BREAKPOINT {
	if (t < del + dur && t > del) {
		i = -amp
	}else{
		i = 0
	}
}
