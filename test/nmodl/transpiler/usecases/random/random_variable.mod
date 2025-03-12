NEURON	{ 
    SUFFIX random_variable
    RANDOM rng
}


FUNCTION negexp() {
    negexp = random_negexp(rng)
}
