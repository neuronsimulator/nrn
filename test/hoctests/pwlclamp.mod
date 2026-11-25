COMMENT
Test-only piecewise-linear (PWL) electrode current stimulus for IDA IC work.

Mirrors continuous Vector.play tables used in test_ida_init_mode.py
(A1 ramp, A3 istep/kink/...) so Section models can receive the same jump and
kink waveforms as LinearMechanism force play.

IClamp-like: ELECTRODE_CURRENT i, at_time at each knot, BREAKPOINT assigns i(t).

Right-continuous at jumps (last knot index with tk[j] <= t). Past the last
knot: linear extrapolation of the last non-zero-length segment when possible;
otherwise hold the last value. (Matches continuous-play end rules used in A3.)

NOT a product mechanism. Does NOT register classical di/dt into Plan A
forcing t+ (play / LM.dforce remain the free-y' 1-jet sources).

Max NKT knots; set nkt and tk[i], ik[i] from Python/HOC.
ENDCOMMENT

NEURON {
	POINT_PROCESS PWLClamp
	RANGE i, nkt
	ELECTRODE_CURRENT i
}

UNITS {
	(nA) = (nanoamp)
}

PARAMETER {
	nkt = 0
	tk[16] (ms)
	ik[16] (nA)
}

ASSIGNED {
	i (nA)
}

INITIAL {
	i = ival(t)
}

BREAKPOINT {
	LOCAL j
	if (nkt > 0) {
		FROM j = 0 TO nkt-1 {
			at_time(tk[j])
		}
	}
	i = ival(t)
}

PROCEDURE set_knot(j, tval (ms), ival_ (nA)) {
	: Python/HOC cannot assign PARAMETER arrays directly on this PP;
	: use set_knot(j, t, i) for j = 0 .. nkt-1 after setting nkt.
	if (j >= 0 && j < 16) {
		tk[j] = tval
		ik[j] = ival_
	}
}

FUNCTION get_tk(j) (ms) {
	if (j >= 0 && j < 16) {
		get_tk = tk[j]
	} else {
		get_tk = 0
	}
}

FUNCTION get_ik(j) (nA) {
	if (j >= 0 && j < 16) {
		get_ik = ik[j]
	} else {
		get_ik = 0
	}
}

FUNCTION ival(tt (ms)) (nA) {
	LOCAL j, jmax, t0, t1, i0, i1
	if (nkt < 1) {
		ival = 0
	} else if (tt < tk[0]) {
		: hold first knot value before the first time (play-like)
		ival = ik[0]
	} else if (tt >= tk[nkt-1]) {
		: at/after last knot: extrap last non-degenerate segment if any
		if (nkt >= 2) {
			j = nkt - 2
			WHILE (j > 0 && tk[j+1] <= tk[j]) {
				j = j - 1
			}
			if (tk[j+1] > tk[j]) {
				ival = ik[j+1] + (ik[j+1] - ik[j]) * (tt - tk[j+1]) / (tk[j+1] - tk[j])
			} else {
				ival = ik[nkt-1]
			}
		} else {
			ival = ik[0]
		}
	} else {
		: last index j with tk[j] <= tt (right-continuous at coincident knots)
		jmax = 0
		FROM j = 0 TO nkt-1 {
			if (tk[j] <= tt) {
				jmax = j
			}
		}
		if (jmax >= nkt-1) {
			ival = ik[nkt-1]
		} else if (tk[jmax+1] > tk[jmax]) {
			t0 = tk[jmax]
			t1 = tk[jmax+1]
			i0 = ik[jmax]
			i1 = ik[jmax+1]
			ival = i0 + (i1 - i0) * (tt - t0) / (t1 - t0)
		} else {
			: degenerate (jump) segment: sit on right value at this index
			ival = ik[jmax]
		}
	}
}
