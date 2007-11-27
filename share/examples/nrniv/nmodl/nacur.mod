NEURON {
	SUFFIX nacur
	USEION na WRITE ina
	RANGE del, dur, amp
}

PARAMETER {
	del (ms)
	dur = 1 (ms)
	amp = -1 (milliamp/cm2)	: negative is inward
}

ASSIGNED {
	ina (milliamp/cm2)
}

BREAKPOINT {
	if (amp) {
		at_time(del)  at_time(del + dur)
	}
	if (t > del && t < del + dur) {
		ina = amp
	}else{
		ina = 0
	}
}

