NEURON {
    SUFFIX nonlin
}

STATE { x }

FUNCTION solve() {
    : Iterative solvers need a starting guess.
    x = 1.0
    SOLVE eq

    solve = x
}

NONLINEAR eq {
    ~ x*x = 4.0
}

FUNCTION residual(x) {
    residual = x - 2.0
}
