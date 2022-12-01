COMMENT
Since this is an electrode current, positive values of i depolarize the cell
and in the presence of the extracellular mechanism there will be a change
in vext since i is not a transmembrane current but a current injected
directly to the inside of the cell.
ENDCOMMENT

NEURON {
	POINT_PROCESS IClamp
	RANGE del, dur, amp, i
	ELECTRODE_CURRENT i
}
UNITS {
	(nA) = (nanoamp)
}

PARAMETER {
	del (ms)
	dur (ms)	<0,1e9>
	amp (nA)
}
ASSIGNED { i (nA) }

INITIAL {
	i = 0
}

BREAKPOINT {
    : for fixed step methos, we can ignore at_time, was introduced for variable timestep, will be deprecated anyway. 
	: at_time(del)
	: at_time(del+dur)

	if (t < del + dur && t >= del) {
		i = amp
	}else{
		i = 0
	}
}
