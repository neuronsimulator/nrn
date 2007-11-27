NEURON {
	SUFFIX cacur
	USEION ca WRITE ica
	RANGE del, dur, amp
}

PARAMETER {
	del (ms)
	dur = 1 (ms)
	amp = -1 (milliamp/cm2)	: negative is inward
}

ASSIGNED {
	ica (milliamp/cm2)
}

BREAKPOINT {
	if (amp) {
		at_time(del)  at_time(del + dur)
	}
	if (t > del && t < del + dur) {
		ica = amp
	}else{
		ica = 0
	}
}

