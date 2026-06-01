NEURON {
    SUFFIX recursion
}

FUNCTION fibonacci(n) {
    if (n == 0 || n == 1) {
        fibonacci = 1
    } else {
        fibonacci = fibonacci(n-1) + fibonacci(n-2)
    }
}
