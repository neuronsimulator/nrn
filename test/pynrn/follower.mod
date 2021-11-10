NEURON {
	ARTIFICIAL_CELL Follower
}

INITIAL {
}

NET_RECEIVE (w(ms)) {
	net_event(t + w)
}
