: natrans.mod
: receive nai

NEURON {
    POINT_PROCESS NaTrans
    RANGE napre, sgid
}

PARAMETER {
    sgid = -1
}

ASSIGNED {
    napre (milli/liter)
}

