: can several thresholds be watched in a POINT_PROCESS?

NEURON {
	POINT_PROCESS t12
	RANGE th1, th2, vnext, del, id
}

PARAMETER { th1 = -50  th2 = -40  id = 0 }
ASSIGNED { v vnext del}

INITIAL {
	net_send(0, 1)
}

NET_RECEIVE(w) {
	: note that the conditions are persistent
	if (flag == 1) {
		WATCH (v > th1) 2
		WATCH (v > th2) 3
		WATCH (v < 20) 4
	}else if (flag != 0) {
		printf("%g %.15g v=%.15g flag=%g\n", id, t, v, flag)
	}
	if (flag == 4) {
		vnext = v - 10
		del = 10
		WATCH (v < vnext - del) 6
		WATCH (v < vnext) 5
	}
	if (flag > 4) {
		vnext = v - 10
		del = del/10
		WATCH (v < vnext - del) 6
		WATCH (v < vnext) 5
	}
}
