COMMENT
This is a copy of netstim.mod and created to demonstrate
the API of new RANDOM construct support in NMODL. All the
VERBATIM blocks, save-state feature and compatibility mode
with old hocRandom is removed.

Once all the futures are implemented in NEURON, the idea is
that this MOD file should replace netstim.mod.
ENDCOMMENT

NEURON {
    ARTIFICIAL_CELL NetStimRNG
    RANGE interval, number, start
    RANGE noise
    RANGE id
    RANDOM NEGEXP(1.0) rrr, xxx
    RANDOM NEGEXP(2.0) yyy
}

PARAMETER {
    interval    = 10 (ms) <1e-9,1e9>    : time between spikes (msec)
    number      = 10 <0,1e9>            : number of spikes (independent of noise)
    start       = 50 (ms)               : start of first spike
    noise       = 0 <0,1>               : amount of randomness (0.0 - 1.0)
    id          = 0                     : id to demonstrate RNG initialization
}

ASSIGNED {
    event (ms)
    on
    ispike
    donotuse
}

INITIAL {
    : a way to initialize in MOD file
    rrr.init(id, 2, 3)

    : a way to set stream id
    rrr.setseq(0, 0)

    on = 0 : off
    ispike = 0
    if (noise < 0) {
        noise = 0
    }
    if (noise > 1) {
        noise = 1
    }
    if (start >= 0 && number > 0) {
        on = 1
        : randomize the first spike so on average it occurs at
        : start + noise*interval
        event = start + invl(interval) - interval*(1. - noise)
        : but not earlier than 0
        if (event < 0) {
            event = 0
        }
        net_send(event, 3)
    }
}

PROCEDURE init_sequence(t(ms)) {
    if (number > 0) {
        on = 1
        event = 0
        ispike = 0
    }
}

FUNCTION invl(mean (ms)) (ms) {
    if (mean <= 0.) {
        mean = .01 (ms) : I would worry if it were 0.
    }
    if (noise == 0) {
        invl = mean
    } else {
        invl = (1. - noise)*mean + noise*mean*erand()
    }
}

FUNCTION erand() {
    : a way to sample/peek from RNG
    erand = rrr.sample()
}

PROCEDURE next_invl() {
    if (number > 0) {
        event = invl(interval)
    }
    if (ispike >= number) {
        on = 0
    }
}

NET_RECEIVE (w) {
    if (flag == 0) { : external event
        if (w > 0 && on == 0) { : turn on spike sequence
            : but not if a netsend is on the queue
            init_sequence(t)
            : randomize the first spike so on average it occurs at
            : noise*interval (most likely interval is always 0)
            next_invl()
            event = event - interval*(1. - noise)
            net_send(event, 1)
        }else if (w < 0) { : turn off spiking definitively
            on = 0
        }
    }
    if (flag == 3) { : from INITIAL
        if (on == 1) { : but ignore if turned off by external event
            init_sequence(t)
            net_send(0, 1)
        }
    }
    if (flag == 1 && on == 1) {
        ispike = ispike + 1
        net_event(t)
        next_invl()
        if (on == 1) {
            net_send(event, 1)
        }
    }
}
