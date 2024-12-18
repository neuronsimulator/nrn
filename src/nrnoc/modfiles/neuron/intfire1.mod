NEURON {
	: ARTIFICIAL_CELL means
	: that this model not only has a NET_RECEIVE block and does NOT
	: have a BREAKPOINT but is not associated with a
	: section location or numerical integrator. i.e it does not
	: refer to v or any ions or have a POINTER. It is entirely isolated
	: and depends on discrete events from the outside to affect it and
	: affects the outside only by sending discrete events.
	ARTIFICIAL_CELL IntFire1
	RANGE tau, refrac, m
	: m plays the role of voltage
}

PARAMETER {
	tau = 10 (ms)
	refrac = 5 (ms)
}

ASSIGNED {
	m
	t0(ms)
	refractory
}

INITIAL {
	m = 0
	t0 = t
	refractory = 0 : 0-integrates input, 1-refractory
}

FUNCTION M() {
	if (refractory == 0) {
		M = m*exp(-(t - t0)/tau)
	}else if (refractory == 1) {
		if (t - t0 < .5) {
			M = 2
		}else{
			M = -1
		}
	}
}

NET_RECEIVE (w) {
	if (refractory == 0) { : inputs integrated only when excitable
		m = m*exp(-(t - t0)/tau)
		t0 = t
		m = m + w
		if (m > 1) {
			refractory = 1
			m = 2
			net_send(refrac, refractory)
			net_event(t)
		}
	}else if (flag == 1) { : ready to integrate again
		t0 = t
		refractory = 0
		m = 0
	}
}
