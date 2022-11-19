: More or less nonsensical test of NONLINEAR block


NEURON {
    ARTIFICIAL_CELL NonLinTest
    RANGE x0, a, y0, b, z0, c
}

PARAMETER { x0 a y0 b z0 c }

STATE { x y z }

INITIAL {
    x = x0
    y = y0
    z = z0
    SOLVE nonlin
}

NONLINEAR nonlin {
    ~ x + y*z = a + b*c
    ~ y + z*x = b + c*a
    ~ z + x*y = c + a*b
}

FUNCTION dif() {
    dif = fabs((x + y*z) - (a + b*c))
    dif = dif + fabs((y + z*x) - (b + c*a))
    dif = dif + fabs(( z + x*y) - (c + a*b))
}
    
