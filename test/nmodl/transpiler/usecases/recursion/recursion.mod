NEURON {
    SUFFIX recursion
}

FUNCTION myfactorial(n) {
    if (n == 0 || n == 1) {
        myfactorial = 1
    } else {
        myfactorial = n * myfactorial(n - 1)
    }
} 
